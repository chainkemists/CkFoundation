#include "CkAudioTrack_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioTrack_Utils.h"

#include "CkAudio/CkAudio_Settings.h"

#include "CkCore/Color/Ck_Utils_Color.h"
#include "CkCore/Debug/CkDebugDraw_Subsystem.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <Components/AudioComponent.h>
#include <Engine/Engine.h>
#include <Sound/SoundCue.h>

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
        AudioComponent->SetVolumeMultiplier(0.0f); // Start silent

        const auto DirectParentThatMayHaveATransform = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto IsSpatial = UCk_Utils_Transform_UE::Has(DirectParentThatMayHaveATransform);

        // Configure spatial vs non-spatial audio
        if (IsSpatial)
        {
            // Enable 3D spatial audio
            AudioComponent->bIsUISound = false;
            AudioComponent->bAllowSpatialization = true;

            // Priority: SoundCue settings > Library settings > Defaults
            USoundAttenuation* AttenuationToUse = nullptr;
            USoundConcurrency* ConcurrencyToUse = nullptr;
            bool SoundCueOverridesAttenuation = false;
            bool SoundCueOverridesConcurrency = false;

            // Check if SoundCue overrides settings
            if (auto* SoundCue = Cast<USoundCue>(InParams.Get_Sound()))
            {
                if (SoundCue->bOverrideAttenuation)
                {
                    SoundCueOverridesAttenuation = true;
                }
                else
                {
                    AttenuationToUse = SoundCue->AttenuationSettings;
                }

                if (SoundCue->bOverrideConcurrency)
                {
                    SoundCueOverridesConcurrency = true;
                }
                else if (SoundCue->ConcurrencySet.Num() > 0)
                {
                    ConcurrencyToUse = *SoundCue->ConcurrencySet.begin();
                }
            }

            // Fall back to library settings if SoundCue doesn't override or have settings
            if (!SoundCueOverridesAttenuation && !ck::IsValid(AttenuationToUse))
            {
                AttenuationToUse = InParams.Get_LibraryAttenuationSettings();
            }
            if (!SoundCueOverridesConcurrency && !ck::IsValid(ConcurrencyToUse))
            {
                ConcurrencyToUse = InParams.Get_LibraryConcurrencySettings();
            }

            // Apply attenuation
            if (!SoundCueOverridesAttenuation)
            {
                if (ck::IsValid(AttenuationToUse))
                {
                    AudioComponent->AttenuationSettings = AttenuationToUse;
                    AudioComponent->bOverrideAttenuation = false;
                }
                else
                {
                    ck::audio::Warning(TEXT("Spatial AudioTrack [{}] has no attenuation settings. Using defaults."),
                        InParams.Get_TrackName());

                    AudioComponent->bOverrideAttenuation = true;
                    auto DefaultAttenuation = FSoundAttenuationSettings{};
                    DefaultAttenuation.bAttenuate = true;
                    DefaultAttenuation.AttenuationShape = EAttenuationShape::Sphere;
                    DefaultAttenuation.FalloffDistance = 1000.0f;
                    DefaultAttenuation.AttenuationShapeExtents = FVector(1000.0f);
                    AudioComponent->AttenuationOverrides = DefaultAttenuation;
                }
            }

            // Apply concurrency
            if (!SoundCueOverridesConcurrency)
            {
                if (ck::IsValid(ConcurrencyToUse))
                {
                    AudioComponent->ConcurrencySet.Add(ConcurrencyToUse);
                }
                else
                {
                    ck::audio::Warning(TEXT("Spatial AudioTrack [{}] has no concurrency settings. Using defaults."),
                        InParams.Get_TrackName());
                }
            }
        }
        else
        {
            // 2D audio (no spatialization)
            AudioComponent->bIsUISound = true;
            AudioComponent->bAllowSpatialization = false;

            // Still apply concurrency for 2D sounds if available (SoundCue first, then library)
            USoundConcurrency* ConcurrencyToUse = nullptr;
            bool SoundCueOverridesConcurrency = false;

            if (auto* SoundCue = Cast<USoundCue>(InParams.Get_Sound()))
            {
                if (SoundCue->bOverrideConcurrency)
                {
                    SoundCueOverridesConcurrency = true;
                }
                else if (SoundCue->ConcurrencySet.Num() > 0)
                {
                    ConcurrencyToUse = *SoundCue->ConcurrencySet.begin();
                }
            }

            if (!SoundCueOverridesConcurrency && !ck::IsValid(ConcurrencyToUse))
            {
                ConcurrencyToUse = InParams.Get_LibraryConcurrencySettings();
            }

            if (!SoundCueOverridesConcurrency && ck::IsValid(ConcurrencyToUse))
            {
                AudioComponent->ConcurrencySet.Add(ConcurrencyToUse);
            }
        }

        InCurrent._AudioComponent = TStrongObjectPtr(AudioComponent);
        InCurrent._State = ECk_AudioTrack_State::Stopped;
        InCurrent._CurrentVolume = 0.0f;
        InCurrent._TargetVolume = 0.0f;
        InCurrent._FadeSpeed = 0.0f;

        // Add transform feature for spatial audio tracks
        if (IsSpatial)
        {
            auto HandleTransform = UCk_Utils_Transform_UE::Add(InHandle, FTransform::Identity, ECk_Replication::DoesNotReplicate);
            auto OwnerTransform = UCk_Utils_Transform_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle));
            UCk_Utils_SceneNode_UE::Add(HandleTransform, OwnerTransform, FTransform::Identity);
        }

        ck::audio::Verbose(TEXT("Setup AudioTrack [{}] with sound [{}], spatial: [{}]"),
            InParams.Get_TrackName(),
            InParams.Get_Sound() ? InParams.Get_Sound()->GetName() : TEXT("None"),
            IsSpatial);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AudioTrack_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            FFragment_AudioTrack_Current& InCurrent,
            const FFragment_AudioTrack_Requests& InRequestsComp) const
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
        FProcessor_AudioTrack_SpatialUpdate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        if (NOT ck::IsValid(InCurrent._AudioComponent))
        { return; }

        auto HandleTransform = UCk_Utils_Transform_UE::Cast(InHandle);

        const auto& WorldTransform = InTransform.Get_Transform();
        InCurrent._AudioComponent->SetWorldLocation(WorldTransform.GetLocation());
        InCurrent._AudioComponent->SetWorldRotation(WorldTransform.GetRotation());
        InCurrent._AudioComponent->OnUpdateTransform(EUpdateTransformFlags::SkipPhysicsUpdate);
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

