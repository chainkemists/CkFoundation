#include "CkVfx_Processor.h"

#include "CkVfx_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkFx/CkFx_Log.h"
#include "CkFx/CkFx_Stats.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

#include <NiagaraComponent.h>
#include <NiagaraFunctionLibrary.h>
#include <NiagaraSystem.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Vfx_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Vfx_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Vfx_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Vfx_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Fx::VfxSpawnAttached"),   STAT_Fx_VfxSpawnAttached,   STATGROUP_CkFx);
DECLARE_CYCLE_STAT(TEXT("Fx::VfxSpawnAtLocation"), STAT_Fx_VfxSpawnAtLocation, STATGROUP_CkFx);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Vfx_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent)
            -> void
    {
        const auto& Params = InParams.Get_Params();

        if (NOT InCurrent._LoadedAssets.Get_IsRequested())
        {
            // An unset system is a legal composition (mirrors Sfx's unset-cue semantics): the vfx
            // composes inert and the PLAY path is where an unset/unresolved system gets loud.
            if (ck::Is_NOT_Valid(Params.Get_ParticleSystem()))
            {
                fx::Verbose(TEXT("Vfx [{}] composed without a ParticleSystem - setting up inert"), InHandle);
                InHandle.Remove<MarkedDirtyBy>();
                return;
            }

            InCurrent._LoadedAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                TEXT("Vfx.Setup"), {Params.Get_ParticleSystem().ToSoftObjectPath()});
        }

        if (NOT InCurrent._LoadedAssets.Get_IsReady())
        {
            InHandle.AddOrGet<FTag_Vfx_PendingAssetLoad>();
            return;
        }

        const auto ResolvedSystem = Cast<UNiagaraSystem>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_ParticleSystem().ToSoftObjectPath()));
        const auto AssetsAreLoaded = NOT InCurrent._LoadedAssets.Get_HasFailed() && ck::IsValid(ResolvedSystem);

        CK_ENSURE_IF_NOT(AssetsAreLoaded,
            TEXT("Cannot setup Vfx [{}] - loading its ParticleSystem [{}] through CkResourceLoader failed"),
            InHandle, Params.Get_ParticleSystem().ToSoftObjectPath())
        { InCurrent._LoadedAssets = {}; }

        InHandle.Try_Remove<FTag_Vfx_PendingAssetLoad>();
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Vfx_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent,
            FFragment_Vfx_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            Result = DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_Vfx_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent,
            const FCk_Request_Vfx_PlayAttached& InRequest)
        -> ECk_Request_OperationResult
    {
        SCOPE_CYCLE_COUNTER(STAT_Fx_VfxSpawnAttached);

        const auto& Params = InParams.Get_Params();

        const auto ResolvedSystem = Cast<UNiagaraSystem>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_ParticleSystem().ToSoftObjectPath()));
        const auto SystemIsResolved = ck::IsValid(ResolvedSystem);

        CK_ENSURE_IF_NOT(SystemIsResolved, TEXT("Vfx [{}] cannot play - its ParticleSystem is not resolved "
            "(unset, the asset load failed at Setup, or the loader-rooted asset was lost)"), InHandle)
        { return ECk_Request_OperationResult::Failed; }

        // The attach component may legitimately die between enqueue and drain — a Failed completion,
        // not an ensure.
        const auto AttachComponent = InRequest.Get_AttachComponent().Get();
        if (ck::Is_NOT_Valid(AttachComponent))
        {
            ck::fx::Verbose(TEXT("Vfx [{}] skipping PlayAttached - the attach component is no longer valid"), InHandle);
            return ECk_Request_OperationResult::Failed;
        }

        const auto& AttachmentSettings = Params.Get_AttachmentSettings();
        const auto& TransformRules     = AttachmentSettings.Get_TransformRules();

        constexpr auto PreCull = true;
        const auto& SpawnedVfx = UNiagaraFunctionLibrary::SpawnSystemAttached
        (
            ResolvedSystem,
            AttachComponent,
            AttachmentSettings.Get_SocketName(),
            InRequest.Get_RelativeTransformSettings().Get_Location(),
            InRequest.Get_RelativeTransformSettings().Get_Rotation(),
            EAttachLocation::Type::KeepRelativeOffset,
            PreCull
        );

        // This may be invalid if it is pre-culled
        if (ck::Is_NOT_Valid(SpawnedVfx))
        { return ECk_Request_OperationResult::Failed; }

        INC_DWORD_STAT(STAT_Fx_EffectsSpawned);

        UCk_Utils_Vfx_UE::DoSet_NiagaraInstanceParameter(SpawnedVfx, InRequest.Get_InstanceParameterSettings());

        SpawnedVfx->SetAbsolute
        (
            TransformRules.Get_LocationPolicy() == ECk_VFX_Location_Policy::UseAbsoluteLocation,
            TransformRules.Get_RotationPolicy() == ECk_VFX_Rotation_Policy::UseAbsoluteRotation,
            TransformRules.Get_ScalePolicy()    == ECk_VFX_Scale_Policy::UseAbsoluteScale
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Vfx_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent,
            const FCk_Request_Vfx_PlayAtLocation& InRequest)
        -> ECk_Request_OperationResult
    {
        SCOPE_CYCLE_COUNTER(STAT_Fx_VfxSpawnAtLocation);

        const auto& Params = InParams.Get_Params();

        const auto ResolvedSystem = Cast<UNiagaraSystem>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_ParticleSystem().ToSoftObjectPath()));
        const auto SystemIsResolved = ck::IsValid(ResolvedSystem);

        CK_ENSURE_IF_NOT(SystemIsResolved, TEXT("Vfx [{}] cannot play - its ParticleSystem is not resolved "
            "(unset, the asset load failed at Setup, or the loader-rooted asset was lost)"), InHandle)
        { return ECk_Request_OperationResult::Failed; }

        // The outer may legitimately die between enqueue and drain — a Failed completion, not an ensure.
        const auto Outer = InRequest.Get_Outer().Get();
        if (ck::Is_NOT_Valid(Outer))
        {
            ck::fx::Verbose(TEXT("Vfx [{}] skipping PlayAtLocation - the outer is no longer valid"), InHandle);
            return ECk_Request_OperationResult::Failed;
        }

        const auto& AttachmentSettings = Params.Get_AttachmentSettings();
        const auto& TransformRules     = AttachmentSettings.Get_TransformRules();

        const auto& SpawnedVfx = UNiagaraFunctionLibrary::SpawnSystemAtLocation
        (
            Outer,
            ResolvedSystem,
            InRequest.Get_TransformSettings().Get_Location(),
            InRequest.Get_TransformSettings().Get_Rotation()
        );

        // This may be invalid if it is pre-culled
        if (ck::Is_NOT_Valid(SpawnedVfx))
        { return ECk_Request_OperationResult::Failed; }

        INC_DWORD_STAT(STAT_Fx_EffectsSpawned);

        UCk_Utils_Vfx_UE::DoSet_NiagaraInstanceParameter(SpawnedVfx, InRequest.Get_InstanceParameterSettings());

        SpawnedVfx->SetAbsolute
        (
            TransformRules.Get_LocationPolicy() == ECk_VFX_Location_Policy::UseAbsoluteLocation,
            TransformRules.Get_RotationPolicy() == ECk_VFX_Rotation_Policy::UseAbsoluteRotation,
            TransformRules.Get_ScalePolicy()    == ECk_VFX_Scale_Policy::UseAbsoluteScale
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Vfx_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vfx_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Vfx_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Vfx_Current& InCurrent)
            -> void
    {
        InCurrent._LoadedAssets = {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
