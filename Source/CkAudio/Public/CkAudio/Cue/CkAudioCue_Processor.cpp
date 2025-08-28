#include "CkAudioCue_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioCue_Utils.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Utils.h"
#include "CkAudio/AudioTrack/CkAudioTrack_Fragment.h"
#include "CkAudio/AudioTrack/CkAudioTrack_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AudioCue_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript)
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::audio::Verbose(TEXT("Setting up AudioCue [{}]"), InHandle);

        // TODO: Why? This should be reset already, we're in Setup
        // Reset state
        InCurrent._RecentTracks.Empty();
        InCurrent._LastSelectedIndex = INDEX_NONE;
        InCurrent._ActiveTracks.Empty();
        InCurrent._HasFiredAllTracksFinished = false;

        const auto AudioCueScript = Cast<UCk_AudioCue_EntityScript>(InEntityScript.Get_Script().Get());
        CK_ENSURE_IF_NOT(ck::IsValid(AudioCueScript),
            TEXT("AudioCue [{}] does not have valid AudioCue EntityScript"), InHandle)
        { return; }

        // Pre-populate tracks for persistent cues
        const auto LifetimeBehavior = AudioCueScript->Get_LifetimeBehavior();
        if (LifetimeBehavior == ECk_Cue_LifetimeBehavior::Persistent ||
            LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed)
        {
            // AudioCue IS an AudioDirector, so we can use AudioDirector utils directly
            auto AudioDirector = UCk_Utils_AudioDirector_UE::Cast(InHandle);

            if (AudioCueScript->Get_HasValidTrackLibrary())
            {
                for (const auto& TrackParams : AudioCueScript->Get_TrackLibrary())
                {
                    UCk_Utils_AudioDirector_UE::Request_AddTrack(AudioDirector, TrackParams);
                }
            }
            if (AudioCueScript->Get_HasValidSingleTrack())
            {
                UCk_Utils_AudioDirector_UE::Request_AddTrack(AudioDirector, AudioCueScript->Get_SingleTrack());
            }
        }

        ck::audio::VeryVerbose(TEXT("AudioCue [{}] setup complete"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioCue_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FFragment_AudioCue_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_AudioCue_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InEntityScript, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_AudioCue_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_AudioCue_Play& InRequest)
            -> void
    {
        const auto AudioCueScript = Cast<UCk_AudioCue_EntityScript>(InEntityScript.Get_Script().Get());
        CK_ENSURE_IF_NOT(ck::IsValid(AudioCueScript),
            TEXT("AudioCue [{}] does not have valid AudioCue EntityScript"), InHandle)
        { return; }

        ck::audio::Verbose(TEXT("Handling play request for AudioCue [{}]"), InHandle);

        DoSelectAndPlayTrack(InHandle, InCurrent, AudioCueScript, InRequest);
    }

    auto
        FProcessor_AudioCue_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_AudioCue_Stop& InRequest)
            -> void
    {
        ck::audio::Verbose(TEXT("Handling stop request for AudioCue [{}]"), InHandle);

        // AudioCue IS an AudioDirector, so we can use AudioDirector utils directly
        auto AudioDirector = UCk_Utils_AudioDirector_UE::Cast(InHandle);
        UCk_Utils_AudioDirector_UE::Request_StopAllTracks(AudioDirector, InRequest.Get_FadeOutTime());
    }

    auto
        FProcessor_AudioCue_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_AudioCue_StopAll& InRequest)
            -> void
    {
        ck::audio::Verbose(TEXT("Handling stop all request for AudioCue [{}]"), InHandle);

        // AudioCue IS an AudioDirector, so we can use AudioDirector utils directly
        auto AudioDirector = UCk_Utils_AudioDirector_UE::Cast(InHandle);
        UCk_Utils_AudioDirector_UE::Request_StopAllTracks(AudioDirector, InRequest.Get_FadeOutTime());
    }

    auto
        FProcessor_AudioCue_HandleRequests::
        DoSelectAndPlayTrack(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const UCk_AudioCue_EntityScript* InAudioCueScript,
            const FCk_Request_AudioCue_Play& InRequest)
            -> void
    {
        FCk_Fragment_AudioTrack_ParamsData SelectedTrack;
        bool TrackSelected = false;

        // Select track based on source priority
        switch (InAudioCueScript->Get_SourcePriority())
        {
            case ECk_AudioCue_SourcePriority::PreferSingleTrack:
            {
                if (InAudioCueScript->Get_HasValidSingleTrack())
                {
                    SelectedTrack = InAudioCueScript->Get_SingleTrack();
                    TrackSelected = true;
                }
                else if (InAudioCueScript->Get_HasValidTrackLibrary())
                {
                    const auto TrackIndex = InAudioCueScript->Get_NextTrackIndex(InCurrent._RecentTracks);
                    if (TrackIndex != INDEX_NONE)
                    {
                        SelectedTrack = InAudioCueScript->Get_TrackLibrary()[TrackIndex];
                        TrackSelected = true;
                        InCurrent._LastSelectedIndex = TrackIndex;
                    }
                }
                break;
            }
            case ECk_AudioCue_SourcePriority::PreferLibrary:
            {
                if (InAudioCueScript->Get_HasValidTrackLibrary())
                {
                    const auto TrackIndex = InAudioCueScript->Get_NextTrackIndex(InCurrent._RecentTracks);
                    if (TrackIndex != INDEX_NONE)
                    {
                        SelectedTrack = InAudioCueScript->Get_TrackLibrary()[TrackIndex];
                        TrackSelected = true;
                        InCurrent._LastSelectedIndex = TrackIndex;
                    }
                }
                else if (InAudioCueScript->Get_HasValidSingleTrack())
                {
                    SelectedTrack = InAudioCueScript->Get_SingleTrack();
                    TrackSelected = true;
                }
                break;
            }
            case ECk_AudioCue_SourcePriority::SingleTrackOnly:
            {
                if (InAudioCueScript->Get_HasValidSingleTrack())
                {
                    SelectedTrack = InAudioCueScript->Get_SingleTrack();
                    TrackSelected = true;
                }
                break;
            }
            case ECk_AudioCue_SourcePriority::LibraryOnly:
            {
                if (InAudioCueScript->Get_HasValidTrackLibrary())
                {
                    const auto TrackIndex = InAudioCueScript->Get_NextTrackIndex(InCurrent._RecentTracks);
                    if (TrackIndex != INDEX_NONE)
                    {
                        SelectedTrack = InAudioCueScript->Get_TrackLibrary()[TrackIndex];
                        TrackSelected = true;
                        InCurrent._LastSelectedIndex = TrackIndex;
                    }
                }
                break;
            }
        }

        CK_ENSURE_IF_NOT(TrackSelected,
            TEXT("AudioCue [{}] could not select a valid track"), InHandle)
        { return; }

        auto StartTrackRequest = FCk_Request_AudioDirector_StartTrack{SelectedTrack.Get_TrackName()};
        StartTrackRequest.Set_PriorityOverrideMode(InRequest.Get_PriorityOverrideMode());
        StartTrackRequest.Set_PriorityOverrideValue(InRequest.Get_PriorityOverrideValue());
        StartTrackRequest.Set_FadeInTime(InRequest.Get_FadeInTime());

        // AudioCue IS an AudioDirector, so we can use AudioDirector utils directly
        auto AudioDirector = UCk_Utils_AudioDirector_UE::Cast(InHandle);
        UCk_Utils_AudioDirector_UE::Request_AddTrack(AudioDirector, SelectedTrack);
        UCk_Utils_AudioDirector_UE::Request_StartTrack(AudioDirector, StartTrackRequest);

        // Update recent tracks for avoidance
        InCurrent._RecentTracks.Add(SelectedTrack.Get_TrackName());
        if (InCurrent._RecentTracks.Num() > 10) // Keep last 10 tracks
        {
            InCurrent._RecentTracks.RemoveAt(0);
        }

        UUtils_Signal_OnAudioCue_TrackStarted::Broadcast(InHandle,
            MakePayload(InHandle, SelectedTrack.Get_TrackName()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioCue_TrackStateMonitor::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            FFragment_AudioCue_Current& InAudioCueCurrent,
            const FFragment_AudioDirector_Current& InDirectorCurrent)
            -> void
    {
        DoCheckAllTracksFinished(InHandle, InAudioCueCurrent, InDirectorCurrent);
    }

    auto
        FProcessor_AudioCue_TrackStateMonitor::
        DoCheckAllTracksFinished(
            HandleType InHandle,
            FFragment_AudioCue_Current& InAudioCueCurrent,
            const FFragment_AudioDirector_Current& InDirectorCurrent)
        -> void
    {
        // Reset state if no tracks exist
        if (InDirectorCurrent.Get_TracksByName().IsEmpty())
        {
            InAudioCueCurrent._ActiveTracks.Empty();
            InAudioCueCurrent._HasFiredAllTracksFinished = false;
            return;
        }

        // Update active tracks based on current director state
        InAudioCueCurrent._ActiveTracks.Empty();
        bool HasPlayingTracks = false;

        for (const auto& [TrackName, TrackHandle] : InDirectorCurrent.Get_TracksByName())
        {
            if (ck::Is_NOT_Valid(TrackHandle))
            { continue; }

            InAudioCueCurrent._ActiveTracks.Add(TrackHandle);

            if (TrackHandle.Has_Any<FTag_AudioTrack_NeedsSetup, FFragment_AudioTrack_Requests>())
            {
                HasPlayingTracks = true;
                break;
            }

            if (const auto TrackState = UCk_Utils_AudioTrack_UE::Get_State(TrackHandle);
                TrackState == ECk_AudioTrack_State::Playing ||
                TrackState == ECk_AudioTrack_State::FadingIn ||
                TrackState == ECk_AudioTrack_State::FadingOut ||
                TrackState == ECk_AudioTrack_State::Paused)
            {
                HasPlayingTracks = true;
                break;
            }
        }

        // Fire "all tracks finished" signal if:
        // 1. We have tracks but none are playing/active
        // 2. We haven't already fired this signal for this set of tracks
        if (NOT HasPlayingTracks &&
            NOT InAudioCueCurrent._ActiveTracks.IsEmpty() &&
            NOT InAudioCueCurrent._HasFiredAllTracksFinished)
        {
            InAudioCueCurrent._HasFiredAllTracksFinished = true;

            ck::audio::Verbose(TEXT("AudioCue [{}] - All tracks finished, firing OnAllTracksFinished signal"), InHandle);

            UUtils_Signal_OnAudioCue_AllTracksFinished::Broadcast(InHandle, MakePayload(InHandle));
        }
        else if (HasPlayingTracks)
        {
            // Reset the flag when we have active tracks again
            InAudioCueCurrent._HasFiredAllTracksFinished = false;
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioCue_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent)
            -> void
    {
        ck::audio::Verbose(TEXT("Tearing down AudioCue [{}]"), InHandle);

        // TODO: Why? We're tearing down...
        InCurrent._RecentTracks.Empty();
        InCurrent._LastSelectedIndex = INDEX_NONE;
        InCurrent._ActiveTracks.Empty();
        InCurrent._HasFiredAllTracksFinished = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
