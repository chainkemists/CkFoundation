#pragma once

#include "CkCue/CkCueBase_EntityScript.h"
#include "CkAudioCue_Fragment_Data.h"
#include "CkAudio/AudioTrack/CkAudioTrack_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkAudioCue_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AudioCue_PlaybackBehavior : uint8
{
    AutoPlay,
    Manual,
    DelayedPlay
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class CKAUDIO_API UCk_AudioCue_EntityScript : public UCk_CueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AudioCue_EntityScript);

private:
    UCk_AudioCue_EntityScript(
        const FObjectInitializer& InObjectInitializer);

    // Source Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio Source",
              meta = (AllowPrivateAccess = true))
    ECk_AudioCue_SourcePriority _SourcePriority = ECk_AudioCue_SourcePriority::PreferSingleTrack;

    // Single Track Mode
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Single Track",
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_AudioTrack_ParamsData _SingleTrack;

    // Library Mode
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Library",
              meta = (AllowPrivateAccess = true, TitleProperty = "_TrackName"))
    TArray<FCk_Fragment_AudioTrack_ParamsData> _TrackLibrary;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Library",
              meta = (AllowPrivateAccess = true))
    ECk_AudioCue_SelectionMode _SelectionMode = ECk_AudioCue_SelectionMode::Random;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Library",
              meta = (AllowPrivateAccess = true))
    float _RecentTrackAvoidanceTime = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Library",
              meta = (AllowPrivateAccess = true, Categories = "Audio.Mood"))
    TArray<FGameplayTag> _ActiveMoodTags;

    // Director Features
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director",
              meta = (AllowPrivateAccess = true))
    FCk_Time _DefaultCrossfadeDuration = FCk_Time{2.0f};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director",
              meta = (AllowPrivateAccess = true))
    int32 _MaxConcurrentTracks = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director",
              meta = (AllowPrivateAccess = true))
    ECk_SamePriorityBehavior _SamePriorityBehavior = ECk_SamePriorityBehavior::Block;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    ECk_AudioCue_PlaybackBehavior _PlaybackBehavior = ECk_AudioCue_PlaybackBehavior::AutoPlay;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playback",
          meta = (AllowPrivateAccess = true, EditCondition = "_PlaybackBehavior == ECk_AudioCue_PlaybackBehavior::DelayedPlay"))
    FCk_Time _DelayTime = FCk_Time{3.0f};

public:
    CK_PROPERTY_GET(_SourcePriority);
    CK_PROPERTY_GET(_SingleTrack);
    CK_PROPERTY_GET(_TrackLibrary);
    CK_PROPERTY_GET(_SelectionMode);
    CK_PROPERTY_GET(_RecentTrackAvoidanceTime);
    CK_PROPERTY_GET(_ActiveMoodTags);
    CK_PROPERTY_GET(_DefaultCrossfadeDuration);
    CK_PROPERTY_GET(_MaxConcurrentTracks);
    CK_PROPERTY_GET(_SamePriorityBehavior);

    CK_PROPERTY_GET(_PlaybackBehavior);
    CK_PROPERTY_GET(_DelayTime);

    // Access to base cue lifetime behavior
    UFUNCTION(BlueprintPure, Category = "Audio Cue")
    ECk_Cue_LifetimeBehavior
    Get_LifetimeBehavior() const;

protected:
    auto Construct(FCk_Handle& InHandle, const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    UFUNCTION()
    void
    OnDelayTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT);

public:
    // Validation
    UFUNCTION(BlueprintPure, Category = "Audio Cue")
    bool Get_HasValidSingleTrack() const;

    UFUNCTION(BlueprintPure, Category = "Audio Cue")
    bool Get_HasValidTrackLibrary() const;

    UFUNCTION(BlueprintPure, Category = "Audio Cue")
    bool Get_IsConfigurationValid() const;

    // Track Selection (for library mode)
    UFUNCTION(BlueprintCallable, Category = "Audio Cue")
    int32 Get_NextTrackIndex(const TArray<FGameplayTag>& InRecentTracks) const;

private:
    auto DoGet_NextTrack_Random() const -> int32;
    auto DoGet_NextTrack_WeightedRandom() const -> int32;
    auto DoGet_NextTrack_Sequential() const -> int32;
    auto DoGet_NextTrack_MoodBased(const TArray<FGameplayTag>& InRecentTracks) const -> int32;

private:
    auto DoBindToAllTracksFinished(FCk_Handle_AudioCue InAudioCueHandle) -> void;

    UFUNCTION()
    void OnAllTracksFinished(FCk_Handle_AudioCue InAudioCue);
};

// --------------------------------------------------------------------------------------------------------------------
