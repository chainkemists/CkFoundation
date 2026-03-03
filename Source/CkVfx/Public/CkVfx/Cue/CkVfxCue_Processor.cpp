#include "CkVfxCue_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkVfx/CkVfx_Log.h"
#include "CkVfxCue_Utils.h"

#include <NiagaraComponent.h>
#include <NiagaraFunctionLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VfxCue_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VfxCue_Params& InParams,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript)
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

        // AutoDestroy must remain false: the NiagaraComponent's lifetime is managed by this ECS system
        // (FProcessor_VfxCue_EffectLifetimeMonitor + FProcessor_VfxCue_EndPlay). If Niagara auto-destroys
        // the component, the monitor's IsActive() check and EndPlay's DestroyComponent() call would operate
        // on a dangling pointer.
        constexpr auto AutoDestroy = false;

        // AutoActivate must remain false: activation is driven by FCk_Request_VfxCue_Play going through the
        // request queue, even for AutoPlay mode. Bypassing this would skip the OnStarted signal and the
        // effect start-time bookkeeping in FProcessor_VfxCue_HandleRequests.
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

        InCurrent._NiagaraComponent = TStrongObjectPtr{NiagaraComponent};
        InCurrent._HasFiredFinished = false;

        switch (VfxCueScript->Get_DurationMode())
        {
            case ECk_VfxCue_DurationMode::UseSystemDuration:
            {
                InCurrent._EffectDuration = FCk_Time{10.0f};
                ck::vfx::Verbose(TEXT("VfxCue [{}] using default duration fallback: 10s"), InHandle);
                break;
            }
            case ECk_VfxCue_DurationMode::Override:
                InCurrent._EffectDuration = VfxCueScript->Get_DurationOverride();
                break;
            case ECk_VfxCue_DurationMode::Infinite:
                InCurrent._EffectDuration = FCk_Time{-1.0f};
                break;
            default:
                CK_INVALID_ENUM(VfxCueScript->Get_DurationMode());
                break;
        }

        ck::vfx::VeryVerbose(TEXT("VfxCue [{}] setup complete, duration: [{}]"),
            InHandle, InCurrent._EffectDuration);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VfxCue_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FFragment_VfxCue_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_VfxCue_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InEntityScript, InRequest);

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
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_VfxCue_Play& InRequest)
            -> void
    {
        const auto VfxCueScript = Cast<UCk_VfxCue_EntityScript>(InEntityScript.Get_Script().Get());
        CK_ENSURE_IF_NOT(ck::IsValid(VfxCueScript),
            TEXT("VfxCue [{}] does not have valid VfxCue EntityScript"), InHandle)
        { return; }

        ck::vfx::Verbose(TEXT("Handling play request for VfxCue [{}]"), InHandle);

        auto NiagaraComponent = InCurrent._NiagaraComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(NiagaraComponent),
            TEXT("VfxCue [{}] has invalid NiagaraComponent"), InHandle)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("VfxCue [{}] could not get valid World"), InHandle)
        { return; }

        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);

        constexpr auto ResetOnActivate = true;
        NiagaraComponent->Activate(ResetOnActivate);
        InCurrent._EffectStartTime = TimeResult.Get_WorldTime().Get_Time();
        InCurrent._HasFiredFinished = false;

        InHandle.Add<FTag_VfxCue_IsPlaying>();

        ck::vfx::Verbose(TEXT("VfxCue [{}] started playing at time [{}]"),
            InHandle, InCurrent._EffectStartTime);

        UUtils_Signal_OnVfxCue_Started::Broadcast(InHandle, MakePayload(InHandle));
    }

    auto
        FProcessor_VfxCue_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_VfxCue_Stop& InRequest)
            -> void
    {
        ck::vfx::Verbose(TEXT("Handling stop request for VfxCue [{}]"), InHandle);

        auto NiagaraComponent = InCurrent._NiagaraComponent.Get();
        if (ck::IsValid(NiagaraComponent))
        {
            NiagaraComponent->Deactivate();
        }

        InHandle.Remove<FTag_VfxCue_IsPlaying>();

        if (NOT InCurrent._HasFiredFinished)
        {
            InCurrent._HasFiredFinished = true;
            UUtils_Signal_OnVfxCue_Finished::Broadcast(InHandle, MakePayload(InHandle));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VfxCue_EffectLifetimeMonitor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent)
            -> void
    {
        auto NiagaraComponent = InCurrent._NiagaraComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(NiagaraComponent),
            TEXT("VfxCue [{}] has invalid NiagaraComponent"), InHandle)
        { return; }

        if (NiagaraComponent->IsActive() == false && NOT InCurrent._HasFiredFinished)
        {
            ck::vfx::Verbose(TEXT("VfxCue [{}] system finished, firing OnFinished"), InHandle);

            InCurrent._HasFiredFinished = true;
            InHandle.Remove<FTag_VfxCue_IsPlaying>();

            UUtils_Signal_OnVfxCue_Finished::Broadcast(InHandle, MakePayload(InHandle));
            return;
        }

        if (InCurrent._EffectDuration.Get_Seconds() > 0.0f)
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            CK_ENSURE_IF_NOT(ck::IsValid(World),
                TEXT("VfxCue [{}] could not get valid World"), InHandle)
            { return; }

            const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
            const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
            const auto CurrentTime = TimeResult.Get_WorldTime().Get_Time();

            const auto ElapsedTime = CurrentTime - InCurrent._EffectStartTime;

            if (ElapsedTime >= InCurrent._EffectDuration && NOT InCurrent._HasFiredFinished)
            {
                ck::vfx::Verbose(TEXT("VfxCue [{}] duration timeout reached, firing OnFinished"), InHandle);

                InCurrent._HasFiredFinished = true;
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
            FFragment_VfxCue_Current& InCurrent)
            -> void
    {
        ck::vfx::Verbose(TEXT("Tearing down VfxCue [{}]"), InHandle);

        auto NiagaraComponent = InCurrent._NiagaraComponent.Get();
        if (ck::IsValid(NiagaraComponent))
        {
            NiagaraComponent->DestroyComponent();
        }

        InCurrent._NiagaraComponent.Reset();
        InCurrent._HasFiredFinished = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
