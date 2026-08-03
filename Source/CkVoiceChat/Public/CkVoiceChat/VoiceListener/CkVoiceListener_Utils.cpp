#include "CkVoiceListener_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

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

    auto Listener = Cast(InHandle);

    // This machine's ears (last added wins - one local listener per world is the contract); the
    // receive path reads it for the local defense-in-depth mute gate.
    auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle);
    TransientEntity.AddOrGet<ck::FFragment_VoiceChat_LocalListener>().Set_Listener(Listener);

    return Listener;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VoiceListener_UE, FCk_Handle_VoiceListener,
    ck::FFragment_VoiceListener_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceListener_UE::
    Request_MuteTalker(
        FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Request_VoiceListener_MuteTalker& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceListener
{
    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceListener.AddOrGet<ck::FFragment_VoiceListener_Requests>()._Requests.Emplace(Request);

    return InVoiceListener;
}

auto
    UCk_Utils_VoiceListener_UE::
    Request_UnmuteTalker(
        FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Request_VoiceListener_UnmuteTalker& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceListener
{
    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceListener.AddOrGet<ck::FFragment_VoiceListener_Requests>()._Requests.Emplace(Request);

    return InVoiceListener;
}

auto
    UCk_Utils_VoiceListener_UE::
    Request_SetTalkerVolume(
        FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Request_VoiceListener_SetTalkerVolume& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceListener
{
    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceListener.AddOrGet<ck::FFragment_VoiceListener_Requests>()._Requests.Emplace(Request);

    return InVoiceListener;
}

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
