#include "CkAnimState_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimState_Goal::operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimGoal() == InOther.Get_AnimGoal();
}

auto
    GetTypeHash(
        const FCk_AnimState_Goal& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimGoal());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimState_Cluster::operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimCluster() == InOther.Get_AnimCluster();
}

auto
    GetTypeHash(
        const FCk_AnimState_Cluster& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimCluster());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimState_State::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimState() == InOther.Get_AnimState();
}

auto
    GetTypeHash(
        const FCk_AnimState_State& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimState());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimState_Overlay::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimOverlay() == InOther.Get_AnimOverlay();
}

auto
    GetTypeHash(
        const FCk_AnimState_Overlay& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimOverlay());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AnimState_Current::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_AnimState() == InOther.Get_AnimState() && Get_AnimCluster() == InOther.Get_AnimCluster() &&
           Get_AnimGoal() == InOther.Get_AnimGoal() && Get_AnimOverlay() == InOther.Get_AnimOverlay();
}

auto
    GetTypeHash(
        const FCk_AnimState_Current& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_AnimState()) + GetTypeHash(InObj.Get_AnimCluster()) + GetTypeHash(InObj.Get_AnimGoal()) +
           GetTypeHash(InObj.Get_AnimOverlay());
}

// --------------------------------------------------------------------------------------------------------------------
