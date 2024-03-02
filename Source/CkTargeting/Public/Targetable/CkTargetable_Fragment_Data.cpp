#include "CkTargetable_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Targetable_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_Targetable_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Targetable_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_Targetable_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleTargetable_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleTargetable_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleTargetable_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleTargetable_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Targetable_BasicInfo::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_Owner() == InOther.Get_Owner() && Get_Targetable() == InOther.Get_Targetable();
}

auto
    FCk_Targetable_BasicInfo::
    operator<(
        const ThisType& InOther) const
    -> bool
{
    return Get_Owner() < InOther.Get_Owner() && Get_Targetable() < InOther.Get_Targetable();
}

auto
    GetTypeHash(
        const FCk_Targetable_BasicInfo& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_Owner()) + GetTypeHash(InObj.Get_Targetable());
}

// --------------------------------------------------------------------------------------------------------------------
