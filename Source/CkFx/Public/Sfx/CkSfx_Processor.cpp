#include "CkSfx_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkFx/CkFx_Log.h"
#include "CkFx/CkFx_Stats.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sfx_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sfx_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sfx_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sfx_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Fx::SfxSpawnAttached"),   STAT_Fx_SfxSpawnAttached,   STATGROUP_CkFx);
DECLARE_CYCLE_STAT(TEXT("Fx::SfxSpawnAtLocation"), STAT_Fx_SfxSpawnAtLocation, STATGROUP_CkFx);

// "Fx Effects Spawned" counter is the single shared stat declared EXTERN in CkFx_Stats.h (defined in CkVfx_Utils.cpp).

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Sfx_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent)
            -> void
    {
        const auto& Params = InParams.Get_Params();

        if (NOT InCurrent._LoadedAssets.Get_IsRequested())
        {
            // An unset cue is a legal composition (pinned by Ck_AutoTest_Sfx_Add_CreatesValidHandle):
            // the sfx composes inert and the PLAY path is where an unset/unresolved cue gets loud.
            if (ck::Is_NOT_Valid(Params.Get_SoundCue()))
            {
                fx::Verbose(TEXT("Sfx [{}] composed without a SoundCue - setting up inert"), InHandle);
                InHandle.Remove<MarkedDirtyBy>();
                return;
            }

            auto PathsToLoad = TArray<FSoftObjectPath>{};
            PathsToLoad.Emplace(Params.Get_SoundCue().ToSoftObjectPath());

            if (ck::IsValid(Params.Get_AttenuationSettings()))
            { PathsToLoad.Emplace(Params.Get_AttenuationSettings().ToSoftObjectPath()); }
            if (ck::IsValid(Params.Get_ConcurrencySettings()))
            { PathsToLoad.Emplace(Params.Get_ConcurrencySettings().ToSoftObjectPath()); }

            InCurrent._LoadedAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                TEXT("Sfx.Setup"), PathsToLoad);
        }

        if (NOT InCurrent._LoadedAssets.Get_IsReady())
        {
            InHandle.AddOrGet<FTag_Sfx_PendingAssetLoad>();
            return;
        }

        const auto ResolvedSoundCue = Cast<USoundBase>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_SoundCue().ToSoftObjectPath()));
        const auto AssetsAreLoaded = NOT InCurrent._LoadedAssets.Get_HasFailed() && ck::IsValid(ResolvedSoundCue);

        CK_ENSURE_IF_NOT(AssetsAreLoaded,
            TEXT("Cannot setup Sfx [{}] - loading its SoundCue [{}] (or a settings asset) through CkResourceLoader failed"),
            InHandle, Params.Get_SoundCue().ToSoftObjectPath())
        {}

        if (NOT AssetsAreLoaded)
        { InCurrent._LoadedAssets = {}; }

        InHandle.Try_Remove<FTag_Sfx_PendingAssetLoad>();
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sfx_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent,
            FFragment_Sfx_Requests& InRequestsComp) const
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
        FProcessor_Sfx_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent,
            const FCk_Request_Sfx_PlayAttached& InRequest)
        -> ECk_Request_OperationResult
    {
        SCOPE_CYCLE_COUNTER(STAT_Fx_SfxSpawnAttached);

        const auto& Params = InParams.Get_Params();

        const auto ResolvedSoundCue = Cast<USoundBase>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_SoundCue().ToSoftObjectPath()));
        const auto SoundCueIsResolved = ck::IsValid(ResolvedSoundCue);

        CK_ENSURE_IF_NOT(SoundCueIsResolved, TEXT("Sfx [{}] cannot play - its SoundCue is not resolved "
            "(the asset load failed at Setup, or the loader-rooted asset was lost)"), InHandle)
        {}

        if (NOT SoundCueIsResolved)
        { return ECk_Request_OperationResult::Failed; }

        // The attach component may legitimately die between enqueue and drain — a Failed completion,
        // not an ensure.
        const auto AttachComponent = InRequest.Get_AttachComponent().Get();
        if (ck::Is_NOT_Valid(AttachComponent))
        {
            ck::fx::Verbose(TEXT("Sfx [{}] skipping PlayAttached - the attach component is no longer valid"), InHandle);
            return ECk_Request_OperationResult::Failed;
        }

        const auto& AudioSettings = InRequest.Get_OverrideAudioSettings()
                                        ? InRequest.Get_OverridenAudioSettings()
                                        : Params.Get_DefaultAudioSettings();

        const auto ResolvedAttenuation = Cast<USoundAttenuation>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_AttenuationSettings().ToSoftObjectPath()));
        const auto ResolvedConcurrency = Cast<USoundConcurrency>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_ConcurrencySettings().ToSoftObjectPath()));

        constexpr auto StartTime = 0.0f;
        UGameplayStatics::SpawnSoundAttached
        (
            ResolvedSoundCue,
            AttachComponent,
            NAME_None,
            InRequest.Get_RelativeTransformSettings().Get_Location(),
            InRequest.Get_RelativeTransformSettings().Get_Rotation(),
            EAttachLocation::Type::KeepRelativeOffset,
            InRequest.Get_StopPolicy() == ECk_Sfx_Stop_Policy::StopWhenFinishedOrDetached,
            AudioSettings.Get_VolumeMultipler(),
            AudioSettings.Get_PitchMultipler(),
            StartTime,
            ResolvedAttenuation,
            ResolvedConcurrency
        );

        INC_DWORD_STAT(STAT_Fx_EffectsSpawned);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Sfx_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent,
            const FCk_Request_Sfx_PlayAtLocation& InRequest)
        -> ECk_Request_OperationResult
    {
        SCOPE_CYCLE_COUNTER(STAT_Fx_SfxSpawnAtLocation);

        const auto& Params = InParams.Get_Params();

        const auto ResolvedSoundCue = Cast<USoundBase>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_SoundCue().ToSoftObjectPath()));
        const auto SoundCueIsResolved = ck::IsValid(ResolvedSoundCue);

        CK_ENSURE_IF_NOT(SoundCueIsResolved, TEXT("Sfx [{}] cannot play - its SoundCue is not resolved "
            "(the asset load failed at Setup, or the loader-rooted asset was lost)"), InHandle)
        {}

        if (NOT SoundCueIsResolved)
        { return ECk_Request_OperationResult::Failed; }

        // The outer may legitimately die between enqueue and drain — a Failed completion, not an ensure.
        const auto Outer = InRequest.Get_Outer().Get();
        if (ck::Is_NOT_Valid(Outer))
        {
            ck::fx::Verbose(TEXT("Sfx [{}] skipping PlayAtLocation - the outer is no longer valid"), InHandle);
            return ECk_Request_OperationResult::Failed;
        }

        const auto& AudioSettings = InRequest.Get_OverrideAudioSettings()
                                        ? InRequest.Get_OverridenAudioSettings()
                                        : Params.Get_DefaultAudioSettings();

        const auto ResolvedAttenuation = Cast<USoundAttenuation>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_AttenuationSettings().ToSoftObjectPath()));
        const auto ResolvedConcurrency = Cast<USoundConcurrency>(
            InCurrent._LoadedAssets.Get_ResolvedObject(Params.Get_ConcurrencySettings().ToSoftObjectPath()));

        constexpr auto StartTime = 0.0f;
        UGameplayStatics::SpawnSoundAtLocation
        (
            Outer,
            ResolvedSoundCue,
            InRequest.Get_TransformSettings().Get_Location(),
            InRequest.Get_TransformSettings().Get_Rotation(),
            AudioSettings.Get_VolumeMultipler(),
            AudioSettings.Get_PitchMultipler(),
            StartTime,
            ResolvedAttenuation,
            ResolvedConcurrency
        );

        INC_DWORD_STAT(STAT_Fx_EffectsSpawned);

        return ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sfx_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sfx_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sfx_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sfx_Current& InCurrent)
            -> void
    {
        // Drops the streamable handle — the sfx's assets become collectible again
        InCurrent._LoadedAssets = {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
