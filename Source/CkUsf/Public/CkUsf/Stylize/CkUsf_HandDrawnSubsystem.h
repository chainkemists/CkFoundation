#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUsf/Stylize/CkUsf_HandDrawn_Params.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkUsf_HandDrawnSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_HandDrawnPreset;
class UMaterialInstanceDynamic;
class UPostProcessComponent;

// --------------------------------------------------------------------------------------------------------------------

// Per-world manager for the CkUsf HandDrawn look: owns the whole-view post-process blendable (a hidden
// transient actor carrying an unbound UPostProcessComponent, per UCkUsf_OutlineSubsystem) and one MID
// built from the generated HandDrawn master.
//
// The settings value is the source of truth; the MID is a projection of it, refreshed only for the fields
// that actually changed. Settings survive a missing master — a world whose HandDrawn master has not been
// generated yet still round-trips Get/Set, it simply renders nothing (warned once per world).
//
// There is no per-object surface here, deliberately: the source feature is strictly full-screen and reads
// no Custom Depth/Stencil anywhere, so there is nothing for an entity API to address.
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_Usf_HandDrawn")
class CKUSF_API UCkUsf_HandDrawnSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkUsf_HandDrawnSubsystem);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Get Hand Drawn Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkUsf_HandDrawnSubsystem*
    Get_HandDrawnSubsystem(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Request Set Hand Drawn Enabled")
    void
    Request_SetEnabled(
        ECk_EnableDisable InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Get Hand Drawn Is Enabled")
    ECk_EnableDisable
    Get_IsEnabled() const;

    // Replaces the whole settings value with the preset's. A null preset is rejected loudly and changes
    // nothing at all — no partial application.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Apply Hand Drawn Preset")
    void
    Apply_Preset(
        UCkUsf_HandDrawnPreset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Request Set Hand Drawn Settings")
    void
    Request_SetSettings(
        const FCk_Usf_HandDrawn_Params& InSettings);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Get Hand Drawn Settings")
    FCk_Usf_HandDrawn_Params
    Get_Settings() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|HandDrawn",
              DisplayName = "[Ck][Usf] Request Reset Hand Drawn To Defaults")
    void
    Request_ResetToDefaults();

private:
    auto DoEnsure_ViewEffect() -> bool;
    auto DoSync_ViewEffect() -> void;
    auto DoWrite_ChangedParams() -> void;

private:
    // Name of the look whose generated master backs this subsystem, and of the .ush entry point's
    // parameters — the writes in DoWrite_ChangedParams are positional in nothing but NAME, so a rename on
    // either side is a silent no-op until a gym shows it.
    static const FName kLookName;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _HandDrawnMID;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> _ViewPP;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _ViewActor;

    FCk_Usf_HandDrawn_Params _Settings;

    // What the MID currently holds. Unset until the first write, which is therefore a full write.
    TOptional<FCk_Usf_HandDrawn_Params> _WrittenSettings;

    // The missing-master warning is worth exactly one line per world, not one per settings change.
    bool _WarnedMissingMaster = false;
};
