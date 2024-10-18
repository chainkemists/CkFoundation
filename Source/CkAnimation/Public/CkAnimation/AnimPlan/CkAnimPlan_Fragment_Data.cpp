#include "CkAnimPlan_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Label_AnimPlan_Goal, TEXT("AnimPlan.Goal"));
UE_DEFINE_GAMEPLAY_TAG(Tag_Label_AnimPlan_Cluster, TEXT("AnimPlan.Cluster"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Label_AnimPlan_State, TEXT("AnimPlan.State"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimPlan_Goal::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimGoal() == InOther.Get_AnimGoal();
}

auto
    GetTypeHash(
        const FCk_AnimPlan_Goal& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimGoal());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimPlan_Cluster::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimGoal() == InOther.Get_AnimGoal() &&
        Get_AnimCluster() == InOther.Get_AnimCluster();
}

auto
    GetTypeHash(
        const FCk_AnimPlan_Cluster& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimGoal()) +
        GetTypeHash(InObj.Get_AnimCluster());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimPlan_State::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimGoal() == InOther.Get_AnimGoal() &&
        Get_AnimCluster() == InOther.Get_AnimCluster() &&
        Get_AnimState() == InOther.Get_AnimState();
}

auto
    GetTypeHash(
        const FCk_AnimPlan_State& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimGoal()) +
        GetTypeHash(InObj.Get_AnimCluster()) +
        GetTypeHash(InObj.Get_AnimState());
}

// --------------------------------------------------------------------------------------------------------------------
