#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkUsf_CelShadeSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_CelShadePreset;
class UMaterialInstanceDynamic;
class UPostProcessComponent;

// --------------------------------------------------------------------------------------------------------------------

// Per-world manager for the CkUsf CelShade look: owns the whole-view post-process blendable (a hidden
// transient actor carrying an unbound UPostProcessComponent, per UCkUsf_OutlineSubsystem) and one MID
// built from the generated CelShade master.
//
// The settings value is the source of truth; the MID is a projection of it, refreshed only for the fields
// that actually changed. Settings survive a missing master — a world whose CelShade master has not been
// generated yet still round-trips Get/Set, it simply renders nothing (warned once per world).
//
// Custom Stencil is SHARED state: this look claims [StencilBase - 1, StencilBase + 9] by direct value and
// UCkUsf_OutlineSubsystem claims its own allocated range. A settings value whose span intersects the
// outline range is REJECTED — accepting it would silently restyle outlined meshes (and vice versa) with
// nothing on screen naming the cause.
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_Usf_CelShade")
class CKUSF_API UCkUsf_CelShadeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkUsf_CelShadeSubsystem);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkUsf_CelShadeSubsystem*
    Get_CelShadeSubsystem(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Request Set Cel Shade Enabled")
    void
    Request_SetEnabled(
        ECk_EnableDisable InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Is Enabled")
    ECk_EnableDisable
    Get_IsEnabled() const;

    // Replaces the whole settings value with the preset's. A null preset is rejected loudly and changes
    // nothing at all — no partial application. Note this sets the STORED settings; a `ck.Usf.CelShade.*`
    // console override still wins over the result until it is set back to -1.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Apply Cel Shade Preset")
    void
    Apply_Preset(
        UCkUsf_CelShadePreset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Request Set Cel Shade Settings")
    void
    Request_SetSettings(
        const FCk_Usf_CelShade_Params& InSettings);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Settings")
    FCk_Usf_CelShade_Params
    Get_Settings() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Request Reset Cel Shade To Defaults")
    void
    Request_ResetToDefaults();

    // Whether InSettings' stencil span is usable at all AND can coexist with this world's outline
    // subsystem. Public because the entity-level cel-pattern API resolves stencil values against the same
    // contract and must not have to re-derive the rule.
    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Stencil Range Is Free")
    bool
    Get_StencilRangeIsFree(
        const FCk_Usf_CelShade_Params& InSettings) const;

    // Custom-Stencil value that selects InPattern on a mesh under the CURRENT settings, or 0 when the
    // stencil contract is disabled (0 is the engine's "no stencil written" value, so it is never a slot).
    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Stencil Value For Pattern")
    int32
    Get_StencilValueFor(
        ECk_Usf_CelPattern InPattern) const;

    // Custom-Stencil value that suppresses band transitions on a mesh, or 0 when the contract is disabled.
    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Stencil Suppress Value")
    int32
    Get_StencilSuppressValue() const;

    // Whether InSettings' custom band-edge list is usable: non-empty, strictly ascending, and every entry
    // strictly inside 0..1. Public for the same reason the stencil check is — an authoring tool can ask
    // before it hands a list over, instead of re-deriving the rule.
    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Band Edges Are Valid")
    static bool
    Get_BandEdgesAreValid(
        const FCk_Usf_CelShade_Params& InSettings);

private:
    // The two halves of Get_StencilRangeIsFree, kept apart so Request_SetSettings can name which one a
    // rejected settings value actually broke.
    static auto DoGet_StencilRangeIsAddressable(const FCk_Usf_CelShade_Params& InSettings) -> bool;
    auto DoGet_StencilRangeAvoidsOutline(const FCk_Usf_CelShade_Params& InSettings) const -> bool;

    auto DoEnsure_ViewEffect() -> bool;
    auto DoSync_ViewEffect() -> void;
    auto DoWrite_ChangedParams(const FCk_Usf_CelShade_Params& InEffective) -> void;

    // The stored settings with any `ck.Usf.CelShade.*` console override folded in. Everything that reaches
    // the MID goes through here, so an override is indistinguishable downstream from a setting.
    auto DoGet_EffectiveSettings() const -> FCk_Usf_CelShade_Params;

    auto DoResolve_ProjectDefaultPreset() const -> UCkUsf_CelShadePreset*;
    auto DoApply_ProjectDefault() -> void;
    auto DoApply_ProjectDefault_Now() -> void;
    auto DoOn_CVarChanged() -> void;

private:
    // Name of the look whose generated master backs this subsystem, and of the .ush entry point's
    // parameters — the writes in DoWrite_ChangedParams are positional in nothing but NAME, so a rename on
    // either side is a silent no-op until a gym shows it.
    static const FName kLookName;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _CelMID;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> _ViewPP;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _ViewActor;

    FCk_Usf_CelShade_Params _Settings;

    // What the MID currently holds — the EFFECTIVE value, not the stored one, so a console override that
    // changes nothing writes nothing. Unset until the first write, which is therefore a full write.
    TOptional<FCk_Usf_CelShade_Params> _WrittenSettings;

    FDelegateHandle _CVarChangedHandle;

    // Set by any explicit Request_SetSettings / Request_SetEnabled. The project default is a
    // DEFAULT, so it must never overwrite a value game code already chose.
    bool _SettingsExplicitlySet = false;

    // The missing-master warning is worth exactly one line per world, not one per settings change.
    bool _WarnedMissingMaster = false;
};
