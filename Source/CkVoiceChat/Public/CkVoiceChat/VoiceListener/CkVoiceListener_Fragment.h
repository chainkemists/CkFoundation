#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceListener_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Local-machine ears state: per-talker client mute and receive volume. The mute set is also
    // reported upstream as a routing exclusion, so muted audio is never sent at all.
    struct CKVOICECHAT_API FFragment_VoiceListener_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceListener_Current);

    public:
        friend class UCk_Utils_VoiceListener_UE;

    private:
        TSet<FCk_Handle> _MutedTalkers;
        TMap<FCk_Handle, float> _TalkerVolumes;

    public:
        CK_PROPERTY_GET(_MutedTalkers);
        CK_PROPERTY_GET(_TalkerVolumes);
    };
}

// --------------------------------------------------------------------------------------------------------------------
