#include "CkVoiceTalker_Processor.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceTalker_Setup);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceTalker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent)
        -> void
    {
        InVoiceTalkerEntity.Remove<MarkedDirtyBy>();

        voice_chat::VeryVerbose(TEXT("Setup complete for VoiceTalker [{}]"), InVoiceTalkerEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
