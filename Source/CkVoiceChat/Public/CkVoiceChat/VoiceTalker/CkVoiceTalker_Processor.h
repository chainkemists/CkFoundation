#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVOICECHAT_API FProcessor_VoiceTalker_Setup : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_Setup,
        FCk_Handle_VoiceTalker,
        ck::TReadOnly<FFragment_VoiceTalker_Params>,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        FTag_VoiceTalker_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_VoiceTalker_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
