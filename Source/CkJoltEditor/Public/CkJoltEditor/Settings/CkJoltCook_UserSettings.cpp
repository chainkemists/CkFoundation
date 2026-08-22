#include "CkJoltCook_UserSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltCook_UserSettings::
    Get_AutoCookMeshShapeOnAssetSave()
    -> ECk_EnableDisable
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_JoltCook_UserSettings_UE>()
        ->Get_AutoCookMeshShapeOnAssetSave();
}

auto
    UCk_Utils_JoltCook_UserSettings::
    Get_AutoCookStaticWorldOnLevelSave()
    -> ECk_EnableDisable
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_JoltCook_UserSettings_UE>()
        ->Get_AutoCookStaticWorldOnLevelSave();
}

auto
    UCk_Utils_JoltCook_UserSettings::
    Get_AutoCookDebounce()
    -> FCk_Time
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_JoltCook_UserSettings_UE>()
        ->Get_AutoCookDebounce();
}

// --------------------------------------------------------------------------------------------------------------------
