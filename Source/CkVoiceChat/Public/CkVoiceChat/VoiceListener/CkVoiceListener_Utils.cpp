#include "CkVoiceListener_Utils.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceListener_UE::
    Add(
        FCk_Handle& InHandle)
    -> FCk_Handle_VoiceListener
{
    ck::voice_chat::VeryVerbose(TEXT("Adding VoiceListener feature to Entity [{}]"), InHandle);

    InHandle.Add<ck::FFragment_VoiceListener_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VoiceListener_UE, FCk_Handle_VoiceListener,
    ck::FFragment_VoiceListener_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceListener_UE::
    Get_IsTalkerMuted(
        const FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> bool
{
    return InVoiceListener.Get<ck::FFragment_VoiceListener_Current>().Get_MutedTalkers().Contains(InVoiceTalker);
}

auto
    UCk_Utils_VoiceListener_UE::
    Get_TalkerVolume(
        const FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> float
{
    const auto& TalkerVolumes = InVoiceListener.Get<ck::FFragment_VoiceListener_Current>().Get_TalkerVolumes();

    if (const auto* FoundVolume = TalkerVolumes.Find(InVoiceTalker))
    { return *FoundVolume; }

    return 1.0f;
}

// --------------------------------------------------------------------------------------------------------------------
