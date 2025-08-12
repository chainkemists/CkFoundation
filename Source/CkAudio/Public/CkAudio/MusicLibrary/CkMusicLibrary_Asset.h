// ============================================================================
// Music Library Asset - CkMusicLibrary_Asset.h
// ============================================================================

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkAudio/AudioTrack/CkAudioTrack_Fragment_Data.h"

#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>
#include <Sound/SoundBase.h>

#include "CkMusicLibrary_Asset.generated.h"

// ============================================================================

UENUM(BlueprintType)
enum class ECk_MusicSelectionMode : uint8
{
    Random,
    WeightedRandom,
    Sequential,
    MoodBased
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MusicSelectionMode);

// ============================================================================

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_MusicTrackEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MusicTrackEntry);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundBase> _Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
              meta = (AllowPrivateAccess = true))
    float _Volume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection",
              meta = (AllowPrivateAccess = true))
    float _Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection",
              meta = (AllowPrivateAccess = true, Categories = "Audio.Mood"))
    TArray<FGameplayTag> _MoodTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    TOptional<bool> _OverrideLoop; // None = use library default

    // EntityScript Support
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scripting",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;

public:
    CK_PROPERTY(_Sound);
    CK_PROPERTY(_Volume);
    CK_PROPERTY(_Weight);
    CK_PROPERTY(_MoodTags);
    CK_PROPERTY(_OverrideLoop);
    CK_PROPERTY(_ScriptAsset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MusicTrackEntry, _Sound);
};

// ============================================================================

UCLASS(BlueprintType)
class CKAUDIO_API UCk_MusicLibrary_Base : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_MusicLibrary_Base);

private:
    // Identity
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity",
              meta = (AllowPrivateAccess = true, Categories = "Audio.Music"))
    FGameplayTag _LibraryName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity",
              meta = (AllowPrivateAccess = true))
    FString _Description;

    // Playback Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    ECk_AudioTrack_OverrideBehavior _OverrideBehavior = ECk_AudioTrack_OverrideBehavior::Crossfade;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    FCk_Time _DefaultFadeInTime = FCk_Time{3.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    FCk_Time _DefaultFadeOutTime = FCk_Time{2.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    bool _Loop = true;

    // Track Selection
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection",
              meta = (AllowPrivateAccess = true))
    ECk_MusicSelectionMode _SelectionMode = ECk_MusicSelectionMode::Random;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection",
              meta = (AllowPrivateAccess = true))
    float _RecentTrackAvoidanceTime = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection",
              meta = (AllowPrivateAccess = true, Categories = "Audio.Mood"))
    TArray<FGameplayTag> _ActiveMoodTags;

    // Music Tracks
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tracks",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_MusicTrackEntry> _Tracks;

    // EntityScript Support
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scripting",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;

public:
    CK_PROPERTY_GET(_LibraryName);
    CK_PROPERTY_GET(_Description);
    CK_PROPERTY_GET(_Priority);
    CK_PROPERTY_GET(_OverrideBehavior);
    CK_PROPERTY_GET(_DefaultFadeInTime);
    CK_PROPERTY_GET(_DefaultFadeOutTime);
    CK_PROPERTY_GET(_Loop);
    CK_PROPERTY_GET(_SelectionMode);
    CK_PROPERTY_GET(_RecentTrackAvoidanceTime);
    CK_PROPERTY_GET(_ActiveMoodTags);
    CK_PROPERTY_GET(_Tracks);
    CK_PROPERTY_GET(_ScriptAsset);

public:
    // Track Selection Logic
    UFUNCTION(BlueprintCallable, Category = "Music Library")
    int32 Get_NextTrack(const TArray<FGameplayTag>& InRecentTracks) const;

private:
    auto DoGet_NextTrack_Random() const -> int32;
    auto DoGet_NextTrack_WeightedRandom() const -> int32;
    auto DoGet_NextTrack_Sequential() const -> int32;
    auto DoGet_NextTrack_MoodBased(const TArray<FGameplayTag>& InRecentTracks) const -> int32;
};
