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
        const FCk_Targeter_BasicInfo& InTargeter,
        const FCk_Targeter_TargetList& InUnfilteredTargets) const
    -> FCk_Targeter_TargetList
{
    return InUnfilteredTargets;
}

auto
    UCk_Targeter_CustomTargetFilter_PDA::
    SortTargets_Implementation(
        const FCk_Targeter_BasicInfo& InTargeter,
        const FCk_Targeter_TargetList& InFilteredTargets) const
    -> FCk_Targeter_TargetList
{
    return InFilteredTargets;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Targeter_BasicInfo::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_Owner() == InOther.Get_Owner() && Get_Targeter() == InOther.Get_Targeter();
}

auto
    FCk_Targeter_BasicInfo::
    operator<(
        const ThisType& InOther) const
    -> bool
{
    return Get_Owner() < InOther.Get_Owner() && Get_Targeter() < InOther.Get_Targeter();
}

auto
    GetTypeHash(
        const FCk_Targeter_BasicInfo& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_Owner()) + GetTypeHash(InObj.Get_Targeter());
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
