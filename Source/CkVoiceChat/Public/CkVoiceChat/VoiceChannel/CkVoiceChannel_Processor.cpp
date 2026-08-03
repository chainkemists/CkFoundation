#include "CkVoiceChannel_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_AssignIdx);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceChannel_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Params& InParams)
        -> void
    {
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
        {}
        if (NOT IdxSpaceRemains)
        { return; }

        const auto NewIdx = static_cast<uint8>(Registry._NextIdx++);
        InCurrent._ChannelIdx = NewIdx;

        Registry._Entries.Emplace(FCk_VoiceChat_ChannelRegistryEntry{
            InParams.Get_ChannelName(), InVoiceChannelEntity, NewIdx});

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
        {}
        if (NOT TalkerIsValid)
        { return false; }

        const auto WasAlreadyMember = InCurrent._Members.Contains(Talker);
        InCurrent._Members.Add(Talker, InRequest.Get_MemberFlags());

        if (NOT WasAlreadyMember)
        {
            UUtils_Signal_OnVoiceChannel_MemberJoined::Broadcast(InHandle, MakePayload(InHandle, Talker));
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
        {}
        if (NOT IsMember)
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
        {}
        if (NOT TalkerIsValid)
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