namespace ck
{
    // Helper functions (add these first)
    auto
        AudioTrack_UpdateDebugInfo(
            FCk_Handle_AudioTrack InHandle,
            FCk_Time InDeltaT,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug)
        -> void
    {
        // Update state color
        switch (InCurrent.Get_State())
        {
            case ECk_AudioTrack_State::Playing:
                InDebug._StateColor = LinearColor::Green;
                break;
            case ECk_AudioTrack_State::FadingIn:
                InDebug._StateColor = LinearColor::LightGreen;
                break;
            case ECk_AudioTrack_State::FadingOut:
                InDebug._StateColor = LinearColor::Orange;
                break;
            case ECk_AudioTrack_State::Paused:
                InDebug._StateColor = LinearColor::Yellow;
                break;
            case ECk_AudioTrack_State::Stopped:
            default:
                InDebug._StateColor = LinearColor::Red;
                break;
        }

        // Setup progress tracking callback when debug is enabled
        if (ck::IsValid(InCurrent.Get_AudioComponent()) && NOT InDebug._ProgressDelegateHandle.IsValid())
        {
            InDebug._ProgressDelegateHandle = InCurrent.Get_AudioComponent()->OnAudioPlaybackPercentNative.AddLambda(
                [InHandle](const UAudioComponent* InAudioComp, const USoundWave* InSoundWave, float InPercent)
                {
                    auto NonConstHandle = InHandle;
                    if (ck::Is_NOT_Valid(NonConstHandle))
                    { return; }

                    auto& Debug = NonConstHandle.Get<FFragment_AudioTrack_Debug>();
                    Debug._PlaybackPercent = InPercent;
                }
            );
        }

        // Clear progress callback if audio component is invalid
        if (NOT ck::IsValid(InCurrent.Get_AudioComponent()) && InDebug._ProgressDelegateHandle.IsValid())
        {
            // Note: We can't clear the delegate here because we don't have access to the old component
            InDebug._ProgressDelegateHandle.Reset();
            InDebug._PlaybackPercent = 0.0f;
        }

        // Update pulse animation for playing tracks
        if (InCurrent.Get_State() == ECk_AudioTrack_State::Playing ||
            InCurrent.Get_State() == ECk_AudioTrack_State::FadingIn)
        {
            constexpr auto PulseFrequency = 2.0f; // Hz
            const auto CurrentTime = InDebug._LastPulseTime + InDeltaT;

            // Create pulsing effect based on current volume
            const auto PulseAmount = FMath::Sin(CurrentTime.Get_Seconds() * PulseFrequency * 2.0f * PI) * 0.3f + 0.7f;
            InDebug._CurrentPulseScale = PulseAmount * InCurrent.Get_CurrentVolume();

            InDebug._LastPulseTime = CurrentTime;
        }
        else
        {
            InDebug._CurrentPulseScale = 1.0f;
            InDebug._LastPulseTime = FCk_Time::ZeroSecond();
        }
    }

