// ============================================================================
// Audio Director Config Asset - CkAudioDirector_Config.h
// ============================================================================

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkAudio/MusicLibrary/CkMusicLibrary_Asset.h"
#include "CkAudio/StingerLibrary/CkStingerLibrary_Asset.h"

#include <Engine/DataAsset.h>

#include "CkAudioDirector_Config.generated.h"

// ============================================================================

UCLASS(BlueprintType)
class CKAUDIO_API UCk_AudioDirector_Config : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AudioDirector_Config);

private:
    // Global Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global",
              meta = (AllowPrivateAccess = true))
    float _DefaultCrossfadeDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global",
              meta = (AllowPrivateAccess = true))
    int32 _MaxConcurrentTracks = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global",
              meta = (AllowPrivateAccess = true))
    bool _AllowSamePriorityTracks = false;

    // Libraries
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Libraries",
              meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UCk_MusicLibrary_Base>> _MusicLibraries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Libraries",
              meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UCk_StingerLibrary_Base>> _StingerLibraries;

    // EntityScript Support
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scripting",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;

public:
    CK_PROPERTY_GET(_DefaultCrossfadeDuration);
    CK_PROPERTY_GET(_MaxConcurrentTracks);
    CK_PROPERTY_GET(_AllowSamePriorityTracks);
    CK_PROPERTY_GET(_MusicLibraries);
    CK_PROPERTY_GET(_StingerLibraries);
    CK_PROPERTY_GET(_ScriptAsset);

public:
    // Library Lookup
    UFUNCTION(BlueprintPure, Category = "Audio Config")
    UCk_MusicLibrary_Base* Find_MusicLibrary(FGameplayTag InLibraryName) const;

    UFUNCTION(BlueprintPure, Category = "Audio Config")
    UCk_StingerLibrary_Base* Find_StingerLibrary(FGameplayTag InLibraryName) const;

    UFUNCTION(BlueprintPure, Category = "Audio Config")
    FCk_StingerEntry Get_StingerEntry(FGameplayTag InStingerName, bool& bFound) const;
};
