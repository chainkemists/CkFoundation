#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Usf_Stylize_Settings_UE::
    Get()
    -> const UCk_Usf_Stylize_ProjectSettings_UE*
{
    return GetDefault<UCk_Usf_Stylize_ProjectSettings_UE>();
}

auto
    UCk_Utils_Usf_Stylize_Settings_UE::
    Get_HandDrawnDefaultPreset()
    -> TSoftObjectPtr<UCkUsf_HandDrawnPreset>
{
    return Get()->Get_HandDrawnDefaultPreset();
}

auto
    UCk_Utils_Usf_Stylize_Settings_UE::
    Get_CelShadeDefaultPreset()
    -> TSoftObjectPtr<UCkUsf_CelShadePreset>
{
    return Get()->Get_CelShadeDefaultPreset();
}

auto
    UCk_Utils_Usf_Stylize_Settings_UE::
    Get_ScreenDitherDefaultPreset()
    -> TSoftObjectPtr<UCkUsf_ScreenDitherPreset>
{
    return Get()->Get_ScreenDitherDefaultPreset();
}

auto
    UCk_Utils_Usf_Stylize_Settings_UE::
    Get_CrossHatchDefaultPreset()
    -> TSoftObjectPtr<UCkUsf_CrossHatchPreset>
{
    return Get()->Get_CrossHatchDefaultPreset();
}

auto
    UCk_Utils_Usf_Stylize_Settings_UE::
    Get_MaskStencilValue()
    -> int32
{
    return Get()->Get_MaskStencilValue();
}

// --------------------------------------------------------------------------------------------------------------------