    static auto
        AudioTrack_DrawSpatialDebug(
            FCk_Handle_AudioTrack InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            const FFragment_AudioTrack_Debug& InDebug,
            const FTransform& InTransform)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Cannot draw spatial debug for AudioTrack [{}] - no valid world"), InHandle)
        { return; }

        const auto& Position = InTransform.GetLocation();
        const auto LineThickness = UCk_Utils_AudioTrack_Settings::Get_DebugLineThickness();

        // Base radius configuration
        constexpr auto MaxRadius = 80.0f;
        constexpr auto CenterDotRadius = 3.0f;

        // Get colors
        const auto StateColor = InDebug.Get_StateColor();
        const auto VolumeColor = StateColor * 0.7f; // Slightly dimmed for the sector

        // 1. Draw center dot (always visible for positioning)
        UCk_Utils_DebugDraw_UE::DrawDebugPoint(
            World,
            Position,
            CenterDotRadius * 2.0f,
            StateColor,
            0.0f
        );

        // 2. Draw 3D volume bar
        if (InCurrent.Get_CurrentVolume() > 0.01f)
        {
            constexpr auto BarHeight = 60.0f;
            constexpr auto BarWidth = 8.0f;
            constexpr auto BarDepth = 8.0f;

            // Calculate filled height based on volume
            const auto FilledHeight = BarHeight * InCurrent.Get_CurrentVolume();
            const auto BarBasePos = Position;

            // Draw outer frame (full bar outline)
            const auto FrameExtent = FVector(BarWidth * 0.5f, BarDepth * 0.5f, BarHeight * 0.5f);
            UCk_Utils_DebugDraw_UE::DrawDebugBox(
                World,
                BarBasePos,
                FrameExtent,
                StateColor * 0.5f, // Dimmed frame color
                FRotator::ZeroRotator,
                0.0f,
                LineThickness * 0.5f
            );

            // Draw filled portion with stacked segments
            const auto NumFillSegments = FMath::Max(1, static_cast<int32>(FilledHeight / 4.0f)); // Segment every 4 units
            const auto SegmentHeight = FilledHeight / NumFillSegments;

            for (int32 I = 0; I < NumFillSegments; ++I)
            {
                const auto SegmentZ = (BarBasePos.Z - BarHeight * 0.5f) + (I * SegmentHeight) + (SegmentHeight * 0.5f);
                const auto SegmentPos = FVector(BarBasePos.X, BarBasePos.Y, SegmentZ);
                const auto SegmentExtent = FVector(BarWidth * 0.4f, BarDepth * 0.4f, SegmentHeight * 0.4f);

                // Gradient color: brighter at top
                const auto ColorIntensity = 0.6f + (0.4f * static_cast<float>(I) / static_cast<float>(NumFillSegments));
                auto SegmentColor = VolumeColor * ColorIntensity;

                UCk_Utils_DebugDraw_UE::DrawDebugBox(
                    World,
                    SegmentPos,
                    SegmentExtent,
                    SegmentColor,
                    FRotator::ZeroRotator,
                    0.0f,
                    LineThickness * 0.3f
                );
            }

            // Add horizontal lines for "fill effect"
            const auto NumFillLines = FMath::Max(2, static_cast<int32>(FilledHeight / 6.0f));
            for (int32 I = 0; I <= NumFillLines; ++I)
            {
                const auto LineZ = (BarBasePos.Z - BarHeight * 0.5f) + (I * FilledHeight / NumFillLines);
                const auto LineStart = FVector(BarBasePos.X - BarWidth * 0.4f, BarBasePos.Y - BarDepth * 0.4f, LineZ);
                const auto LineEnd = FVector(BarBasePos.X + BarWidth * 0.4f, BarBasePos.Y + BarDepth * 0.4f, LineZ);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(
                    World, LineStart, LineEnd, VolumeColor, 0.0f, LineThickness * 0.5f
                );
            }
        }

