#include "CkTargeter_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Targeter, TEXT("Targeter"));

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
    return Get_Targeter() < InOther.Get_Targeter();
}

auto
    GetTypeHash(
        const FCk_Targeter_BasicInfo& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_Owner()) + GetTypeHash(InObj.Get_Targeter());
}

// --------------------------------------------------------------------------------------------------------------------
