#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceTalker_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_IsTransmitting);
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_IsSpeaking);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoiceTalker_Params = FCk_Fragment_VoiceTalker_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceTalker_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_Current);

    public:
        friend class FProcessor_VoiceTalker_Setup;
        friend class UCk_Utils_VoiceTalker_UE;

    private:
        uint16 _Seq = 0;
        uint8 _AmplitudeQ8 = 0;

    public:
        CK_PROPERTY_GET(_Seq);
        CK_PROPERTY_GET(_AmplitudeQ8);
    };
}

// --------------------------------------------------------------------------------------------------------------------
