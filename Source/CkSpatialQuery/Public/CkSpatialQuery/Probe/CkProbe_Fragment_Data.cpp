#include "CkProbe_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Probe_OverlapInfo::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_OtherEntity() == InOther.Get_OtherEntity();
}

auto
    GetTypeHash(
        const FCk_Probe_OverlapInfo& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_OtherEntity());
}

// --------------------------------------------------------------------------------------------------------------------
