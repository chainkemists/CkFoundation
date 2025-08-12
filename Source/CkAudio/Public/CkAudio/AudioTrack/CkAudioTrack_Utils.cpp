#include "CkAudioTrack_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioTrack_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioTrack_UE::
    Create(
        FCk_Handle& InParentDirector,
        const FCk_Fragment_AudioTrack_ParamsData& InParams)
        -> FCk_Handle_AudioTrack
{
    ck::audio::VeryVerbose(TEXT("Creating AudioTrack [{}] for Director [{}]"), InParams.Get_TrackName(), InParentDirector);

    auto AudioTrack = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_AudioTrack>(InParentDirector);
    if (InParams.Get_TrackName().IsValid())
    {
        UCk_Utils_GameplayLabel_UE::Add(AudioTrack, InParams.Get_TrackName());
    }

    AudioTrack.Add<ck::FFragment_AudioTrack_Params>(InParams);
    AudioTrack.Add<ck::FFragment_AudioTrack_Current>();
    AudioTrack.Add<ck::FTag_AudioTrack_NeedsSetup>();

    UCk_Utils_Handle_UE::Set_DebugName(AudioTrack, *ck::Format_UE(TEXT("AudioTrack: {}"), InParams.Get_TrackName()));

    // Connect to parent director's RecordOfAudioTracks
    using RecordOfAudioTracks_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAudioTracks>;
    RecordOfAudioTracks_Utils::AddIfMissing(InParentDirector);
    RecordOfAudioTracks_Utils::Request_Connect(InParentDirector, AudioTrack);

    return AudioTrack;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AudioTrack_UE, FCk_Handle_AudioTrack,
    ck::FFragment_AudioTrack_Params, ck::FFragment_AudioTrack_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioTrack_UE::
    Get_TrackName(
        const FCk_Handle_AudioTrack& InTrack)
        -> FGameplayTag
{
    return InTrack.Get<ck::FFragment_AudioTrack_Params>().Get_TrackName();
}

auto
    UCk_Utils_AudioTrack_UE::
    Get_Priority(
        const FCk_Handle_AudioTrack& InTrack)
        -> int32
{
    return InTrack.Get<ck::FFragment_AudioTrack_Params>().Get_Priority();
}

auto
    UCk_Utils_AudioTrack_UE::
    Get_State(
        const FCk_Handle_AudioTrack& InTrack)
        -> ECk_AudioTrack_State
{
    return InTrack.Get<ck::FFragment_AudioTrack_Current>().Get_State();
}

auto
    UCk_Utils_AudioTrack_UE::
    Get_CurrentVolume(
        const FCk_Handle_AudioTrack& InTrack)
        -> float
{
    return InTrack.Get<ck::FFragment_AudioTrack_Current>().Get_CurrentVolume();
}

auto
    UCk_Utils_AudioTrack_UE::
    Get_OverrideBehavior(
        const FCk_Handle_AudioTrack& InTrack)
        -> ECk_AudioTrack_OverrideBehavior
{
    return InTrack.Get<ck::FFragment_AudioTrack_Params>().Get_OverrideBehavior();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioTrack_UE::
    Request_Play(
        FCk_Handle_AudioTrack& InTrack,
        FCk_Time InFadeInTime)
        -> FCk_Handle_AudioTrack
{
    ck::audio::VeryVerbose(TEXT("Requesting play for AudioTrack [{}] with fade time [{}s]"),
        Get_TrackName(InTrack), InFadeInTime.Get_Seconds());

    InTrack.AddOrGet<ck::FFragment_AudioTrack_Requests>()._Requests.Emplace(
        FCk_Request_AudioTrack_Play{InFadeInTime});

    return InTrack;
}

auto
    UCk_Utils_AudioTrack_UE::
    Request_Stop(
        FCk_Handle_AudioTrack& InTrack,
        FCk_Time InFadeOutTime)
        -> FCk_Handle_AudioTrack
{
    ck::audio::VeryVerbose(TEXT("Requesting stop for AudioTrack [{}] with fade time [{}s]"),
        Get_TrackName(InTrack), InFadeOutTime.Get_Seconds());

    InTrack.AddOrGet<ck::FFragment_AudioTrack_Requests>()._Requests.Emplace(
        FCk_Request_AudioTrack_Stop{InFadeOutTime});

    return InTrack;
}

auto
    UCk_Utils_AudioTrack_UE::
    Request_SetVolume(
        FCk_Handle_AudioTrack& InTrack,
        float InTargetVolume,
        FCk_Time InFadeTime)
        -> FCk_Handle_AudioTrack
{
    ck::audio::VeryVerbose(TEXT("Requesting volume change for AudioTrack [{}] to [{}] over [{}s]"),
        Get_TrackName(InTrack), InTargetVolume, InFadeTime.Get_Seconds());

    InTrack.AddOrGet<ck::FFragment_AudioTrack_Requests>()._Requests.Emplace(
        FCk_Request_AudioTrack_SetVolume{InTargetVolume, InFadeTime});

    return InTrack;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioTrack_UE::
    BindTo_OnPlaybackStarted(
        FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_Event& InDelegate)
        -> FCk_Handle_AudioTrack
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioTrack_PlaybackStarted, InTrack, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTrack;
}

auto
    UCk_Utils_AudioTrack_UE::
    BindTo_OnPlaybackFinished(
        FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_Event& InDelegate)
        -> FCk_Handle_AudioTrack
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioTrack_PlaybackFinished, InTrack, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTrack;
}

auto
    UCk_Utils_AudioTrack_UE::
    BindTo_OnFadeCompleted(
        FCk_Handle_AudioTrack& InTrack,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioTrack_Fade& InDelegate)
        -> FCk_Handle_AudioTrack
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioTrack_FadeCompleted, InTrack, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTrack;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioTrack_UE::
    UnbindFrom_OnPlaybackStarted(
        FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_Event& InDelegate)
        -> FCk_Handle_AudioTrack
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioTrack_PlaybackStarted, InTrack, InDelegate);
    return InTrack;
}

auto
    UCk_Utils_AudioTrack_UE::
    UnbindFrom_OnPlaybackFinished(
        FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_Event& InDelegate)
        -> FCk_Handle_AudioTrack
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioTrack_PlaybackFinished, InTrack, InDelegate);
    return InTrack;
}

auto
    UCk_Utils_AudioTrack_UE::
    UnbindFrom_OnFadeCompleted(
        FCk_Handle_AudioTrack& InTrack,
        const FCk_Delegate_AudioTrack_Fade& InDelegate)
        -> FCk_Handle_AudioTrack
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioTrack_FadeCompleted, InTrack, InDelegate);
    return InTrack;
}

// --------------------------------------------------------------------------------------------------------------------
