#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"
#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SetLocation(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetLocation& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_LocationRequests>();
    requestsComp._Requests.Emplace(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

auto
    UCk_Utils_ActorModifier_UE::
    Request_AddLocationOffset(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_AddLocationOffset& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_LocationRequests>();
    requestsComp._Requests.Emplace(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_ActorModifier_UE::
    Request_SetScale(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetScale& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_ScaleRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SetRotation(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetRotation& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_RotationRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_AddRotationOffset(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_AddRotationOffset& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_RotationRequests>();
    requestsComp._Requests.Emplace(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SetTransform(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetTransform& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_TransformRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SpawnActor(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SpawnActor& InRequest,
        const FCk_Delegate_ActorModifier_OnActorSpawned& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_SpawnActorRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_AddActorComponent(
        FCk_Handle                                              InHandle,
        const FCk_Request_ActorModifier_AddActorComponent&      InRequest,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& InDelegate)
    -> void
{
    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.Add<ck::FCk_Fragment_ActorModifier_AddActorComponentRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }


    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------
