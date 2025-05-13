#include "CkProbe_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Probe, TEXT("Probe"));

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

FCk_Request_Probe_OverlapUpdated::
FCk_Request_Probe_OverlapUpdated(
    FCk_Request_Probe_BeginOverlap InOther)
    : FCk_Request_Probe_BeginOverlap(MoveTemp(InOther))
{
}

// --------------------------------------------------------------------------------------------------------------------
