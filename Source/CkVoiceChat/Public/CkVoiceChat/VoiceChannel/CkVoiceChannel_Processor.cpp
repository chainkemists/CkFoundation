#include "CkVoiceChannel_Processor.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChannel_Setup);

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
}

// --------------------------------------------------------------------------------------------------------------------
