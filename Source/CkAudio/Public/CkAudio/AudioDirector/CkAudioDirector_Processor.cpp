#include "CkAudioDirector_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioDirector_Utils.h"

#include "CkAudio/AudioTrack/CkAudioTrack_Utils.h"
#include "CkAudio/MusicLibrary/CkMusicLibrary_Asset.h"
#include "CkAudio/StingerLibrary/CkStingerLibrary_Asset.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using RecordOfAudioTracks_Utils = TUtils_RecordOfEntities<FFragment_RecordOfAudioTracks>;

    auto
        FProcessor_AudioDirector_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent) const
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::audio::Verbose(TEXT("Setting up AudioDirector [{}]"), InHandle);

        InCurrent._CurrentHighestPriority = -1;
        InCurrent._TracksByName.Empty();

        // NEW: Initialize EntityScript if enabled
        if (ck::IsValid(InParams.Get_Script()))
        {
            ck::audio::Verbose(TEXT("Adding EntityScript [{}] to AudioDirector [{}]"),
                InParams.Get_Script()->GetName(), InHandle);

            UCk_Utils_EntityScript_UE::Add(InHandle, InParams.Get_Script(), {});
        }

        ck::audio::VeryVerbose(TEXT("AudioDirector [{}] setup complete"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioDirector_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            FFragment_AudioDirector_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_AudioDirector_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_AddTrack& InRequest)
            -> void
    {
        const auto& TrackParams = InRequest.Get_TrackParams();
        const auto TrackName = TrackParams.Get_TrackName();

        ck::audio::Verbose(TEXT("Adding track [{}] to AudioDirector [{}]"), TrackName, InHandle);

        // Check if track already exists
        if (InCurrent._TracksByName.Contains(TrackName))
        {
            ck::audio::Warning(TEXT("Track [{}] already exists in AudioDirector [{}], skipping"), TrackName, InHandle);
            return;
        }

        // Create track entity as child of director - connection handled in Create
        auto TrackHandle = UCk_Utils_AudioTrack_UE::Create(InHandle, TrackParams);

        // Store track reference
        InCurrent._TracksByName.Add(TrackName, TrackHandle);

        ck::audio::VeryVerbose(TEXT("Successfully added track [{}] as [{}] to AudioDirector [{}]"),
            TrackName, TrackHandle, InHandle);

        UUtils_Signal_OnAudioDirector_TrackAdded::Broadcast(InHandle, MakePayload(InHandle, TrackName, TrackHandle));
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StartTrack& InRequest)
            -> void
    {
        const auto TrackName = InRequest.Get_TrackName();

        ck::audio::Verbose(TEXT("Starting track [{}] on AudioDirector [{}]"), TrackName, InHandle);

        // Find track
        auto* TrackHandlePtr = InCurrent._TracksByName.Find(TrackName);
        CK_ENSURE_IF_NOT(TrackHandlePtr != nullptr,
            TEXT("Track [{}] not found in AudioDirector [{}]"), TrackName, InHandle)
        { return; }

        auto TrackHandle = *TrackHandlePtr;
        CK_ENSURE_IF_NOT(ck::IsValid(TrackHandle),
            TEXT("Track [{}] handle is invalid in AudioDirector [{}]"), TrackName, InHandle)
        { return; }

        // Determine effective priority
        const auto TrackPriority = InRequest.Get_OverridePriority().Get(
            UCk_Utils_AudioTrack_UE::Get_Priority(TrackHandle));

        // Check priority override logic
        if (TrackPriority > InCurrent._CurrentHighestPriority ||
            (TrackPriority == InCurrent._CurrentHighestPriority && InParams.Get_AllowSamePriorityTracks()))
        {
            const auto OverrideBehavior = UCk_Utils_AudioTrack_UE::Get_OverrideBehavior(TrackHandle);
            DoHandlePriorityOverride(InHandle, InParams, InCurrent, TrackHandle, TrackPriority, OverrideBehavior);
        }
        else if (TrackPriority < InCurrent._CurrentHighestPriority)
        {
            ck::audio::VeryVerbose(TEXT("Track [{}] priority [{}] is lower than current highest [{}], ignoring"),
                TrackName, TrackPriority, InCurrent._CurrentHighestPriority);
            return;
        }

        // Start the track
        auto FadeInTime = InRequest.Get_FadeInTime();
        if (FadeInTime <= FCk_Time::ZeroSecond())
        {
            FadeInTime = InParams.Get_DefaultCrossfadeDuration();
        }

        UCk_Utils_AudioTrack_UE::Request_Play(TrackHandle, FadeInTime);
        InCurrent._CurrentHighestPriority = TrackPriority;

        ck::audio::Verbose(TEXT("Started track [{}] with priority [{}] on AudioDirector [{}]"),
            TrackName, TrackPriority, InHandle);

        UUtils_Signal_OnAudioDirector_TrackStarted::Broadcast(InHandle, MakePayload(InHandle, TrackName, TrackHandle));
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StopTrack& InRequest)
            -> void
    {
        const auto TrackName = InRequest.Get_TrackName();

        ck::audio::Verbose(TEXT("Stopping track [{}] on AudioDirector [{}]"), TrackName, InHandle);

        auto* TrackHandlePtr = InCurrent._TracksByName.Find(TrackName);
        CK_ENSURE_IF_NOT(TrackHandlePtr != nullptr,
            TEXT("Track [{}] not found in AudioDirector [{}]"), TrackName, InHandle)
        { return; }

        auto TrackHandle = *TrackHandlePtr;
        CK_ENSURE_IF_NOT(ck::IsValid(TrackHandle),
            TEXT("Track [{}] handle is invalid in AudioDirector [{}]"), TrackName, InHandle)
        { return; }

        auto FadeOutTime = InRequest.Get_FadeOutTime();
        if (FadeOutTime <= FCk_Time::ZeroSecond())
        {
            FadeOutTime = InParams.Get_DefaultCrossfadeDuration();
        }

        UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FadeOutTime);

        // Update highest priority if this was the highest priority track
        const auto TrackPriority = UCk_Utils_AudioTrack_UE::Get_Priority(TrackHandle);
        if (TrackPriority >= InCurrent._CurrentHighestPriority)
        {
            // Recalculate highest priority from remaining playing tracks
            auto NewHighestPriority = -1;
            for (const auto& [OtherTrackName, OtherTrackHandle] : InCurrent._TracksByName)
            {
                if (OtherTrackHandle != TrackHandle && ck::IsValid(OtherTrackHandle))
                {
                    const auto OtherState = UCk_Utils_AudioTrack_UE::Get_State(OtherTrackHandle);
                    if (OtherState == ECk_AudioTrack_State::Playing || OtherState == ECk_AudioTrack_State::FadingIn)
                    {
                        const auto OtherPriority = UCk_Utils_AudioTrack_UE::Get_Priority(OtherTrackHandle);
                        NewHighestPriority = FMath::Max(NewHighestPriority, OtherPriority);
                    }
                }
            }
            InCurrent._CurrentHighestPriority = NewHighestPriority;
        }

        ck::audio::Verbose(TEXT("Stopped track [{}] on AudioDirector [{}], new highest priority: [{}]"),
            TrackName, InHandle, InCurrent._CurrentHighestPriority);

        UUtils_Signal_OnAudioDirector_TrackStopped::Broadcast(InHandle, MakePayload(InHandle, TrackName, TrackHandle));
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StopAllTracks& InRequest)
            -> void
    {
        ck::audio::Verbose(TEXT("Stopping all tracks on AudioDirector [{}]"), InHandle);

        auto FadeOutTime = InRequest.Get_FadeOutTime();
        if (FadeOutTime <= FCk_Time::ZeroSecond())
        {
            FadeOutTime = InParams.Get_DefaultCrossfadeDuration();
        }

        for (auto& [TrackName, TrackHandle] : InCurrent._TracksByName)
        {
            if (ck::IsValid(TrackHandle))
            {
                UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FadeOutTime);
                UUtils_Signal_OnAudioDirector_TrackStopped::Broadcast(InHandle, MakePayload(InHandle, TrackName, TrackHandle));
            }
        }

        for (auto& [TrackName, TrackHandle] : InCurrent._ActiveMusicTracks)
        {
            if (ck::IsValid(TrackHandle))
            {
                UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FadeOutTime);
                UUtils_Signal_OnAudioDirector_TrackStopped::Broadcast(InHandle, MakePayload(InHandle, TrackName, TrackHandle));
            }
        }

        InCurrent._CurrentHighestPriority = -1;

        ck::audio::Verbose(TEXT("Stopped all tracks on AudioDirector [{}]"), InHandle);
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_AddMusicLibrary& InRequest)
            -> void
    {
        const auto& Library = InRequest.Get_MusicLibrary();
        CK_ENSURE_IF_NOT(ck::IsValid(Library), TEXT("Invalid MusicLibrary in request")) { return; }

        const auto LibraryName = Library->Get_LibraryName();
        ck::audio::Verbose(TEXT("Adding MusicLibrary [{}] to AudioDirector [{}]"), LibraryName, InHandle);

        InCurrent._MusicLibrariesByName.Add(LibraryName, TStrongObjectPtr(Library.Get()));

        // Initialize EntityScript if library has one
        if (ck::IsValid(Library->Get_ScriptAsset()))
        {
            UCk_Utils_EntityScript_UE::Add(InHandle, Library->Get_ScriptAsset(), FInstancedStruct{});
        }
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_AddStingerLibrary& InRequest)
            -> void
    {
        const auto& Library = InRequest.Get_StingerLibrary();
        CK_ENSURE_IF_NOT(ck::IsValid(Library), TEXT("Invalid StingerLibrary in request")) { return; }

        const auto LibraryName = Library->Get_LibraryName();
        ck::audio::Verbose(TEXT("Adding StingerLibrary [{}] to AudioDirector [{}]"), LibraryName, InHandle);

        InCurrent._StingerLibrariesByName.Add(LibraryName, TStrongObjectPtr(Library.Get()));
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StartMusicLibrary& InRequest)
            -> void
    {
        const auto LibraryName = InRequest.Get_LibraryName();
        ck::audio::Verbose(TEXT("Starting MusicLibrary [{}] on AudioDirector [{}]"), LibraryName, InHandle);

        auto Library = InCurrent._MusicLibrariesByName.FindRef(LibraryName);
        CK_ENSURE_IF_NOT(ck::IsValid(Library), TEXT("MusicLibrary [{}] not found"), LibraryName)
        { return; }

        // Get next track from library
        const auto TrackIndex = Library->Get_NextTrack({});
        CK_ENSURE_IF_NOT(TrackIndex != INDEX_NONE, TEXT("No tracks available in library [{}]"), LibraryName)
        { return; }

        const auto& TrackEntry = Library->Get_Tracks()[TrackIndex];
        CK_ENSURE_IF_NOT(ck::IsValid(TrackEntry.Get_Sound()), TEXT("Invalid sound in track [{}]"), TrackIndex)
        { return; }

        // Create AudioTrack from library entry
        const auto EffectivePriority = InRequest.Get_OverridePriority().Get(Library->Get_Priority());

        auto TrackParams = FCk_Fragment_AudioTrack_ParamsData{
            LibraryName,
            TrackEntry.Get_Sound()
        }.Set_Priority(EffectivePriority)
         .Set_OverrideBehavior(Library->Get_OverrideBehavior())
         .Set_Loop(TrackEntry.Get_OverrideLoop().Get(Library->Get_Loop()))
         .Set_Volume(TrackEntry.Get_Volume())
         .Set_DefaultFadeInTime(Library->Get_DefaultFadeInTime())
         .Set_DefaultFadeOutTime(Library->Get_DefaultFadeOutTime());

        // Add EntityScript if track has one
        if (ck::IsValid(TrackEntry.Get_ScriptAsset()))
        {
            TrackParams.Set_ScriptAsset(TrackEntry.Get_ScriptAsset());
        }

        // Stop current track from same library if exists
        if (auto* CurrentTrack = InCurrent._ActiveMusicTracks.Find(LibraryName))
        {
            if (ck::IsValid(*CurrentTrack))
            {
                UCk_Utils_AudioTrack_UE::Request_Stop(*CurrentTrack, Library->Get_DefaultFadeOutTime());
            }
        }

        // Create and start new track
        auto TrackEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
        UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(TrackEntity, InHandle);
        auto TrackHandle = UCk_Utils_AudioTrack_UE::Create(InHandle, TrackParams);

        InCurrent._ActiveMusicTracks.Add(LibraryName, TrackHandle);

        auto FadeInTime = InRequest.Get_FadeInTime();
        if (FadeInTime <= FCk_Time::ZeroSecond())
        {
            FadeInTime = Library->Get_DefaultFadeInTime();
        }

        UCk_Utils_AudioTrack_UE::Request_Play(TrackHandle, FadeInTime);
        InCurrent._CurrentHighestPriority = FMath::Max(InCurrent._CurrentHighestPriority, EffectivePriority);
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_PlayStinger& InRequest)
            -> void
    {
        const auto StingerName = InRequest.Get_StingerName();
        ck::audio::Verbose(TEXT("Playing stinger [{}] on AudioDirector [{}]"), StingerName, InHandle);

        const auto CurrentTime = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle)->GetTimeSeconds();
        if (const auto* CooldownEnd = InCurrent._StingerCooldowns.Find(StingerName))
        {
            if (CurrentTime < *CooldownEnd)
            {
                ck::audio::VeryVerbose(TEXT("Stinger [{}] on cooldown, ignoring"), StingerName);
                return;
            }
        }

        auto StingerEntry = FCk_StingerEntry{};
        auto StingerFound = false;

        for (const auto& [LibraryName, Library] : InCurrent._StingerLibrariesByName)
        {
            if (ck::IsValid(Library))
            {
                StingerEntry = Library->Get_StingerEntry(StingerName, StingerFound);
                if (StingerFound) break;
            }
        }

        CK_ENSURE_IF_NOT(StingerFound, TEXT("Stinger [{}] not found in any library"), StingerName) { return; }

        // Create AudioTrack for stinger
        auto TrackParams = FCk_Fragment_AudioTrack_ParamsData{
            StingerName,
            StingerEntry.Get_Sound()
        }.Set_Priority(StingerEntry.Get_Priority())
         .Set_Loop(false)  // Stingers never loop
         .Set_Volume(InRequest.Get_OverrideVolume().Get(StingerEntry.Get_Volume()));

        if (ck::IsValid(StingerEntry.Get_ScriptAsset()))
        {
            TrackParams.Set_ScriptAsset(StingerEntry.Get_ScriptAsset());
        }

        auto TrackEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
        UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(TrackEntity, InHandle);
        auto TrackHandle = UCk_Utils_AudioTrack_UE::Create(InHandle, TrackParams);

        UCk_Utils_AudioTrack_UE::Request_Play(TrackHandle, FCk_Time::ZeroSecond());

        // Set cooldown
        InCurrent._StingerCooldowns.Add(StingerName, CurrentTime + StingerEntry.Get_Cooldown());
    }

    // --------------------------------------------------------------------------------------------------------------------


    auto
        FProcessor_AudioDirector_HandleRequests::
        DoHandlePriorityOverride(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            FCk_Handle_AudioTrack InNewTrack,
            int32 InNewTrackPriority,
            ECk_AudioTrack_OverrideBehavior InOverrideBehavior)
            -> void
    {
        ck::audio::VeryVerbose(TEXT("Handling priority override for track with priority [{}], behavior [{}]"),
            InNewTrackPriority, InOverrideBehavior);

        switch (InOverrideBehavior)
        {
            case ECk_AudioTrack_OverrideBehavior::Interrupt:
            {
                // Stop all lower priority tracks immediately
                DoStopLowerPriorityTracks(InParams, InCurrent, InNewTrackPriority);
                break;
            }
            case ECk_AudioTrack_OverrideBehavior::Crossfade:
            {
                // Fade out lower priority tracks over crossfade duration
                const auto CrossfadeTime = InParams.Get_DefaultCrossfadeDuration();
                for (auto& [TrackName, TrackHandle] : InCurrent._TracksByName)
                {
                    if (TrackHandle != InNewTrack && ck::IsValid(TrackHandle))
                    {
                        const auto TrackPriority = UCk_Utils_AudioTrack_UE::Get_Priority(TrackHandle);
                        if (TrackPriority < InNewTrackPriority)
                        {
                            const auto TrackState = UCk_Utils_AudioTrack_UE::Get_State(TrackHandle);
                            if (TrackState == ECk_AudioTrack_State::Playing || TrackState == ECk_AudioTrack_State::FadingIn)
                            {
                                UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, CrossfadeTime);
                            }
                        }
                    }
                }
                break;
            }
            case ECk_AudioTrack_OverrideBehavior::Queue:
            {
                // Don't stop anything, let current tracks finish naturally
                // TODO: Could implement queuing logic with CkTimer here
                break;
            }
        }
    }

    auto
        FProcessor_AudioDirector_HandleRequests::
        DoStopLowerPriorityTracks(
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            int32 InNewTrackPriority)
            -> void
    {
        for (auto& [TrackName, TrackHandle] : InCurrent._TracksByName)
        {
            if (ck::IsValid(TrackHandle))
            {
                const auto TrackPriority = UCk_Utils_AudioTrack_UE::Get_Priority(TrackHandle);
                if (TrackPriority < InNewTrackPriority)
                {
                    const auto TrackState = UCk_Utils_AudioTrack_UE::Get_State(TrackHandle);
                    if (TrackState == ECk_AudioTrack_State::Playing || TrackState == ECk_AudioTrack_State::FadingIn)
                    {
                        UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FCk_Time::ZeroSecond());
                    }
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioDirector_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent) const
            -> void
    {
        ck::audio::Verbose(TEXT("Tearing down AudioDirector [{}]"), InHandle);

        // Stop all tracks immediately
        for (auto& [TrackName, TrackHandle] : InCurrent._TracksByName)
        {
            if (ck::IsValid(TrackHandle))
            {
                UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FCk_Time::ZeroSecond());
            }
        }

        for (auto& [LibraryName, TrackHandle] : InCurrent._ActiveMusicTracks)
        {
            if (ck::IsValid(TrackHandle))
            {
                UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FCk_Time::ZeroSecond());
            }
        }
        InCurrent._ActiveMusicTracks.Empty();

        InCurrent._TracksByName.Empty();
        InCurrent._CurrentHighestPriority = -1;

        ck::audio::VeryVerbose(TEXT("AudioDirector [{}] teardown complete"), InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
