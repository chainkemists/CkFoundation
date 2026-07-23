#include "CkDialog_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Dialog_Settings_UE::
    Get()
    -> const UCk_Dialog_ProjectSettings_UE*
{
    return GetDefault<UCk_Dialog_ProjectSettings_UE>();
}

auto
    UCk_Utils_Dialog_Settings_UE::
    Get_DialogBanks()
    -> TArray<TSoftObjectPtr<UCk_DialogBank_DataAsset>>
{
    return Get()->Get_DialogBanks();
}

auto
    UCk_Utils_Dialog_Settings_UE::
    Get_QueryReadinessTimeoutSeconds()
    -> FCk_Time
{
    return Get()->Get_QueryReadinessTimeoutSeconds();
}

auto
    UCk_Utils_Dialog_Settings_UE::
    Get_DebugHistorySize()
    -> int32
{
    return Get()->Get_DebugHistorySize();
}

// --------------------------------------------------------------------------------------------------------------------
