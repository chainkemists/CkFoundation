#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUsf_Stylize_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_CelShadePreset;
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

public:
    CK_PROPERTY_GET(_HandDrawnDefaultPreset);
    CK_PROPERTY_GET(_CelShadeDefaultPreset);
    CK_PROPERTY_GET(_ScreenDitherDefaultPreset);
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
};

// --------------------------------------------------------------------------------------------------------------------
