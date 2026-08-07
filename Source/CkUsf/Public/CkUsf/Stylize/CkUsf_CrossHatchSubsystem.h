#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUsf/Stylize/CkUsf_CrossHatch_Params.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkUsf_CrossHatchSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_CrossHatchPreset;
class UMaterialInstanceDynamic;
class UPostProcessComponent;

// --------------------------------------------------------------------------------------------------------------------

// Per-world manager for the CkUsf CrossHatch look: owns the whole-view post-process blendable (a hidden
// transient actor carrying an unbound UPostProcessComponent, per UCkUsf_OutlineSubsystem) and one MID
// built from the generated CrossHatch master.
//
// The settings value is the source of truth; the MID is a projection of it, refreshed only for the
// fields that actually changed. Settings survive a missing master — a world whose CrossHatch master has
// not been generated yet still round-trips Get/Set, it simply renders nothing (warned once per world).
//
// The look is strictly full-screen apart from the EFFECT MASK, which reads Custom Stencil. That byte is
// shared with UCkUsf_OutlineSubsystem's allocated range and UCkUsf_CelShadeSubsystem's pattern span, so
// a settings value whose mask range intersects either is REJECTED — accepting it would restyle the wrong
// meshes with nothing on screen naming the cause.
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_Usf_CrossHatch")
class CKUSF_API UCkUsf_CrossHatchSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkUsf_CrossHatchSubsystem);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Get Cross Hatch Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkUsf_CrossHatchSubsystem*
    Get_CrossHatchSubsystem(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Request Set Cross Hatch Enabled")
    void
    Request_SetEnabled(
        ECk_EnableDisable InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Get Cross Hatch Is Enabled")
    ECk_EnableDisable
    Get_IsEnabled() const;

    // Replaces the whole settings value with the preset's. A null preset is rejected loudly and changes
    // nothing at all — no partial application. Note this sets the STORED settings; a
    // `ck.Usf.CrossHatch.*` console override still wins over the result until it is set back to -1.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Apply Cross Hatch Preset")
    void
    Apply_Preset(
        UCkUsf_CrossHatchPreset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Request Set Cross Hatch Settings")
    void
    Request_SetSettings(
        const FCk_Usf_CrossHatch_Params& InSettings);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Get Cross Hatch Settings")
    FCk_Usf_CrossHatch_Params
    Get_Settings() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|CrossHatch",
              DisplayName = "[Ck][Usf] Request Reset Cross Hatch To Defaults")
    void
    Request_ResetToDefaults();

private:
    auto DoEnsure_ViewEffect() -> bool;
    auto DoSync_ViewEffect() -> void;
    auto DoWrite_ChangedParams(const FCk_Usf_CrossHatch_Params& InEffective) -> void;

    // The stored settings with any `ck.Usf.CrossHatch.*` console override folded in. Everything that
    // reaches the MID goes through here, so an override is indistinguishable downstream from a setting.
    auto DoGet_EffectiveSettings() const -> FCk_Usf_CrossHatch_Params;

    auto DoResolve_ProjectDefaultPreset() const -> UCkUsf_CrossHatchPreset*;
    auto DoApply_ProjectDefault() -> void;
    auto DoApply_ProjectDefault_Now() -> void;
    auto DoOn_CVarChanged() -> void;

private:
    // Name of the look whose generated master backs this subsystem, and of the .ush entry point's
    // parameters — the writes in DoWrite_ChangedParams are positional in nothing but NAME, so a rename on
    // either side is a silent no-op until a gym shows it.
    static const FName kLookName;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _CrossHatchMID;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> _ViewPP;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _ViewActor;

    FCk_Usf_CrossHatch_Params _Settings;

    // What the MID currently holds — the EFFECTIVE value, not the stored one, so a console override that
    // changes nothing writes nothing. Unset until the first write, which is therefore a full write.
    TOptional<FCk_Usf_CrossHatch_Params> _WrittenSettings;

    FDelegateHandle _CVarChangedHandle;

    // Set by any explicit Request_SetSettings / Request_SetEnabled. The project default is a
    // DEFAULT, so it must never overwrite a value game code already chose.
    bool _SettingsExplicitlySet = false;

    // The missing-master warning is worth exactly one line per world, not one per settings change.
    bool _WarnedMissingMaster = false;
};
