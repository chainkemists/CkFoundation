#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceChannel_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoiceChannel_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoiceChannel_Params = FCk_Fragment_VoiceChannel_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // TRANSIENT: channels are runtime net state; nothing voice-related is ever saved.
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfVoiceChannels, FCk_Handle_VoiceChannel);
}

// --------------------------------------------------------------------------------------------------------------------