        // 3. Draw outer boundary circle (state-colored, dynamic size for fading)
        auto BoundaryRadius = MaxRadius;

        // Adjust boundary size based on fade state
        if (InCurrent.Get_State() == ECk_AudioTrack_State::FadingIn)
        {
            // Boundary grows from 0 to target size
            const auto FadeProgress = InCurrent.Get_CurrentVolume() / InParams.Get_Volume();
            BoundaryRadius = MaxRadius * FadeProgress;
        }
        else if (InCurrent.Get_State() == ECk_AudioTrack_State::FadingOut)
        {
            // Boundary shrinks based on remaining volume
            const auto FadeProgress = InCurrent.Get_CurrentVolume() / InParams.Get_Volume();
            BoundaryRadius = MaxRadius * FadeProgress;
        }

        UCk_Utils_DebugDraw_UE::DrawDebugCircle(
            World,
            Position,
            BoundaryRadius,
            16, // segments
            StateColor,
            0.0f,
            LineThickness,
            FVector(0.f, 1.f, 0.f), // Y axis
            FVector(0.f, 0.f, 1.f), // Z axis
            false // don't draw axis
        );

        // 4. Draw animated waves (emanating from center)
        if (InCurrent.Get_State() == ECk_AudioTrack_State::Playing ||
            InCurrent.Get_State() == ECk_AudioTrack_State::FadingIn)
        {
            const auto WaveSpeed = InCurrent.Get_State() == ECk_AudioTrack_State::FadingIn ? 3.0f : 2.0f;
            const auto CurrentTime = InDebug.Get_LastPulseTime().Get_Seconds();

            // Create multiple wave rings
            for (int32 WaveIndex = 0; WaveIndex < 3; ++WaveIndex)
            {
                const auto WaveOffset = WaveIndex * 0.5f; // Stagger waves
                const auto WaveProgress = FMath::Fmod(CurrentTime * WaveSpeed + WaveOffset, 1.0f);
                const auto WaveRadius = BoundaryRadius * WaveProgress;
                const auto WaveAlpha = 1.0f - WaveProgress; // Fade as it travels

                if (WaveRadius > 5.0f && WaveAlpha > 0.1f) // Only draw visible waves
                {
                    auto WaveColor = LinearColor::Teal;
                    WaveColor.A = WaveAlpha * 0.8f;

                    UCk_Utils_DebugDraw_UE::DrawDebugCircle(
                        World,
                        Position,
                        WaveRadius,
                        12, // segments
                        WaveColor,
                        0.0f,
                        LineThickness * 0.5f,
                        FVector(0.f, 1.f, 0.f),
                        FVector(0.f, 0.f, 1.f),
                        false
                    );
                }
            }
        }
        else if (InCurrent.Get_State() == ECk_AudioTrack_State::FadingOut)
        {
            // Slower, decelerating waves for fade out
            constexpr auto WaveSpeed = 1.0f;
            const auto CurrentTime = InDebug.Get_LastPulseTime().Get_Seconds();
            const auto WaveProgress = FMath::Fmod(CurrentTime * WaveSpeed, 1.0f);
            const auto WaveRadius = BoundaryRadius * WaveProgress;

            if (const auto WaveAlpha = (1.0f - WaveProgress) * 0.5f;
                WaveRadius > 5.0f && WaveAlpha > 0.1f)
            {
                auto WaveColor = LinearColor::Orange;
                WaveColor.A = WaveAlpha;

                UCk_Utils_DebugDraw_UE::DrawDebugCircle(
                    World,
                    Position,
                    WaveRadius,
                    12,
                    WaveColor,
                    0.0f,
                    LineThickness * 0.5f,
                    FVector(0.f, 1.f, 0.f),
                    FVector(0.f, 0.f, 1.f),
                    false
                );
            }
        }

