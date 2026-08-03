#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
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
}

// --------------------------------------------------------------------------------------------------------------------
