#include "CkVoiceChat_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_SampleRate()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::SampleRate; }

    return Settings->Get_SampleRate();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_BitrateBps()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::BitrateBps; }

    return Settings->Get_BitrateBps();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_Vbr()
    -> ECk_EnableDisable
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_EnableDisable::Enable; }

    return Settings->Get_Vbr();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_FrameSizeMs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::FrameSizeMs; }

    return Settings->Get_FrameSizeMs();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_FramesPerRpc()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::FramesPerRpc; }

    return Settings->Get_FramesPerRpc();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_MaxVoiceBytesPerConnectionPerTick()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::MaxVoiceBytesPerConnectionPerTick; }

    return Settings->Get_MaxVoiceBytesPerConnectionPerTick();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_MaxAudibleSpeakers()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::MaxAudibleSpeakers; }

    return Settings->Get_MaxAudibleSpeakers();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_ProximityHysteresisMarginCm()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck::voice_chat::defaults::ProximityHysteresisMarginCm; }

    return Settings->Get_ProximityHysteresisMarginCm();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_JitterBufferMin()
    -> FCk_Time
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return FCk_Time{ck::voice_chat::defaults::JitterBufferMinSeconds}; }

    return Settings->Get_JitterBufferMin();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_JitterBufferTarget()
    -> FCk_Time
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return FCk_Time{ck::voice_chat::defaults::JitterBufferTargetSeconds}; }

    return Settings->Get_JitterBufferTarget();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_JitterBufferMax()
    -> FCk_Time
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return FCk_Time{ck::voice_chat::defaults::JitterBufferMaxSeconds}; }

    return Settings->Get_JitterBufferMax();
}

auto
    UCk_Utils_VoiceChat_Settings_UE::
    Get_DefaultAttenuation()
    -> TSoftObjectPtr<USoundAttenuation>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoiceChat_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_DefaultAttenuation();
}

// --------------------------------------------------------------------------------------------------------------------
