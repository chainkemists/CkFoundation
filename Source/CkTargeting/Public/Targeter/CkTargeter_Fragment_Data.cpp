#include "CkTargeter_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Targeter_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_Targeter_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Targeter_CustomTargetFilter_PDA::
    FilterTargets_Implementation(
        const FCk_Handle_Targeter& InTargeter,
        const TArray<FCk_Handle_Targetable>& InUnfilteredTargets) const
    -> TArray<FCk_Handle_Targetable>
{
    return InUnfilteredTargets;
}

auto
    UCk_Targeter_CustomTargetFilter_PDA::
    SortTargets_Implementation(
        const FCk_Handle_Targeter& InTargeter,
        const TArray<FCk_Handle_Targetable>& InFilteredTargets) const
    -> TArray<FCk_Handle_Targetable>
{
    return InFilteredTargets;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Targeter_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_Targeter_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleTargeter_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleTargeter_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleTargeter_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleTargeter_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
