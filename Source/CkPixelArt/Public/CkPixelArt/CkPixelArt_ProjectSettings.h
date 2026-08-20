#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkPixelArt_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkPixelArt_Preset;

// --------------------------------------------------------------------------------------------------------------------

// Per-project default pixel-art style. The world subsystem reads this row once the world begins play and applies
// it, so a project can ship the look with no game code at all.
//
// UNSET means the renderer stays off — a soft ref rather than a preset plus an enable flag, because "no preset"
// and "disabled" would otherwise be two ways to say one thing and could disagree. Same rule and same reason as
// the Usf Stylize default-preset rows.
UCLASS(meta = (DisplayName = "Pixel Art"))
class CKPIXELART_API UCk_PixelArt_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PixelArt_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Default Preset",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCkPixelArt_Preset> _DefaultPreset;

public:
    CK_PROPERTY_GET(_DefaultPreset);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPIXELART_API UCk_Utils_PixelArt_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PixelArt_Settings_UE);

public:
    static const UCk_PixelArt_ProjectSettings_UE*
    Get();

    static TSoftObjectPtr<UCkPixelArt_Preset>
    Get_DefaultPreset();
};

// --------------------------------------------------------------------------------------------------------------------
