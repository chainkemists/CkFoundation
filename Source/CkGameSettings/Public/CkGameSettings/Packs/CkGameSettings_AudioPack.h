#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include <Engine/World.h>

#include "CkGameSettings_AudioPack.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GameSettings_Subsystem_UE;
class USoundMix;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Apply target for ONE Audio-pack category: receives the category's Float volume and routes it
 * into SetSoundMixClassOverride on the pack's SoundMix (pushed once). Assets load synchronously at
 * first apply — this runs at boot/menu cadence, not per-frame. A volume applied before any world
 * exists (packaged boot replay) is held and re-applied on the owning GameInstance's first world.
 */
UCLASS(NotBlueprintable)
class CKGAMESETTINGS_API UCk_GameSettings_AudioCategoryHandler_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_AudioCategoryHandler_UE);

public:
    auto Initialize_Handler(const FCk_GameSettings_AudioCategory& InCategory, const TSoftObjectPtr<USoundMix>& InAudioMix) -> void;
    auto Shutdown_Handler() -> void;

public:
    UFUNCTION()
    void
    OnVolumeApplied(
        float InNewValue);

private:
    auto DoApplyVolume(float InVolume, UWorld* InWorld) -> void;
    auto DoHandleWorldInitialized(UWorld* InWorld, const UWorld::InitializationValues InValues) -> void;

private:
    FCk_GameSettings_AudioCategory _Category;
    TSoftObjectPtr<USoundMix> _AudioMix;
    TOptional<float> _PendingVolume;
    FDelegateHandle _WorldInitHandle;
    bool _MixPushed = false;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::game_settings
{
    /**
     * Registers one Float volume setting (Provider policy, Handler binding, range 0..1) per
     * configured audio category, plus its apply handler, through the subsystem's PUBLIC API only.
     * Idempotent per key. Created handler objects are appended to InOutOwnedObjects — the caller
     * owns their GC rooting. Returns how many categories were newly registered.
     */
    CKGAMESETTINGS_API auto
    RegisterAudioPack(
        UCk_GameSettings_Subsystem_UE& InSubsystem,
        const TSoftObjectPtr<USoundMix>& InAudioMix,
        const TArray<FCk_GameSettings_AudioCategory>& InCategories,
        TArray<TObjectPtr<UObject>>& InOutOwnedObjects) -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
