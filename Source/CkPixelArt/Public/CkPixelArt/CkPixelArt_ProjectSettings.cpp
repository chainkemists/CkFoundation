#include "CkPixelArt/CkPixelArt_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PixelArt_Settings_UE::
    Get()
    -> const UCk_PixelArt_ProjectSettings_UE*
{
    return GetDefault<UCk_PixelArt_ProjectSettings_UE>();
}

auto
    UCk_Utils_PixelArt_Settings_UE::
    Get_DefaultPreset()
    -> TSoftObjectPtr<UCkPixelArt_Preset>
{
    const auto* Settings = Get();

    if (Settings == nullptr)
    { return {}; }

    return Settings->Get_DefaultPreset();
}
