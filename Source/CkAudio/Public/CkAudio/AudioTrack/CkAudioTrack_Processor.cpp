#include "CkAudioTrack_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioTrack_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include <Components/AudioComponent.h>
#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AudioTrack_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent) const
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Cannot setup AudioTrack [{}] - no valid world"), InHandle)
        { return; }

        // Create AudioComponent
        auto AudioComponent = NewObject<UAudioComponent>(World);
        CK_ENSURE_IF_NOT(ck::IsValid(AudioComponent), TEXT("Failed to create AudioComponent for AudioTrack [{}]"), InHandle)
        { return; }

        if (ck::IsValid(InParams.Get_ScriptAsset()))
        { UCk_Utils_EntityScript_UE::Add(InHandle, InParams.Get_ScriptAsset(), FInstancedStruct{}); }

        AudioComponent->SetSound(InParams.Get_Sound());
        AudioComponent->bAutoActivate = false;
        AudioComponent->bIsUISound = false;
        AudioComponent->SetVolumeMultiplier(0.0f); // Start silent

        InCurrent._AudioComponent = TStrongObjectPtr(AudioComponent);
        InCurrent._State = ECk_AudioTrack_State::Stopped;
        InCurrent._CurrentVolume = 0.0f;
        InCurrent._TargetVolume = 0.0f;
        InCurrent._FadeSpeed = 0.0f;

        ck::audio::Verbose(TEXT("Setup AudioTrack [{}] with sound [{}]"),
            InParams.Get_TrackName(),
            InParams.Get_Sound() ? InParams.Get_Sound()->GetName() : TEXT("None"));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioTrack_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_AudioTrack_Requests& InRequests)
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
        FProcessor_AudioTrack_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FCk_Request_AudioTrack_Play& InRequest)
            -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._AudioComponent), TEXT("AudioTrack [{}] has no AudioComponent"), InHandle)
        { return; }

        ck::audio::Verbose(TEXT("Handling play request for AudioTrack [{}]"), InParams.Get_TrackName());

        auto FadeTime = InRequest.Get_FadeInTime();
        if (FadeTime <= FCk_Time::ZeroSecond())
        { FadeTime = InParams.Get_DefaultFadeInTime(); }

        const auto TargetVolume = InParams.Get_Volume();

        if (InCurrent._State == ECk_AudioTrack_State::Stopped || NOT InCurrent._AudioComponent->IsPlaying())
        {
            // Start playback
            InCurrent._AudioComponent->SetSound(InParams.Get_Sound());
            InCurrent._AudioComponent->SetBoolParameter(TEXT("Loop"), InParams.Get_Loop());
            InCurrent._AudioComponent->Play();

            InCurrent._State = FadeTime > FCk_Time::ZeroSecond() ? ECk_AudioTrack_State::FadingIn : ECk_AudioTrack_State::Playing;

            UUtils_Signal_OnAudioTrack_PlaybackStarted::Broadcast(InHandle, MakePayload(InHandle, InCurrent._State));
        }

        // Setup fade if needed
        if (FadeTime > FCk_Time::ZeroSecond())
        {
            InCurrent._TargetVolume = TargetVolume;
            InCurrent._FadeSpeed = TargetVolume / FadeTime.Get_Seconds();
            InHandle.AddOrGet<FTag_AudioTrack_IsFading>();
        }
        else
        {
            InCurrent._CurrentVolume = TargetVolume;
            InCurrent._TargetVolume = TargetVolume;
            InCurrent._AudioComponent->SetVolumeMultiplier(TargetVolume);
            InCurrent._State = ECk_AudioTrack_State::Playing;
        }

        InHandle.AddOrGet<FTag_AudioTrack_IsPlaying>();
    }

    auto
        FProcessor_AudioTrack_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FCk_Request_AudioTrack_Stop& InRequest)
            -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._AudioComponent), TEXT("AudioTrack [{}] has no AudioComponent"), InHandle)
        { return; }

        ck::audio::Verbose(TEXT("Handling stop request for AudioTrack [{}]"), InParams.Get_TrackName());

        auto FadeTime = InRequest.Get_FadeOutTime();
        if (FadeTime <= FCk_Time::ZeroSecond())
        {
            FadeTime = InParams.Get_DefaultFadeOutTime();
        }

        if (FadeTime > FCk_Time::ZeroSecond() && InCurrent._CurrentVolume > 0.0f)
        {
            // Fade out
            InCurrent._TargetVolume = 0.0f;
            InCurrent._FadeSpeed = -InCurrent._CurrentVolume / FadeTime.Get_Seconds();
            InCurrent._State = ECk_AudioTrack_State::FadingOut;
            InHandle.AddOrGet<FTag_AudioTrack_IsFading>();
        }
        else
        {
            // Stop immediately
            InCurrent._AudioComponent->Stop();
            InCurrent._State = ECk_AudioTrack_State::Stopped;
            InCurrent._CurrentVolume = 0.0f;
            InCurrent._TargetVolume = 0.0f;
            InCurrent._AudioComponent->SetVolumeMultiplier(0.0f);

            InHandle.Try_Remove<FTag_AudioTrack_IsPlaying>();
            InHandle.Try_Remove<FTag_AudioTrack_IsFading>();

            UUtils_Signal_OnAudioTrack_PlaybackFinished::Broadcast(InHandle, MakePayload(InHandle, InCurrent._State));
        }
    }

    auto
        FProcessor_AudioTrack_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FCk_Request_AudioTrack_SetVolume& InRequest)
            -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._AudioComponent), TEXT("AudioTrack [{}] has no AudioComponent"), InHandle)
        { return; }

        ck::audio::VeryVerbose(TEXT("Handling volume request for AudioTrack [{}] to [{}]"),
            InParams.Get_TrackName(), InRequest.Get_TargetVolume());

        const auto FadeTime = InRequest.Get_FadeTime();
        const auto TargetVolume = FMath::Clamp(InRequest.Get_TargetVolume(), 0.0f, 1.0f);

        if (FadeTime > FCk_Time::ZeroSecond())
        {
            InCurrent._TargetVolume = TargetVolume;
            InCurrent._FadeSpeed = (TargetVolume - InCurrent._CurrentVolume) / FadeTime.Get_Seconds();
            InHandle.AddOrGet<FTag_AudioTrack_IsFading>();
        }
        else
        {
            InCurrent._CurrentVolume = TargetVolume;
            InCurrent._TargetVolume = TargetVolume;
            InCurrent._AudioComponent->SetVolumeMultiplier(TargetVolume);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioTrack_Playback::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent) const
            -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._AudioComponent), TEXT("AudioTrack [{}] has no AudioComponent"), InHandle)
        { return; }

        // Update fading
        if (InCurrent._FadeSpeed != 0.0f)
        {
            const auto DeltaVolume = InCurrent._FadeSpeed * InDeltaT.Get_Seconds();
            auto NewVolume = InCurrent._CurrentVolume + DeltaVolume;

            // Check if fade completed
            const auto FadeCompleted = (InCurrent._FadeSpeed > 0.0f && NewVolume >= InCurrent._TargetVolume) ||
                                      (InCurrent._FadeSpeed < 0.0f && NewVolume <= InCurrent._TargetVolume);

            if (FadeCompleted)
            {
                NewVolume = InCurrent._TargetVolume;
                InCurrent._FadeSpeed = 0.0f;
                InHandle.Remove<FTag_AudioTrack_IsFading>();

                // Handle state transitions after fade
                if (InCurrent._State == ECk_AudioTrack_State::FadingIn)
                {
                    InCurrent._State = ECk_AudioTrack_State::Playing;
                }
                else if (InCurrent._State == ECk_AudioTrack_State::FadingOut)
                {
                    InCurrent._State = ECk_AudioTrack_State::Stopped;
                    InCurrent._AudioComponent->Stop();
                    InHandle.Try_Remove<FTag_AudioTrack_IsPlaying>();

                    UUtils_Signal_OnAudioTrack_PlaybackFinished::Broadcast(InHandle, MakePayload(InHandle, InCurrent._State));
                }

                UUtils_Signal_OnAudioTrack_FadeCompleted::Broadcast(InHandle, MakePayload(InHandle, NewVolume, InCurrent._State));
            }

            InCurrent._CurrentVolume = NewVolume;
            InCurrent._AudioComponent->SetVolumeMultiplier(NewVolume);
        }

        // Check if audio finished naturally (non-looping)
        if (InHandle.Has<FTag_AudioTrack_IsPlaying>() &&
            InCurrent._State == ECk_AudioTrack_State::Playing &&
            NOT InCurrent._AudioComponent->IsPlaying())
        {
            ck::audio::VeryVerbose(TEXT("AudioTrack [{}] finished playing naturally"), InParams.Get_TrackName());

            InCurrent._State = ECk_AudioTrack_State::Stopped;
            InCurrent._CurrentVolume = 0.0f;
            InCurrent._TargetVolume = 0.0f;
            InHandle.Try_Remove<FTag_AudioTrack_IsPlaying>();

            UUtils_Signal_OnAudioTrack_PlaybackFinished::Broadcast(InHandle, MakePayload(InHandle, InCurrent._State));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioTrack_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent) const
            -> void
    {
        ck::audio::Verbose(TEXT("Tearing down AudioTrack [{}]"), InParams.Get_TrackName());

        if (ck::IsValid(InCurrent._AudioComponent))
        {
            InCurrent._AudioComponent->Stop();
            InCurrent._AudioComponent->SetSound(nullptr);
            InCurrent._AudioComponent->DestroyComponent();
            InCurrent._AudioComponent = nullptr;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
