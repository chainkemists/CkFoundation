#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Processor.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The Positional3D routing set (ADR-6): reconciles a talker's probe pair against its current
    // Positional3D memberships whenever those change. Any member gets a presence dot (so other
    // talkers' range probes can detect it) plus a range probe sized max member-channel range +
    // hysteresis margin whose Jolt contacts maintain the candidate set event-driven. A talker
    // without a Transform gets NO probes - no position means no proximity, and Positional3D
    // routing fails CLOSED for it (never falls back to membership-wide sends).
    class CKVOICECHAT_API FProcessor_VoiceChat_ProximityProbes : public ck_exp::TProcessor<
        FProcessor_VoiceChat_ProximityProbes,
        FCk_Handle_VoiceTalker,
        FTag_VoiceTalker_ProximityDirty,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceChannel_HandleRequests>;
        using MarkedDirtyBy = FTag_VoiceTalker_ProximityDirty;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The server's packet path (spec §7.3): drains each talker's routing inbox, re-validates
    // every bundle against the authorization state (sender binding, membership, CanTalk, server
    // mute), resolves the wire ChannelIdx against the registry - an unresolvable idx is DROPPED
    // and counted, NEVER stashed (review N1: voice is disposable; a stale idx must not resurrect
    // when the registry catches up) - then forwards the packed bytes untouched (the server never
    // decodes) to every eligible recipient under the per-connection byte budget.
    class CKVOICECHAT_API FProcessor_VoiceChat_Route : public ck_exp::TProcessor<
        FProcessor_VoiceChat_Route,
        FCk_Handle_VoiceTalker,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        ck::TReadWrite<FFragment_VoiceTalker_ServerInbox>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceTalker_Capture, FProcessor_VoiceChat_ProximityProbes>;
        using MarkedDirtyBy = FFragment_VoiceTalker_ServerInbox;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Current& InCurrent,
            FFragment_VoiceTalker_ServerInbox& InInbox)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The send half of routing: Route STAGES authorized forwards per talker; this processor sees
    // every talker competing for each recipient this tick and applies the audible-speaker cap
    // (envelope-bucket priority, least-recently-served rotation - the CTO amplitude-fairness
    // ruling), then the per-connection byte budget, then the actual Client RPC.
    class CKVOICECHAT_API FProcessor_VoiceChat_FlushForwards : public ck_exp::TProcessor<
        FProcessor_VoiceChat_FlushForwards,
        FCk_Handle,
        ck::TReadWrite<FFragment_VoiceChat_PendingForwards>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceChat_Route>;
        using MarkedDirtyBy = FFragment_VoiceChat_PendingForwards;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTransientEntity,
            FFragment_VoiceChat_PendingForwards& InPending)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
