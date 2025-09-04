#include "CkAudioCue_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioCue_Fragment.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioCue_UE::
    Add(
        FCk_Handle& InHandle,
        const UCk_AudioCue_EntityScript& InAudioCueScript)
    -> FCk_Handle_AudioCue
{
    ck::audio::VeryVerbose(TEXT("Adding AudioCue feature to Entity [{}]"), InHandle);

    CK_ENSURE_IF_NOT(InAudioCueScript.Get_IsConfigurationValid(),
        TEXT("AudioCue configuration is invalid for Entity [{}]"), InHandle)
    { return {}; }

    // AudioCue IS an AudioDirector - add AudioDirector fragments directly
    const auto AudioDirectorParams = FCk_Fragment_AudioDirector_ParamsData{}
        .Set_DefaultCrossfadeDuration(InAudioCueScript.Get_DefaultCrossfadeDuration())
        .Set_MaxConcurrentTracks(InAudioCueScript.Get_MaxConcurrentTracks())
        .Set_SamePriorityBehavior(InAudioCueScript.Get_SamePriorityBehavior());

    UCk_Utils_AudioDirector_UE::Add(InHandle, AudioDirectorParams);

    InHandle.Add<ck::FFragment_AudioCue_Current>();
    InHandle.Add<ck::FTag_AudioCue_NeedsSetup>();

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *ck::Format_UE(TEXT("AudioCue: {}"), InAudioCueScript.Get_CueName()));

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AudioCue_UE, FCk_Handle_AudioCue,
    ck::FFragment_AudioCue_Current, ck::FFragment_AudioDirector_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioCue_UE::
    Request_Play(
        FCk_Handle_AudioCue& InAudioCue,
        const FCk_Request_AudioCue_Play& InRequest)
    -> FCk_Handle_AudioCue
{
    ck::audio::Verbose(TEXT("Requesting play for AudioCue [{}]"), InAudioCue);

    InAudioCue.AddOrGet<ck::FFragment_AudioCue_Requests>()._Requests.Emplace(InRequest);

    return InAudioCue;
}

auto
    UCk_Utils_AudioCue_UE::
    Request_Stop(
        FCk_Handle_AudioCue& InAudioCue,
        FCk_Time InFadeOutTime)
    -> FCk_Handle_AudioCue
{
    ck::audio::Verbose(TEXT("Requesting stop for AudioCue [{}]"), InAudioCue);

    InAudioCue.AddOrGet<ck::FFragment_AudioCue_Requests>()._Requests.Emplace(
        FCk_Request_AudioCue_Stop{InFadeOutTime});

    return InAudioCue;
}

auto
    UCk_Utils_AudioCue_UE::
    Request_StopAll(
        FCk_Handle_AudioCue& InAudioCue,
        FCk_Time InFadeOutTime)
    -> FCk_Handle_AudioCue
{
    ck::audio::Verbose(TEXT("Requesting stop all for AudioCue [{}]"), InAudioCue);

    InAudioCue.AddOrGet<ck::FFragment_AudioCue_Requests>()._Requests.Emplace(
        FCk_Request_AudioCue_StopAll{InFadeOutTime});

    return InAudioCue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioCue_UE::
    BindTo_OnTrackStarted(
        FCk_Handle_AudioCue& InAudioCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioCue_Event& InDelegate)
    -> FCk_Handle_AudioCue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioCue_TrackStarted, InAudioCue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAudioCue;
}

auto
    UCk_Utils_AudioCue_UE::
    BindTo_OnTrackStopped(
        FCk_Handle_AudioCue& InAudioCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioCue_Event& InDelegate)
    -> FCk_Handle_AudioCue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioCue_TrackStopped, InAudioCue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAudioCue;
}

auto
    UCk_Utils_AudioCue_UE::
    BindTo_OnAllTracksFinished(
        FCk_Handle_AudioCue& InAudioCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioCue_AllTracksFinished& InDelegate)
    -> FCk_Handle_AudioCue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioCue_AllTracksFinished, InAudioCue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAudioCue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioCue_UE::
    UnbindFrom_OnTrackStarted(
        FCk_Handle_AudioCue& InAudioCue,
        const FCk_Delegate_AudioCue_Event& InDelegate)
    -> FCk_Handle_AudioCue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioCue_TrackStarted, InAudioCue, InDelegate);
    return InAudioCue;
}

auto
    UCk_Utils_AudioCue_UE::
    UnbindFrom_OnTrackStopped(
        FCk_Handle_AudioCue& InAudioCue,
        const FCk_Delegate_AudioCue_Event& InDelegate)
    -> FCk_Handle_AudioCue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioCue_TrackStopped, InAudioCue, InDelegate);
    return InAudioCue;
}

auto
    UCk_Utils_AudioCue_UE::
    UnbindFrom_OnAllTracksFinished(
        FCk_Handle_AudioCue& InAudioCue,
        const FCk_Delegate_AudioCue_AllTracksFinished& InDelegate)
    -> FCk_Handle_AudioCue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioCue_AllTracksFinished, InAudioCue, InDelegate);
    return InAudioCue;
}

// --------------------------------------------------------------------------------------------------------------------
