#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVOICECHAT_API FProcessor_VoiceChannel_Setup : public ck_exp::TProcessor<
        FProcessor_VoiceChannel_Setup,
        FCk_Handle_VoiceChannel,
        ck::TReadOnly<FFragment_VoiceChannel_Params>,
        FTag_VoiceChannel_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_VoiceChannel_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Params& InParams)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Allocates the channel's wire index from the world registry - authority only. Indices are
    // session-append-only (never reused) so a stale wire ChannelIdx resolves to nothing rather
    // than to a different channel. Clients receive their index via the control-plane replication;
    // the NeedsIdx tag is inert on them (this processor is net-gated off).
    class CKVOICECHAT_API FProcessor_VoiceChannel_AssignIdx : public ck_exp::TProcessor<
        FProcessor_VoiceChannel_AssignIdx,
        FCk_Handle_VoiceChannel,
        ck::TReadOnly<FFragment_VoiceChannel_Params>,
        ck::TReadWrite<FFragment_VoiceChannel_Current>,
        FTag_VoiceChannel_NeedsIdx,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceChannel_Setup>;
        using MarkedDirtyBy = FTag_VoiceChannel_NeedsIdx;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Params& InParams,
            FFragment_VoiceChannel_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVOICECHAT_API FProcessor_VoiceChannel_HandleRequests : public ck_exp::TProcessor<
        FProcessor_VoiceChannel_HandleRequests,
        FCk_Handle_VoiceChannel,
        ck::TReadWrite<FFragment_VoiceChannel_Current>,
        ck::TReadWrite<FFragment_VoiceChannel_Requests>,
        TExclude<FTag_VoiceChannel_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceChannel_Setup>;
        using MarkedDirtyBy = FFragment_VoiceChannel_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            FFragment_VoiceChannel_Current& InCurrent,
            FFragment_VoiceChannel_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_Join& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_Leave& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_SetMemberFlags& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_ServerMute& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceChannel_Current& InCurrent,
            const FCk_Request_VoiceChannel_ServerUnmute& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed channel's
    // still-queued requests are never drained; this fires each pending request's completion
    // delegate with Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKVOICECHAT_API FProcessor_VoiceChannel_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_VoiceChannel_CancelPendingRequests,
        FCk_Handle_VoiceChannel,
        ck::TReadOnly<FFragment_VoiceChannel_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceChannelEntity,
            const FFragment_VoiceChannel_Requests& InRequests)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
