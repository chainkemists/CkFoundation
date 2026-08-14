#include "CkVoiceChannel_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/Settings/CkVoiceChat_Settings.h"
#include "CkVoiceChat/Net/CkVoiceChat_RepData.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"

#include <Sound/SoundAttenuation.h>
#include <Sound/SoundEffectSource.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_AssignIdx);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_channel_processor
{
    // Publishes this channel's control-plane entry onto the HOST entity's replicated container.
    // Host-gated inside the Try* calls, so mutation sites call it unconditionally; a
    // non-replicating host (local-only usage, unit tests) is a clean no-op.
    auto
    Push_ControlPlane(
        FCk_Handle_VoiceChannel InChannel,
        const ck::FFragment_VoiceChannel_Current& InCurrent) -> void
    {
        auto HostEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InChannel);

        if (ck::Is_NOT_Valid(HostEntity))
        { return; }

        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_VoiceChat>(HostEntity);
        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_VoiceChat>(HostEntity,
            [&](FCk_RepData_VoiceChat& InRepData) -> void
            {
                const auto& ChannelName = InChannel.Get<ck::FFragment_VoiceChannel_Params>().Get_ChannelName();

                auto& Entry = InRepData.FindOrAdd_Channel(ChannelName);
                Entry.Set_ChannelIdx(InCurrent.Get_ChannelIdx());

                auto Members = TArray<FCk_RepData_VoiceChat_Member>{};
                Members.Reserve(InCurrent.Get_Members().Num());
                for (const auto& [MemberHandle, MemberFlags] : InCurrent.Get_Members())
                {
                    Members.Emplace(FCk_RepData_VoiceChat_Member{MemberHandle, MemberFlags});
                }
                Entry.Set_Members(MoveTemp(Members));

                auto Muted = TArray<FCk_Handle>{};
                Muted.Reserve(InCurrent.Get_ServerMuted().Num());
                for (const auto& MutedHandle : InCurrent.Get_ServerMuted())
                {
                    Muted.Emplace(MutedHandle);
                }
                Entry.Set_ServerMuted(MoveTemp(Muted));
            });
    }

    // A membership change on a Positional3D channel re-shapes the talker's routing probe -
    // flag it for the (authority-only) proximity reconcile processor. Runs only from the
    // authority-side request handlers, so clients never grow probe state.
    auto
    MarkTalker_ProximityDirty(
        FCk_Handle_VoiceChannel InChannel,
        const FCk_Handle& InTalker) -> void
    {
        if (InChannel.Get<ck::FFragment_VoiceChannel_Params>().Get_SpatializationPolicy() != ECk_VoiceChat_SpatializationPolicy::Positional3D)
        { return; }

        if (ck::Is_NOT_Valid(InTalker))
        { return; }

        auto TalkerEntity = InTalker;
        TalkerEntity.AddOrGet<ck::FTag_VoiceTalker_ProximityDirty>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceChannel_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Params& InParams,
            FFragment_VoiceChannel_Current& InCurrent)
        -> void
    {
        // A spatializing channel that authors no attenuation falls back to the module default -
        // resolved through the same batch so playback never loads on demand.
        const auto Attenuation = [&]() -> TSoftObjectPtr<USoundAttenuation>
        {
            if (ck::IsValid(InParams.Get_Attenuation()))
            { return InParams.Get_Attenuation(); }

            if (InParams.Get_SpatializationPolicy() == ECk_VoiceChat_SpatializationPolicy::Global2D)
            { return {}; }

            return UCk_Utils_VoiceChat_Settings_UE::Get_DefaultAttenuation();
        }();
        const auto& SourceEffectChain = InParams.Get_SourceEffectChain();

        // Both assets are optional-by-design (ADR-5); a channel that resolves neither has nothing
        // to load. Resolution runs on EVERY machine - clients apply the config at playback.
        if (ck::IsValid(Attenuation) || ck::IsValid(SourceEffectChain))
        {
            if (NOT InCurrent._LoadedAudioAssets.Get_IsRequested())
            {
                auto PathsToLoad = TArray<FSoftObjectPath>{};

                if (ck::IsValid(Attenuation))
                { PathsToLoad.Emplace(Attenuation.ToSoftObjectPath()); }
                if (ck::IsValid(SourceEffectChain))
                { PathsToLoad.Emplace(SourceEffectChain.ToSoftObjectPath()); }

                InCurrent._LoadedAudioAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                    TEXT("VoiceChannel.Setup"), PathsToLoad);
            }

            if (NOT InCurrent._LoadedAudioAssets.Get_IsReady())
            {
                InVoiceChannelEntity.AddOrGet<FTag_VoiceChannel_PendingAssetLoad>();
                return;
            }

            const auto ResolvedAttenuation = ck::IsValid(Attenuation)
                ? Cast<USoundAttenuation>(InCurrent._LoadedAudioAssets.Get_ResolvedObject(Attenuation.ToSoftObjectPath()))
                : nullptr;
            const auto ResolvedSourceEffectChain = ck::IsValid(SourceEffectChain)
                ? Cast<USoundEffectSourcePresetChain>(InCurrent._LoadedAudioAssets.Get_ResolvedObject(SourceEffectChain.ToSoftObjectPath()))
                : nullptr;

            const auto EveryAuthoredAssetResolved =
                NOT InCurrent._LoadedAudioAssets.Get_HasFailed() &&
                (ck::Is_NOT_Valid(Attenuation) || ck::IsValid(ResolvedAttenuation)) &&
                (ck::Is_NOT_Valid(SourceEffectChain) || ck::IsValid(ResolvedSourceEffectChain));

            CK_ENSURE_IF_NOT(EveryAuthoredAssetResolved,
                TEXT("VoiceChannel [{}] failed to resolve its authored audio config through CkResourceLoader "
                     "(Attenuation [{}], SourceEffectChain [{}]) - playback on this machine falls back to module defaults"),
                InVoiceChannelEntity, Attenuation.ToSoftObjectPath(), SourceEffectChain.ToSoftObjectPath())
            {
                InCurrent._LoadedAudioAssets = {};
                InCurrent._ResolvedAttenuation = nullptr;
                InCurrent._ResolvedSourceEffectChain = nullptr;
                InVoiceChannelEntity.Try_Remove<FTag_VoiceChannel_PendingAssetLoad>();
                InVoiceChannelEntity.Remove<MarkedDirtyBy>();
                return;
            }

            InCurrent._ResolvedAttenuation = ResolvedAttenuation;
            InCurrent._ResolvedSourceEffectChain = ResolvedSourceEffectChain;
            InVoiceChannelEntity.Try_Remove<FTag_VoiceChannel_PendingAssetLoad>();
        }

        InVoiceChannelEntity.Remove<MarkedDirtyBy>();

        voice_chat::VeryVerbose(TEXT("Setup complete for VoiceChannel [{}] with ChannelName [{}]"),
            InVoiceChannelEntity, InParams.Get_ChannelName());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceChannel_AssignIdx::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Params& InParams,
            FFragment_VoiceChannel_Current& InCurrent)
        -> void
    {
        InVoiceChannelEntity.Remove<MarkedDirtyBy>();

        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InVoiceChannelEntity);
        auto& Registry = TransientEntity.AddOrGet<FFragment_VoiceChat_ChannelRegistry>();

        const auto IdxSpaceRemains = Registry._NextIdx < VoiceChannel_UnassignedIdx;
        CK_ENSURE_IF_NOT(IdxSpaceRemains,
            TEXT("VoiceChannel index space exhausted ([{}] channels this session) - channel [{}] ([{}]) stays "
                 "unroutable. Indices are session-append-only by design: a stale wire ChannelIdx must resolve "
                 "to nothing, never to a different channel."),
            VoiceChannel_UnassignedIdx, InVoiceChannelEntity, InParams.Get_ChannelName())
        { return; }

        const auto NewIdx = static_cast<uint8>(Registry._NextIdx++);
        InCurrent._ChannelIdx = NewIdx;

        Registry._Entries.Emplace(FCk_VoiceChat_ChannelRegistryEntry{
            InParams.Get_ChannelName(), InVoiceChannelEntity, NewIdx});

        ck_voice_channel_processor::Push_ControlPlane(InVoiceChannelEntity, InCurrent);

        voice_chat::VeryVerbose(TEXT("Assigned ChannelIdx [{}] to VoiceChannel [{}] ([{}])"),
            NewIdx, InVoiceChannelEntity, InParams.Get_ChannelName());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceChannel_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            FFragment_VoiceChannel_Current& InCurrent,
            FFragment_VoiceChannel_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InVoiceChannelEntity, Result);

            if (DoHandleRequest(InVoiceChannelEntity, InCurrent, InRequest))
            {
                Result = ECk_Request_OperationResult::Succeeded;
                ck_voice_channel_processor::Push_ControlPlane(InVoiceChannelEntity, InCurrent);
            }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InVoiceChannelEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_VoiceChannel_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_Join& InRequest)
        -> bool
    {
        const auto& Talker = InRequest.Get_Talker();

        const auto TalkerIsValid = ck::IsValid(Talker);
        CK_ENSURE_IF_NOT(TalkerIsValid,
            TEXT("Join on VoiceChannel [{}] with an invalid Talker handle"), InHandle)
        { return false; }

        const auto WasAlreadyMember = InCurrent._Members.Contains(Talker);
        InCurrent._Members.Add(Talker, InRequest.Get_MemberFlags());

        if (NOT WasAlreadyMember)
        {
            UUtils_Signal_OnVoiceChannel_MemberJoined::Broadcast(InHandle, MakePayload(InHandle, Talker));
            ck_voice_channel_processor::MarkTalker_ProximityDirty(InHandle, Talker);
        }

        return true;
    }

    auto
        FProcessor_VoiceChannel_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_Leave& InRequest)
        -> bool
    {
        const auto& Talker = InRequest.Get_Talker();

        if (InCurrent._Members.Remove(Talker) > 0)
        {
            UUtils_Signal_OnVoiceChannel_MemberLeft::Broadcast(InHandle, MakePayload(InHandle, Talker));
            ck_voice_channel_processor::MarkTalker_ProximityDirty(InHandle, Talker);
        }

        // The server-mute entry deliberately survives leave: a moderated talker who rejoins is
        // still muted until an explicit ServerUnmute.
        return true;
    }

    auto
        FProcessor_VoiceChannel_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_SetMemberFlags& InRequest)
        -> bool
    {
        const auto& Talker = InRequest.Get_Talker();

        const auto IsMember = InCurrent._Members.Contains(Talker);
        CK_ENSURE_IF_NOT(IsMember,
            TEXT("SetMemberFlags on VoiceChannel [{}] for Talker [{}] who is not a member"), InHandle, Talker)
        { return false; }

        InCurrent._Members.Add(Talker, InRequest.Get_MemberFlags());
        return true;
    }

    auto
        FProcessor_VoiceChannel_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_ServerMute& InRequest)
        -> bool
    {
        const auto& Talker = InRequest.Get_Talker();

        const auto TalkerIsValid = ck::IsValid(Talker);
        CK_ENSURE_IF_NOT(TalkerIsValid,
            TEXT("ServerMute on VoiceChannel [{}] with an invalid Talker handle"), InHandle)
        { return false; }

        InCurrent._ServerMuted.Add(Talker);
        return true;
    }

    auto
        FProcessor_VoiceChannel_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_ServerUnmute& InRequest)
        -> bool
    {
        InCurrent._ServerMuted.Remove(InRequest.Get_Talker());
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceChannel_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Current& InCurrent)
        -> void
    {
        for (const auto& [MemberHandle, MemberFlags] : InCurrent.Get_Members())
        {
            ck_voice_channel_processor::MarkTalker_ProximityDirty(InVoiceChannelEntity, MemberHandle);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceChannel_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Requests& InRequests)
        -> void
    {
        ck::request::FireCancelledForPending(InVoiceChannelEntity, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
