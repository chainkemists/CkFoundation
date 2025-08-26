#include "CkAudioDirector_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioDirector_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioDirector_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AudioDirector_ParamsData& InParams)
        -> FCk_Handle_AudioDirector
{
    ck::audio::VeryVerbose(TEXT("Adding AudioDirector feature to Entity [{}]"), InHandle);

    InHandle.Add<ck::FFragment_AudioDirector_Params>(InParams);
    InHandle.Add<ck::FFragment_AudioDirector_Current>();
    InHandle.Add<ck::FTag_AudioDirector_NeedsSetup>();

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, TEXT("AudioDirector"));

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AudioDirector_UE, FCk_Handle_AudioDirector,
    ck::FFragment_AudioDirector_Params, ck::FFragment_AudioDirector_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioDirector_UE::
    Get_CurrentHighestPriority(
        const FCk_Handle_AudioDirector& InDirector)
        -> int32
{
    return InDirector.Get<ck::FFragment_AudioDirector_Current>().Get_CurrentHighestPriority();
}

auto
    UCk_Utils_AudioDirector_UE::
    Get_TrackByName(
        const FCk_Handle_AudioDirector& InDirector,
        FGameplayTag InTrackName)
        -> FCk_Handle_AudioTrack
{
    const auto& TracksByName = InDirector.Get<ck::FFragment_AudioDirector_Current>().Get_TracksByName();

    if (const auto* FoundTrack = TracksByName.Find(InTrackName))
    {
        return *FoundTrack;
    }

    return {};
}

auto
    UCk_Utils_AudioDirector_UE::
    Get_AllTracks(
        const FCk_Handle_AudioDirector& InDirector)
        -> TArray<FCk_Handle_AudioTrack>
{
    auto Result = TArray<FCk_Handle_AudioTrack>{};
    const auto& TracksByName = InDirector.Get<ck::FFragment_AudioDirector_Current>().Get_TracksByName();

    for (const auto& [TagName, TrackHandle] : TracksByName)
    {
        if (ck::IsValid(TrackHandle))
        {
            Result.Add(TrackHandle);
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioDirector_UE::
    Request_AddTrack(
        FCk_Handle_AudioDirector& InDirector,
        const FCk_Fragment_AudioTrack_ParamsData& InTrackParams)
        -> FCk_Handle_AudioDirector
{
    ck::audio::VeryVerbose(TEXT("Requesting to add track [{}] to AudioDirector [{}]"),
        InTrackParams.Get_TrackName(), InDirector);

    InDirector.AddOrGet<ck::FFragment_AudioDirector_Requests>()._Requests.Emplace(
        FCk_Request_AudioDirector_AddTrack{InTrackParams});

    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    Request_StartTrack(
        FCk_Handle_AudioDirector& InDirector,
        const FCk_Request_AudioDirector_StartTrack& InRequest)
        -> FCk_Handle_AudioDirector
{
    ck::audio::Verbose(TEXT("Requesting to start track [{}] on AudioDirector [{}]"),
        InRequest.Get_TrackName(), InDirector);

    // Convert request to internal request format
    auto InternalRequest = FCk_Request_AudioDirector_StartTrack{InRequest.Get_TrackName()};
    InternalRequest.Set_PriorityOverrideMode(InRequest.Get_PriorityOverrideMode());
    InternalRequest.Set_PriorityOverrideValue(InRequest.Get_PriorityOverrideValue());
    InternalRequest.Set_FadeInTime(InRequest.Get_FadeInTime());

    InDirector.AddOrGet<ck::FFragment_AudioDirector_Requests>()._Requests.Emplace(InternalRequest);

    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    Request_StopTrack(
        FCk_Handle_AudioDirector& InDirector,
        FGameplayTag InTrackName,
        FCk_Time InFadeOutTime)
        -> FCk_Handle_AudioDirector
{
    ck::audio::Verbose(TEXT("Requesting to stop track [{}] on AudioDirector [{}]"),
        InTrackName, InDirector);

    InDirector.AddOrGet<ck::FFragment_AudioDirector_Requests>()._Requests.Emplace(
        FCk_Request_AudioDirector_StopTrack{InTrackName}.Set_FadeOutTime(InFadeOutTime));

    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    Request_StopAllTracks(
        FCk_Handle_AudioDirector& InDirector,
        FCk_Time InFadeOutTime)
        -> FCk_Handle_AudioDirector
{
    ck::audio::Verbose(TEXT("Requesting to stop all tracks on AudioDirector [{}]"), InDirector);

    InDirector.AddOrGet<ck::FFragment_AudioDirector_Requests>()._Requests.Emplace(
        FCk_Request_AudioDirector_StopAllTracks{InFadeOutTime});

    return InDirector;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioDirector_UE::
    BindTo_OnTrackStarted(
        FCk_Handle_AudioDirector& InDirector,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioDirector_Track& InDelegate)
        -> FCk_Handle_AudioDirector
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioDirector_TrackStarted, InDirector, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    BindTo_OnTrackStopped(
        FCk_Handle_AudioDirector& InDirector,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioDirector_Track& InDelegate)
        -> FCk_Handle_AudioDirector
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioDirector_TrackStopped, InDirector, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    BindTo_OnTrackAdded(
        FCk_Handle_AudioDirector& InDirector,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AudioDirector_Track& InDelegate)
        -> FCk_Handle_AudioDirector
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAudioDirector_TrackAdded, InDirector, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InDirector;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioDirector_UE::
    UnbindFrom_OnTrackStarted(
        FCk_Handle_AudioDirector& InDirector,
        const FCk_Delegate_AudioDirector_Track& InDelegate)
        -> FCk_Handle_AudioDirector
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioDirector_TrackStarted, InDirector, InDelegate);
    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    UnbindFrom_OnTrackStopped(
        FCk_Handle_AudioDirector& InDirector,
        const FCk_Delegate_AudioDirector_Track& InDelegate)
        -> FCk_Handle_AudioDirector
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioDirector_TrackStopped, InDirector, InDelegate);
    return InDirector;
}

auto
    UCk_Utils_AudioDirector_UE::
    UnbindFrom_OnTrackAdded(
        FCk_Handle_AudioDirector& InDirector,
        const FCk_Delegate_AudioDirector_Track& InDelegate)
        -> FCk_Handle_AudioDirector
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAudioDirector_TrackAdded, InDirector, InDelegate);
    return InDirector;
}

// --------------------------------------------------------------------------------------------------------------------
