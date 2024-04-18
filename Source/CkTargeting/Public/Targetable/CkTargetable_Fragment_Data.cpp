#include "CkTargetable_Fragment_Data.h"

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
    return Get_Targetable() < InOther.Get_Targetable();
}

auto
    GetTypeHash(
        const FCk_Targetable_BasicInfo& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_Owner()) + GetTypeHash(InObj.Get_Targetable());
}

// --------------------------------------------------------------------------------------------------------------------
