// ============================================================================
// Stinger Library Asset - CkStingerLibrary_Asset.h
// ============================================================================

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>
#include <Sound/SoundBase.h>

#include "CkStingerLibrary_Asset.generated.h"

// ============================================================================

UENUM(BlueprintType)
enum class ECk_SFXVoiceStealingMode : uint8
{
    KillOldest,
    KillNewest,
    KillLowestPriority,
    IgnoreNew
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SFXVoiceStealingMode);

UENUM(BlueprintType)
enum class ECk_SFXSameSourceBehavior : uint8
{
    KillOldest,
    KillNewest,
    Layer,
    Ignore
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SFXSameSourceBehavior);

// ============================================================================

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_StingerEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_StingerEntry);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity",
              meta = (AllowPrivateAccess = true, Categories = "Audio.SFX"))
    FGameplayTag _StingerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundBase> _Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
              meta = (AllowPrivateAccess = true))
    float _Volume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    float _Cooldown = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    ECk_SFXSameSourceBehavior _SameSourceBehavior = ECk_SFXSameSourceBehavior::KillOldest;

    // EntityScript Support
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scripting",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;

public:
    CK_PROPERTY_GET(_StingerName);
    CK_PROPERTY_GET(_Sound);
    CK_PROPERTY_GET(_Volume);
    CK_PROPERTY_GET(_Priority);
    CK_PROPERTY_GET(_Cooldown);
    CK_PROPERTY_GET(_SameSourceBehavior);
    CK_PROPERTY_GET(_ScriptAsset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_StingerEntry, _StingerName, _Sound);
};

// ============================================================================

UCLASS(BlueprintType)
class CKAUDIO_API UCk_StingerLibrary_Base : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_StingerLibrary_Base);

private:
    // Identity
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity",
              meta = (AllowPrivateAccess = true, Categories = "Audio.Stinger"))
    FGameplayTag _LibraryName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity",
              meta = (AllowPrivateAccess = true))
    FString _Description;

    // Playback Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    bool _InterruptAmbientMusic = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    int32 _MaxConcurrent = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    ECk_SFXVoiceStealingMode _VoiceStealing = ECk_SFXVoiceStealingMode::KillLowestPriority;

    // Stinger Entries
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stingers",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_StingerEntry> _Stingers;

    // EntityScript Support
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scripting",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;

public:
    CK_PROPERTY_GET(_LibraryName);
    CK_PROPERTY_GET(_Description);
    CK_PROPERTY_GET(_InterruptAmbientMusic);
    CK_PROPERTY_GET(_MaxConcurrent);
    CK_PROPERTY_GET(_VoiceStealing);
    CK_PROPERTY_GET(_Stingers);
    CK_PROPERTY_GET(_ScriptAsset);

public:
    // Stinger Lookup
    UFUNCTION(BlueprintPure, Category = "Stinger Library")
    FCk_StingerEntry Get_StingerEntry(FGameplayTag InStingerName, bool& bFound) const;

    UFUNCTION(BlueprintPure, Category = "Stinger Library")
    int32 Get_StingerIndex(FGameplayTag InStingerName) const;
};
