#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUsf_Stylize_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_CelShadePreset;
class UCkUsf_CrossHatchPreset;
class UCkUsf_HandDrawnPreset;
class UCkUsf_ScreenDitherPreset;

// --------------------------------------------------------------------------------------------------------------------

// Per-project default style for each Stylize effect. Each world subsystem reads its own row once the
// world begins play and applies it, so a project can ship a look with no game code at all.
//
// An UNSET row means the effect stays at its params-struct defaults with the blendable disabled — i.e.
// off. That is why the rows are soft refs rather than a preset plus an enable flag: "no preset" and
// "disabled" would otherwise be two ways to say one thing, and they could disagree.
UCLASS(meta = (DisplayName = "Usf Stylize"))
class CKUSF_API UCk_Usf_Stylize_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Usf_Stylize_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Default Presets",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCkUsf_HandDrawnPreset> _HandDrawnDefaultPreset;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Default Presets",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCkUsf_CelShadePreset> _CelShadeDefaultPreset;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Default Presets",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCkUsf_ScreenDitherPreset> _ScreenDitherDefaultPreset;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Default Presets",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCkUsf_CrossHatchPreset> _CrossHatchDefaultPreset;

    // The ONE Custom-Stencil value UCk_Utils_Usf_StylizeMask_UE stamps on an entity. Project-wide rather
    // than per-effect on purpose: a masked entity is masked for whichever effects are configured to look
    // at it, so there is one byte value to reserve, not four. It must stay clear of the cel-pattern span
    // ([StencilBase-1, +9], 199-209 by default) and the outline range (240-255) — 190 sits below both.
    //
    // Changing it does NOT retune the effects: each effect's own mask RANGE is what decides whether this
    // value is inside the mask, and the shipped params default that range to [190, 190]. Move this and
    // every effect's range moves with it or the mask silently stops biting.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Effect Mask",
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 255, ClampMax = 255))
    int32 _MaskStencilValue = 190;

public:
    CK_PROPERTY_GET(_HandDrawnDefaultPreset);
    CK_PROPERTY_GET(_CelShadeDefaultPreset);
    CK_PROPERTY_GET(_ScreenDitherDefaultPreset);
    CK_PROPERTY_GET(_CrossHatchDefaultPreset);
    CK_PROPERTY_GET(_MaskStencilValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUSF_API UCk_Utils_Usf_Stylize_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Usf_Stylize_Settings_UE);

public:
    static const UCk_Usf_Stylize_ProjectSettings_UE*
    Get();

    static TSoftObjectPtr<UCkUsf_HandDrawnPreset>
    Get_HandDrawnDefaultPreset();

    static TSoftObjectPtr<UCkUsf_CelShadePreset>
    Get_CelShadeDefaultPreset();

    static TSoftObjectPtr<UCkUsf_ScreenDitherPreset>
    Get_ScreenDitherDefaultPreset();

    static TSoftObjectPtr<UCkUsf_CrossHatchPreset>
    Get_CrossHatchDefaultPreset();

    // The Custom-Stencil value the entity-level effect mask stamps. Reflected so a gym or a test can read
    // the configured value rather than restating 190.
    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Stylize",
              DisplayName = "[Ck][Usf] Get Stylize Mask Stencil Value")
    static int32
    Get_MaskStencilValue();
};

// --------------------------------------------------------------------------------------------------------------------
