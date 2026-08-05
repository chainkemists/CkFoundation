#include "CkVoiceTalker_Utils.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VoiceTalker_ParamsData& InParams)
    -> FCk_Handle_VoiceTalker
{
    ck::voice_chat::VeryVerbose(TEXT("Adding VoiceTalker feature to Entity [{}]"), InHandle);

    InHandle.Add<ck::FFragment_VoiceTalker_Params>(InParams);
    InHandle.Add<ck::FFragment_VoiceTalker_Current>();
    InHandle.Add<ck::FTag_VoiceTalker_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VoiceTalker_UE, FCk_Handle_VoiceTalker,
    ck::FFragment_VoiceTalker_Params, ck::FFragment_VoiceTalker_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceTalker_UE::
    Get_TransmitMode(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> ECk_VoiceChat_TransmitMode
{
    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Params>().Get_TransmitMode();
}

auto
    UCk_Utils_VoiceTalker_UE::
    Get_InputGain(
        const FCk_Handle_VoiceTalker& InVoiceTalker)
    -> float
{
    return InVoiceTalker.Get<ck::FFragment_VoiceTalker_Params>().Get_InputGain();
}

// --------------------------------------------------------------------------------------------------------------------
