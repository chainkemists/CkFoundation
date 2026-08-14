#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include <GenericPlatform/GenericWindow.h>

#include "CkGameSettings_VideoPack.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GameSettings_Subsystem_UE;
class UGameUserSettings;

// --------------------------------------------------------------------------------------------------------------------

/**
 * External read/write-through accessors for every Video-pack setting. All values live in
 * UGameUserSettings (resolved EXCLUSIVELY through GEngine->GetGameUserSettings()) — the settings
 * store never holds a copy. Non-resolution setters ApplySettings + SaveSettings; resolution/mode
 * setters ApplyResolutionSettings + SaveSettings. Apply calls are suppressed (Display log) under
 * null-RHI/commandlet so headless automation stays green; the object writes and reads still work.
 */
UCLASS(NotBlueprintable)
class CKGAMESETTINGS_API UCk_GameSettings_VideoPackHandlers_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_VideoPackHandlers_UE);

public:
    UFUNCTION()
    int32 Get_WindowMode();
    UFUNCTION()
    void Set_WindowMode(int32 InNewValue);

    UFUNCTION()
    FString Get_Resolution();
    UFUNCTION()
    void Set_Resolution(const FString& InNewValue);

    UFUNCTION()
    bool Get_VSync();
    UFUNCTION()
    void Set_VSync(bool InNewValue);

    UFUNCTION()
    float Get_FpsCap();
    UFUNCTION()
    void Set_FpsCap(float InNewValue);

    UFUNCTION()
    int32 Get_QualityPreset();
    UFUNCTION()
    void Set_QualityPreset(int32 InNewValue);

    UFUNCTION()
    int32 Get_ViewDistanceQuality();
    UFUNCTION()
    void Set_ViewDistanceQuality(int32 InNewValue);

    UFUNCTION()
    int32 Get_AntiAliasingQuality();
    UFUNCTION()
    void Set_AntiAliasingQuality(int32 InNewValue);

    UFUNCTION()
    int32 Get_ShadowQuality();
    UFUNCTION()
    void Set_ShadowQuality(int32 InNewValue);

    UFUNCTION()
    int32 Get_PostProcessQuality();
    UFUNCTION()
    void Set_PostProcessQuality(int32 InNewValue);

    UFUNCTION()
    int32 Get_TextureQuality();
    UFUNCTION()
    void Set_TextureQuality(int32 InNewValue);

    UFUNCTION()
    int32 Get_EffectsQuality();
    UFUNCTION()
    void Set_EffectsQuality(int32 InNewValue);

    UFUNCTION()
    int32 Get_FoliageQuality();
    UFUNCTION()
    void Set_FoliageQuality(int32 InNewValue);

private:
    auto DoApplyNonResolution() -> void;
    auto DoApplyResolution() -> void;

    static auto DoGet_GameUserSettings() -> UGameUserSettings*;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::game_settings
{
    inline const FName Key_Video_WindowMode{TEXT("video.window_mode")};
    inline const FName Key_Video_Resolution{TEXT("video.resolution")};
    inline const FName Key_Video_VSync{TEXT("video.vsync")};
    inline const FName Key_Video_FpsCap{TEXT("video.fps_cap")};
    inline const FName Key_Video_QualityPreset{TEXT("video.quality_preset")};
    inline const FName Key_Video_Sg_ViewDistance{TEXT("video.sg.view_distance")};
    inline const FName Key_Video_Sg_AntiAliasing{TEXT("video.sg.anti_aliasing")};
    inline const FName Key_Video_Sg_Shadow{TEXT("video.sg.shadow")};
    inline const FName Key_Video_Sg_PostProcess{TEXT("video.sg.post_process")};
    inline const FName Key_Video_Sg_Texture{TEXT("video.sg.texture")};
    inline const FName Key_Video_Sg_Effects{TEXT("video.sg.effects")};
    inline const FName Key_Video_Sg_Foliage{TEXT("video.sg.foliage")};

    /** Every Video-pack key, in registration order. */
    CKGAMESETTINGS_API auto
    Get_VideoSettingKeys() -> const TArray<FName>&;

    /** "1920x1080" (case-insensitive separator) -> point; both dimensions must be positive integers. */
    CKGAMESETTINGS_API auto
    TryParse_Resolution(
        const FString& InResolution,
        FIntPoint& OutResolution) -> bool;

    CKGAMESETTINGS_API auto
    Format_Resolution(
        const FIntPoint& InResolution) -> FString;

    /** Int32 -> EWindowMode with the engine's clamping (out-of-range collapses to Windowed). */
    CKGAMESETTINGS_API auto
    Get_WindowModeFromInt(
        int32 InWindowMode) -> EWindowMode::Type;

    /** True when Apply/benchmark-style presentation work must be suppressed (null-RHI or commandlet). */
    CKGAMESETTINGS_API auto
    Get_IsHeadlessPresentation() -> bool;

    /**
     * Registers every Video-pack setting (all External policy, Handler binding) plus its
     * read/write-through accessor pair, through the subsystem's PUBLIC API only. Idempotent per
     * key. The one created handlers object is appended to InOutOwnedObjects — the caller owns its
     * GC rooting. Returns how many settings were newly registered.
     */
    CKGAMESETTINGS_API auto
    RegisterVideoPack(
        UCk_GameSettings_Subsystem_UE& InSubsystem,
        TArray<TObjectPtr<UObject>>& InOutOwnedObjects) -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
