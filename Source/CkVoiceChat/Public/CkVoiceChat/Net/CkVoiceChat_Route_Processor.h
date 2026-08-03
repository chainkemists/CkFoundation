#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The server's packet path (spec §7.3): drains each talker's routing inbox, re-validates
    // every bundle against the authorization state (sender binding, membership, CanTalk, server
    // mute), resolves the wire ChannelIdx against the registry - an unresolvable idx is DROPPED
    // and counted, NEVER stashed (review N1: voice is disposable; a stale idx must not resurrect
    // when the registry catches up) - then forwards the packed bytes untouched (the server never
    // decodes) to every eligible recipient under the per-connection byte budget.
    class CKVOICECHAT_API FProcessor_VoiceChat_Route : public ck_exp::TProcessor<
        FProcessor_VoiceChat_Route,
        FCk_Handle_VoiceTalker,
        ck::TReadWrite<FFragment_VoiceTalker_ServerInbox>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceTalker_Capture>;
        using MarkedDirtyBy = FFragment_VoiceTalker_ServerInbox;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_ServerInbox& InInbox)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