        // 5. Draw track progress arc (if available)
        if (InDebug.Get_PlaybackPercent() > 0.0f && InDebug.Get_PlaybackPercent() <= 1.0f)
        {
            const auto ProgressRadius = BoundaryRadius + 10.0f;
            const auto ProgressAngle = 360.0f * InDebug.Get_PlaybackPercent();
            const auto NumProgressSegments = FMath::Max(4, (int32)(ProgressAngle / 10.0f));

            for (int32 I = 0; I < NumProgressSegments; ++I)
            {
                const auto Angle1 = FMath::DegreesToRadians((I * ProgressAngle) / NumProgressSegments - 90.0f); // Start at top
                const auto Angle2 = FMath::DegreesToRadians(((I + 1) * ProgressAngle) / NumProgressSegments - 90.0f);

                const auto Point1 = Position + FVector(
                    FMath::Cos(Angle1) * ProgressRadius,
                    FMath::Sin(Angle1) * ProgressRadius,
                    0.0f
                );
                const auto Point2 = Position + FVector(
                    FMath::Cos(Angle2) * ProgressRadius,
                    FMath::Sin(Angle2) * ProgressRadius,
                    0.0f
                );

                UCk_Utils_DebugDraw_UE::DrawDebugLine(
                    World, Point1, Point2, FLinearColor::White, 0.0f, LineThickness
                );
            }
        }

        // 6. Draw track name and info above the visualization
        const auto TextPosition = Position + FVector(0, 0, BoundaryRadius + 50.0f);
        const auto TrackInfo = ck::Format_UE(TEXT("{}\nVol: {:.2f} | Progress: {:.1f}%\nState: {}"),
            InParams.Get_TrackName().ToString(),
            InCurrent.Get_CurrentVolume(),
            InDebug.Get_PlaybackPercent() * 100.0f,
            InCurrent.Get_State());

        UCk_Utils_DebugDraw_UE::DrawDebugString(
            World,
            TextPosition,
            TrackInfo,
            nullptr,
            StateColor,
            0.0f
        );

