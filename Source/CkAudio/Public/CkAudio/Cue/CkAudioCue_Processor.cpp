#include "CkAudioCue_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioCue_Utils.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Utils.h"
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
            const FFragment_EntityScript_Current& InEntityScript) const
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::audio::Verbose(TEXT("Setting up AudioCue [{}]"), InHandle);

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
            FFragment_AudioCue_Requests& InRequestsComp) const
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
        FProcessor_AudioCue_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent) const
            -> void
    {
        ck::audio::Verbose(TEXT("Tearing down AudioCue [{}]"), InHandle);

        InCurrent._RecentTracks.Empty();
        InCurrent._LastSelectedIndex = INDEX_NONE;

        ck::audio::VeryVerbose(TEXT("AudioCue [{}] teardown complete"), InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
