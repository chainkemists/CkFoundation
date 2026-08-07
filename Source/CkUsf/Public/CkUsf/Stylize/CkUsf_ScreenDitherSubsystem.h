#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUsf/Stylize/CkUsf_ScreenDither_Params.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkUsf_ScreenDitherSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_ScreenDitherPreset;
class UMaterialInstanceDynamic;
class UPostProcessComponent;

// --------------------------------------------------------------------------------------------------------------------

// Per-world manager for the CkUsf ScreenDither look: owns the whole-view post-process blendable (a hidden
// transient actor carrying an unbound UPostProcessComponent, per UCkUsf_OutlineSubsystem) and one MID
// built from the generated ScreenDither master.
//
// The settings value is the source of truth; the MID is a projection of it, refreshed only for the fields
// that actually changed. Settings survive a missing master — a world whose ScreenDither master has not
// been generated yet still round-trips Get/Set, it simply renders nothing (warned once per world).
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_Usf_ScreenDither")
class CKUSF_API UCkUsf_ScreenDitherSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkUsf_ScreenDitherSubsystem);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Get Screen Dither Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkUsf_ScreenDitherSubsystem*
    Get_ScreenDitherSubsystem(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Request Set Screen Dither Enabled")
    void
    Request_SetEnabled(
        ECk_EnableDisable InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Get Screen Dither Is Enabled")
    ECk_EnableDisable
    Get_IsEnabled() const;

    // Replaces the whole settings value with the preset's. A null preset is rejected loudly and changes
    // nothing at all — no partial application. Note this sets the STORED settings; a `ck.Usf.ScreenDither.*`
    // console override still wins over the result until it is set back to -1.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Apply Screen Dither Preset")
    void
    Apply_Preset(
        UCkUsf_ScreenDitherPreset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Request Set Screen Dither Settings")
    void
    Request_SetSettings(
        const FCk_Usf_ScreenDither_Params& InSettings);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Get Screen Dither Settings")
    FCk_Usf_ScreenDither_Params
    Get_Settings() const;

    // Back to the project default preset if one is configured, otherwise to the params-struct defaults.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|ScreenDither",
              DisplayName = "[Ck][Usf] Request Reset Screen Dither To Defaults")
    void
    Request_ResetToDefaults();

private:
    auto DoEnsure_ViewEffect() -> bool;
    auto DoSync_ViewEffect() -> void;
    auto DoWrite_ChangedParams(const FCk_Usf_ScreenDither_Params& InEffective) -> void;

    // The stored settings with any `ck.Usf.ScreenDither.*` console override folded in. Everything that
    // reaches the MID goes through here, so an override is indistinguishable downstream from a setting.
    auto DoGet_EffectiveSettings() const -> FCk_Usf_ScreenDither_Params;

    auto DoResolve_ProjectDefaultPreset() const -> UCkUsf_ScreenDitherPreset*;
    auto DoApply_ProjectDefault() -> void;
    auto DoApply_ProjectDefault_Now() -> void;
    auto DoOn_CVarChanged() -> void;

private:
    // Name of the look whose generated master backs this subsystem, and of the .ush entry point's
    // parameters — the writes in DoWrite_ChangedParams are positional in nothing but NAME, so a rename
    // on either side is a silent no-op until a gym shows it.
    static const FName kLookName;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _DitherMID;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> _ViewPP;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _ViewActor;

    FCk_Usf_ScreenDither_Params _Settings;

    // What the MID currently holds — the EFFECTIVE value, not the stored one, so a console override
    // that changes nothing writes nothing. Unset until the first write, which is therefore a full write.
    TOptional<FCk_Usf_ScreenDither_Params> _WrittenSettings;

    FDelegateHandle _CVarChangedHandle;

    // Set by any explicit Request_SetSettings / Request_SetEnabled. The project default is a
    // DEFAULT, so it must never overwrite a value game code already chose.
    bool _SettingsExplicitlySet = false;

    // The missing-master warning is worth exactly one line per world, not one per settings change.
    bool _WarnedMissingMaster = false;
};