        // 7. Optional: Fade direction indicator
        if (InCurrent.Get_State() == ECk_AudioTrack_State::FadingIn ||
            InCurrent.Get_State() == ECk_AudioTrack_State::FadingOut)
        {
            const auto FadeDirection = InCurrent.Get_State() == ECk_AudioTrack_State::FadingIn ?
                FVector::UpVector : FVector::DownVector;
            const auto ArrowStart = Position + FVector(0, 0, BoundaryRadius + 20.0f);
            const auto ArrowEnd = ArrowStart + (FadeDirection * 30.0f);

            UCk_Utils_DebugDraw_UE::DrawDebugArrow(
                World,
                ArrowStart,
                ArrowEnd,
                8.0f, // arrow size
                FLinearColor::Yellow,
                0.0f,
                LineThickness
            );
        }
    }

    static auto
        AudioTrack_DrawNonSpatialDebug(
            FCk_Handle_AudioTrack InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            const FFragment_AudioTrack_Debug& InDebug)
            -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Cannot draw non-spatial debug for AudioTrack [{}] - no valid world"), InHandle)
        { return; }

        auto DebugSubsystem = World->GetSubsystem<UCk_DebugDraw_Subsystem_UE>();
        CK_ENSURE_IF_NOT(ck::IsValid(DebugSubsystem), TEXT("DebugDraw subsystem not available"))
        { return; }

        const auto HUDSize = UCk_Utils_AudioTrack_Settings::Get_NonSpatialHUDSize();
        constexpr auto SlotHeight = 80.0f; // Increased from 70.0f to accommodate 4 lines of text
        constexpr auto StartY = 10.0f; // Start position from top of screen
        constexpr auto SlotX = 10.0f; // Left margin
        const auto ColumnWidth = HUDSize + 20.0f; // Width of each column including margin
        constexpr auto MaxItemsPerColumn = 8; // Maximum items per column before starting a new column

        // Calculate column and row for this slot
        const auto ColumnIndex = InDebug.Get_HUDSlotIndex() / MaxItemsPerColumn;
        const auto RowIndex = InDebug.Get_HUDSlotIndex() % MaxItemsPerColumn;

        // Calculate position for this slot
        const auto SlotY = StartY + (RowIndex * SlotHeight);
        const auto SlotXPos = SlotX + (ColumnIndex * ColumnWidth);
        const auto RectPosition = FVector2D(SlotXPos, SlotY);
        const auto RectSize = FVector2D(HUDSize, 75.0f); // Increased from 60.0f to 75.0f

        // Create pulsing effect by modulating the color alpha
        auto PulseColor = InDebug.Get_StateColor();
        PulseColor.A = InDebug.Get_CurrentPulseScale();

        // Draw background rect
        const auto BackgroundRect = FBox2D(RectPosition, RectPosition + RectSize);
        DebugSubsystem->Request_DrawRect_OnScreen(FCk_Request_DebugDrawOnScreen_Rect{BackgroundRect}
            .Set_RectColor(PulseColor * 0.3f)); // Dimmed background

        // Draw volume bar
        const auto VolumeBarWidth = RectSize.X * InCurrent.Get_CurrentVolume();
        const auto VolumeBarRect = FBox2D(
            RectPosition + FVector2D(5.0f, 5.0f),
            RectPosition + FVector2D(VolumeBarWidth - 5.0f, 15.0f)
        );

        if (VolumeBarWidth > 10.0f) // Only draw if there's something to show
        {
            DebugSubsystem->Request_DrawRect_OnScreen(FCk_Request_DebugDrawOnScreen_Rect{VolumeBarRect}
                .Set_RectColor(PulseColor));
        }

        // Draw border
        const auto LineColor = InDebug.Get_StateColor();
        const auto TopLeft = RectPosition;
        const auto TopRight = RectPosition + FVector2D(RectSize.X, 0);
        const auto BottomLeft = RectPosition + FVector2D(0, RectSize.Y);
        const auto BottomRight = RectPosition + RectSize;

        DebugSubsystem->Request_DrawLine_OnScreen(FCk_Request_DebugDrawOnScreen_Line{TopLeft, TopRight}
            .Set_LineColor(LineColor).Set_LineThickness(2.0f));
        DebugSubsystem->Request_DrawLine_OnScreen(FCk_Request_DebugDrawOnScreen_Line{TopRight, BottomRight}
            .Set_LineColor(LineColor).Set_LineThickness(2.0f));
        DebugSubsystem->Request_DrawLine_OnScreen(FCk_Request_DebugDrawOnScreen_Line{BottomRight, BottomLeft}
            .Set_LineColor(LineColor).Set_LineThickness(2.0f));
        DebugSubsystem->Request_DrawLine_OnScreen(FCk_Request_DebugDrawOnScreen_Line{BottomLeft, TopLeft}
            .Set_LineColor(LineColor).Set_LineThickness(2.0f));

        // Draw text on screen
        const auto TextColor = LineColor;
        const auto TextStartPos = RectPosition + FVector2D(5.0f, 20.0f); // Below the volume bar
        constexpr auto LineHeight = 14.0f;
        constexpr auto TextScale = 0.8f;

        // Track name
        const auto TrackNameText = InParams.Get_TrackName().ToString();
        DebugSubsystem->Request_DrawText_OnScreen(
            FCk_Request_DebugDrawOnScreen_Text{TextStartPos, TrackNameText}
            .Set_TextColor(TextColor)
            .Set_TextScale(TextScale)
        );

        // Volume text
        const auto VolumeText = ck::Format_UE(TEXT("Vol: {:.2f}"), InCurrent.Get_CurrentVolume());
        const auto VolumeTextPos = TextStartPos + FVector2D(0.0f, LineHeight);
        DebugSubsystem->Request_DrawText_OnScreen(
            FCk_Request_DebugDrawOnScreen_Text{VolumeTextPos, VolumeText}
            .Set_TextColor(TextColor)
            .Set_TextScale(TextScale)
        );

        // Progress text
        const auto ProgressText = ck::Format_UE(TEXT("Progress: {:.1f}%"), InDebug.Get_PlaybackPercent() * 100.0f);
        const auto ProgressTextPos = TextStartPos + FVector2D(0.0f, LineHeight * 2.0f);
        DebugSubsystem->Request_DrawText_OnScreen(
            FCk_Request_DebugDrawOnScreen_Text{ProgressTextPos, ProgressText}
            .Set_TextColor(TextColor)
            .Set_TextScale(TextScale)
        );

        // State text
        const auto StateText = ck::Format_UE(TEXT("State: {}"), InCurrent.Get_State());
        const auto StateTextPos = TextStartPos + FVector2D(0.0f, LineHeight * 3.0f);
        DebugSubsystem->Request_DrawText_OnScreen(
            FCk_Request_DebugDrawOnScreen_Text{StateTextPos, StateText}
            .Set_TextColor(TextColor)
            .Set_TextScale(TextScale)
        );
    }

    // Individual Debug Processors (only run when FTag_AudioTrack_DebugDraw is present)
    auto
        FProcessor_AudioTrack_DebugDraw_Individual_Spatial::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug,
            const FFragment_Transform& InTransform) const
        -> void
    {
        AudioTrack_UpdateDebugInfo(InHandle, InDeltaT, InCurrent, InDebug);
        AudioTrack_DrawSpatialDebug(InHandle, InParams, InCurrent, InDebug, InTransform.Get_Transform());
    }

    auto
        FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug) const
        -> void
    {
        // Update HUD slot for non-spatial tracks
        InDebug._HUDSlotIndex = _NonSpatialSlotCounter;
        _NonSpatialSlotCounter++;

        AudioTrack_UpdateDebugInfo(InHandle, InDeltaT, InCurrent, InDebug);
        AudioTrack_DrawNonSpatialDebug(InHandle, InParams, InCurrent, InDebug);
    }

    // Global Debug Processors (run when CVAR is enabled, on ALL tracks)
    auto
        FProcessor_AudioTrack_DebugDraw_All_Spatial::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        if (NOT UCk_Utils_AudioTrack_Settings::Get_DebugPreviewAllAudioTracks())
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_AudioTrack_DebugDraw_All_Spatial::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug,
            const FFragment_Transform& InTransform) const
        -> void
    {
        AudioTrack_UpdateDebugInfo(InHandle, InDeltaT, InCurrent, InDebug);
        AudioTrack_DrawSpatialDebug(InHandle, InParams, InCurrent, InDebug, InTransform.Get_Transform());
    }

    // Global Debug Processors (run when CVAR is enabled, on ALL tracks)
    auto
        FProcessor_AudioTrack_DebugDraw_All_NonSpatial::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        if (NOT UCk_Utils_AudioTrack_Settings::Get_DebugPreviewAllAudioTracks())
        { return; }

        // Collect all non-spatial tracks first for sorting
        _TracksToProcess.Reset();

        // First pass: collect all tracks
        _TransientEntity.View<FFragment_AudioTrack_Params, FFragment_AudioTrack_Current, FFragment_AudioTrack_Debug,
            TExclude<FFragment_Transform>, TExclude<FTag_AudioTrack_NeedsSetup>, CK_IGNORE_PENDING_KILL>().ForEach(
        [&](EntityType InEntity, const FFragment_AudioTrack_Params& InParams, const FFragment_AudioTrack_Current& InCurrent, FFragment_AudioTrack_Debug& InDebug)
        {
            _TracksToProcess.Emplace(FCk_Entity{InEntity});
        });

        // Sort tracks: Playing/FadingIn first (green states), then others
        _TracksToProcess.Sort([&](const FCk_Entity& A, const FCk_Entity& B) -> bool
        {
            auto HandleA = ck::MakeHandle(A, _TransientEntity);
            auto HandleB = ck::MakeHandle(B, _TransientEntity);

            if (NOT HandleA.Has<FFragment_AudioTrack_Current>() || NOT HandleB.Has<FFragment_AudioTrack_Current>())
            { return false; }

            const auto& StateA = HandleA.Get<FFragment_AudioTrack_Current>().Get_State();
            const auto& StateB = HandleB.Get<FFragment_AudioTrack_Current>().Get_State();

            // Priority order: Playing > FadingIn > FadingOut > Paused > Stopped
            const auto GetStatePriority = [](ECk_AudioTrack_State State) -> int32
            {
                switch (State)
                {
                    case ECk_AudioTrack_State::Playing: return 0;
                    case ECk_AudioTrack_State::FadingIn: return 1;
                    case ECk_AudioTrack_State::FadingOut: return 2;
                    case ECk_AudioTrack_State::Paused: return 3;
                    case ECk_AudioTrack_State::Stopped: return 4;
                    default: return 5;
                }
            };

            return GetStatePriority(StateA) < GetStatePriority(StateB);
        });

        // Reset slot counter
        _NonSpatialSlotCounter = 0;

        // Process sorted tracks
        for (const auto& Entity : _TracksToProcess)
        {
            auto Handle = ck::MakeHandle(Entity, _TransientEntity);
            const auto TypeSafeHandle = UCk_Utils_AudioTrack_UE::Cast(Handle);

            if (NOT Handle.Has<FFragment_AudioTrack_Params>() ||
                NOT Handle.Has<FFragment_AudioTrack_Current>() ||
                NOT Handle.Has<FFragment_AudioTrack_Debug>())
            { continue; }

            const auto& Params = Handle.Get<FFragment_AudioTrack_Params>();
            const auto& Current = Handle.Get<FFragment_AudioTrack_Current>();
            auto& Debug = Handle.Get<FFragment_AudioTrack_Debug>();

            // Update HUD slot for non-spatial tracks
            Debug._HUDSlotIndex = _NonSpatialSlotCounter;
            _NonSpatialSlotCounter++;

            AudioTrack_UpdateDebugInfo(TypeSafeHandle, InDeltaT, Current, Debug);
            AudioTrack_DrawNonSpatialDebug(TypeSafeHandle, Params, Current, Debug);
        }
    }

    auto
        FProcessor_AudioTrack_DebugDraw_All_NonSpatial::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioTrack_Params& InParams,
            const FFragment_AudioTrack_Current& InCurrent,
            FFragment_AudioTrack_Debug& InDebug) const
        -> void
    {
        // This function is now handled in DoTick() for sorting purposes
        // The actual processing happens in the custom DoTick() implementation above
    }
}

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
