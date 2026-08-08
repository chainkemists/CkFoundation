#include "CkVfxCue_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkVfx/CkVfx_Log.h"
#include "CkVfx/CkVfx_Stats.h"
#include "CkVfxCue_Utils.h"

#include <NiagaraComponent.h>
#include <NiagaraFunctionLibrary.h>

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Vfx::CueLifetimeMonitor"), STAT_Vfx_CueLifetimeMonitor, STATGROUP_CkVfx);
DECLARE_DWORD_COUNTER_STAT(TEXT("Vfx Active Managed Effects"), STAT_Vfx_ActiveManagedEffects, STATGROUP_CkVfx);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_VfxCue_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VfxCue_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VfxCue_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VfxCue_EffectLifetimeMonitor);
CK_REGISTER_PROCESSOR(ck::FProcessor_VfxCue_EndPlay);

namespace ck
{
    auto
        FProcessor_VfxCue_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VfxCue_Params& InParams,
            FFragment_VfxCue& InVfxCue,
            const FFragment_EntityScript& InEntityScript)
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::vfx::Verbose(TEXT("Setting up VfxCue [{}]"), InHandle);

        const auto VfxCueScript = Cast<UCk_VfxCue_EntityScript>(InEntityScript.Get_Script().Get());
        CK_ENSURE_IF_NOT(ck::IsValid(VfxCueScript),
            TEXT("VfxCue [{}] does not have valid VfxCue EntityScript"), InHandle)
        { return; }

        CK_ENSURE_IF_NOT(VfxCueScript->Get_IsConfigurationValid(),
            TEXT("VfxCue [{}] has invalid configuration"), InHandle)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("VfxCue [{}] could not get valid World"), InHandle)
        { return; }

        const auto& SpawnTransform = VfxCueScript->Get_SpawnTransform();

        // Both must stay false (see CkVfx/Claude.md, Anti-patterns): Niagara auto-destroy dangles the
        // monitor/EndPlay component pointer, and auto-activate skips OnStarted plus start-time bookkeeping.
        constexpr auto AutoDestroy = false;
        constexpr auto AutoActivate = false;

        const auto PreCullCheck = InParams.Get_PreCullCheck();

        auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            VfxCueScript->Get_Effect(),
            SpawnTransform.GetLocation(),
            SpawnTransform.GetRotation().Rotator(),
            SpawnTransform.GetScale3D(),
            AutoDestroy,
            AutoActivate,
            ENCPoolMethod::None,
            PreCullCheck
        );

        CK_ENSURE_IF_NOT(ck::IsValid(NiagaraComponent),
            TEXT("VfxCue [{}] failed to spawn NiagaraComponent"), InHandle)
        { return; }

        for (const auto& Param : VfxCueScript->Get_UserParameters())
        {
            switch (Param.Get_ParameterType())
            {
                case ECk_VfxCue_ParameterType::Float:
                    NiagaraComponent->SetFloatParameter(Param.Get_ParameterName(), Param.Get_FloatValue());
                    break;
                case ECk_VfxCue_ParameterType::Vector:
                    NiagaraComponent->SetVectorParameter(Param.Get_ParameterName(), Param.Get_VectorValue());
                    break;
                case ECk_VfxCue_ParameterType::Color:
                    NiagaraComponent->SetColorParameter(Param.Get_ParameterName(), Param.Get_ColorValue());
                    break;
                case ECk_VfxCue_ParameterType::Bool:
                    NiagaraComponent->SetBoolParameter(Param.Get_ParameterName(), Param.Get_BoolValue());
                    break;
                case ECk_VfxCue_ParameterType::Int:
                    NiagaraComponent->SetIntParameter(Param.Get_ParameterName(), Param.Get_IntValue());
                    break;
                default:
                    CK_INVALID_ENUM(Param.Get_ParameterType());
                    break;
            }
        }

        InVfxCue._NiagaraComponent = TStrongObjectPtr{NiagaraComponent};
        InVfxCue._HasFiredFinished = false;

        switch (VfxCueScript->Get_DurationMode())
        {
            case ECk_VfxCue_DurationMode::UseSystemDuration:
            {
                InVfxCue._EffectDuration = FCk_Time{10.0f};
                ck::vfx::Verbose(TEXT("VfxCue [{}] using default duration fallback: 10s"), InHandle);
                break;
            }
            case ECk_VfxCue_DurationMode::Override:
                InVfxCue._EffectDuration = VfxCueScript->Get_DurationOverride();
                break;
            case ECk_VfxCue_DurationMode::Infinite:
                InVfxCue._EffectDuration = FCk_Time{-1.0f};
                break;
            default:
                CK_INVALID_ENUM(VfxCueScript->Get_DurationMode());
                break;
        }

        ck::vfx::VeryVerbose(TEXT("VfxCue [{}] setup complete, duration: [{}]"),
            InHandle, InVfxCue._EffectDuration);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VfxCue_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue& InVfxCue,
            const FFragment_EntityScript& InEntityScript,
            const FFragment_VfxCue_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_VfxCue_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                if (DoHandleRequest(InHandle, InVfxCue, InEntityScript, InRequest))
                { Result = ECk_Request_OperationResult::Succeeded; }

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_VfxCue_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VfxCue& InVfxCue,
            const FFragment_EntityScript& InEntityScript,
            const FCk_Request_VfxCue_Play& InRequest)
            -> bool
    {
        const auto VfxCueScript = Cast<UCk_VfxCue_EntityScript>(InEntityScript.Get_Script().Get());
        CK_ENSURE_IF_NOT(ck::IsValid(VfxCueScript),
            TEXT("VfxCue [{}] does not have valid VfxCue EntityScript"), InHandle)
        { return false; }

        ck::vfx::Verbose(TEXT("Handling play request for VfxCue [{}]"), InHandle);

        auto NiagaraComponent = InVfxCue._NiagaraComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(NiagaraComponent),
            TEXT("VfxCue [{}] has invalid NiagaraComponent"), InHandle)
        { return false; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("VfxCue [{}] could not get valid World"), InHandle)
        { return false; }

        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);

        constexpr auto ResetOnActivate = true;
        NiagaraComponent->Activate(ResetOnActivate);
        InVfxCue._EffectStartTime = TimeResult.Get_WorldTime().Get_Time();
        InVfxCue._HasFiredFinished = false;

        InHandle.Add<FTag_VfxCue_IsPlaying>();

        ck::vfx::Verbose(TEXT("VfxCue [{}] started playing at time [{}]"),
            InHandle, InVfxCue._EffectStartTime);

        UUtils_Signal_OnVfxCue_Started::Broadcast(InHandle, MakePayload(InHandle));

        return true;
    }

    auto
        FProcessor_VfxCue_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VfxCue& InVfxCue,
            const FFragment_EntityScript& InEntityScript,
            const FCk_Request_VfxCue_Stop& InRequest)
            -> bool
    {
        ck::vfx::Verbose(TEXT("Handling stop request for VfxCue [{}]"), InHandle);

        auto NiagaraComponent = InVfxCue._NiagaraComponent.Get();
        if (ck::IsValid(NiagaraComponent))
        {
            NiagaraComponent->Deactivate();
        }

        InHandle.Remove<FTag_VfxCue_IsPlaying>();

        if (NOT InVfxCue._HasFiredFinished)
        {
            InVfxCue._HasFiredFinished = true;
            UUtils_Signal_OnVfxCue_Finished::Broadcast(InHandle, MakePayload(InHandle));
        }

        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VfxCue_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VfxCue_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VfxCue_EffectLifetimeMonitor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue& InVfxCue)
            -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Vfx_CueLifetimeMonitor);
        INC_DWORD_STAT(STAT_Vfx_ActiveManagedEffects);

        auto NiagaraComponent = InVfxCue._NiagaraComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(NiagaraComponent),
            TEXT("VfxCue [{}] has invalid NiagaraComponent"), InHandle)
        { return; }

        if (NiagaraComponent->IsActive() == false && NOT InVfxCue._HasFiredFinished)
        {
            ck::vfx::Verbose(TEXT("VfxCue [{}] system finished, firing OnFinished"), InHandle);

            InVfxCue._HasFiredFinished = true;
            InHandle.Remove<FTag_VfxCue_IsPlaying>();

            UUtils_Signal_OnVfxCue_Finished::Broadcast(InHandle, MakePayload(InHandle));
            return;
        }

        if (InVfxCue._EffectDuration.Get_Seconds() > 0.0f)
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            CK_ENSURE_IF_NOT(ck::IsValid(World),
                TEXT("VfxCue [{}] could not get valid World"), InHandle)
            { return; }

            const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
            const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
            const auto CurrentTime = TimeResult.Get_WorldTime().Get_Time();

            const auto ElapsedTime = CurrentTime - InVfxCue._EffectStartTime;

            if (ElapsedTime >= InVfxCue._EffectDuration && NOT InVfxCue._HasFiredFinished)
            {
                ck::vfx::Verbose(TEXT("VfxCue [{}] duration timeout reached, firing OnFinished"), InHandle);

                InVfxCue._HasFiredFinished = true;
                InHandle.Remove<FTag_VfxCue_IsPlaying>();
                NiagaraComponent->Deactivate();

                UUtils_Signal_OnVfxCue_Finished::Broadcast(InHandle, MakePayload(InHandle));
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VfxCue_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue& InVfxCue)
            -> void
    {
        ck::vfx::Verbose(TEXT("Tearing down VfxCue [{}]"), InHandle);

        auto NiagaraComponent = InVfxCue._NiagaraComponent.Get();
        if (ck::IsValid(NiagaraComponent))
        {
            NiagaraComponent->DestroyComponent();
        }

        InVfxCue._NiagaraComponent.Reset();
        InVfxCue._HasFiredFinished = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
