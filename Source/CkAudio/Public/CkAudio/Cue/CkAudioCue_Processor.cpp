#include "CkAudioCue_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioCue_Utils.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Utils.h"
#include "CkAudio/AudioTrack/CkAudioTrack_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using RecordOfAudioDirectors_Utils = TUtils_RecordOfEntities<FFragment_RecordOfAudioDirectors>;

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

        const UCk_AudioCue_EntityScript* AudioCueScript = Cast<UCk_AudioCue_EntityScript>(InEntityScript.Get_Script().Get());
        CK_ENSURE_IF_NOT(ck::IsValid(AudioCueScript),
            TEXT("AudioCue [{}] does not have valid AudioCue EntityScript"), InHandle)
        { return; }

        // Determine if we need AudioDirector
        const auto NeedsDirector = AudioCueScript->Get_HasValidTrackLibrary() ||
                                  AudioCueScript->Get_MaxConcurrentTracks() > 1 ||
                                  (AudioCueScript->Get_HasValidSingleTrack() && AudioCueScript->Get_HasValidTrackLibrary());

        if (NeedsDirector)
        {
            // Create AudioDirector for orchestration
            auto DirectorParams = FCk_Fragment_AudioDirector_ParamsData{AudioCueScript->GetClass()}
                .Set_DefaultCrossfadeDuration(AudioCueScript->Get_DefaultCrossfadeDuration())
                .Set_MaxConcurrentTracks(AudioCueScript->Get_MaxConcurrentTracks())
                .Set_AllowSamePriorityTracks(AudioCueScript->Get_AllowSamePriorityTracks());

            auto AudioDirector = UCk_Utils_AudioDirector_UE::Add(InHandle, DirectorParams);
            InCurrent._AudioDirector = AudioDirector;

            // Connect to RecordOfAudioDirectors
            RecordOfAudioDirectors_Utils::AddIfMissing(InHandle);
            RecordOfAudioDirectors_Utils::Request_Connect(InHandle, AudioDirector);

            // Pre-populate tracks for persistent cues
            const auto LifetimeBehavior = AudioCueScript->Get_LifetimeBehavior();
            if (LifetimeBehavior == ECk_Cue_LifetimeBehavior::Persistent ||
                LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed)
            {
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

            ck::audio::VeryVerbose(TEXT("AudioCue [{}] setup complete with Director [{}]"), InHandle, AudioDirector);
        }
        else
        {
            // Single track, no director needed
            InCurrent._AudioDirector = FCk_Handle_AudioDirector{}; // Invalid handle
            ck::audio::VeryVerbose(TEXT("AudioCue [{}] setup complete - single track mode"), InHandle);
        }
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

        DoSelectAndPlayTrack(InHandle, InCurrent, AudioCueScript,
            InRequest.Get_OverridePriority(), InRequest.Get_FadeInTime());
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

        CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._AudioDirector),
            TEXT("AudioCue [{}] has no AudioDirector"), InHandle)
        { return; }

        UCk_Utils_AudioDirector_UE::Request_StopAllTracks(InCurrent._AudioDirector, InRequest.Get_FadeOutTime());
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

        CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._AudioDirector),
            TEXT("AudioCue [{}] has no AudioDirector"), InHandle)
        { return; }

        UCk_Utils_AudioDirector_UE::Request_StopAllTracks(InCurrent._AudioDirector, InRequest.Get_FadeOutTime());
    }

    auto
        FProcessor_AudioCue_HandleRequests::
        DoSelectAndPlayTrack(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const UCk_AudioCue_EntityScript* InAudioCueScript,
            TOptional<int32> InOverridePriority,
            FCk_Time InFadeInTime)
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

        // Override priority if specified
        if (InOverridePriority.IsSet())
        {
            SelectedTrack.Set_Priority(InOverridePriority.GetValue());
        }

        if (ck::IsValid(InCurrent._AudioDirector))
        {
            // Use AudioDirector for orchestration
            UCk_Utils_AudioDirector_UE::Request_AddTrack(InCurrent._AudioDirector, SelectedTrack);
            UCk_Utils_AudioDirector_UE::Request_StartTrack(InCurrent._AudioDirector,
                SelectedTrack.Get_TrackName(), InOverridePriority, InFadeInTime);
        }
        else
        {
            // Direct AudioTrack creation for single-track cues
            auto AudioTrack = UCk_Utils_AudioTrack_UE::Create(InHandle, SelectedTrack);
            UCk_Utils_AudioTrack_UE::Request_Play(AudioTrack, InFadeInTime);
        }

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

        // AudioDirector will be destroyed as child entity automatically
        InCurrent._AudioDirector = FCk_Handle_AudioDirector{};
        InCurrent._RecentTracks.Empty();
        InCurrent._LastSelectedIndex = INDEX_NONE;

        ck::audio::VeryVerbose(TEXT("AudioCue [{}] teardown complete"), InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
