#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"
#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Add(
        FCk_Handle InHandle,
        TSubclassOf<UCk_Fragment_ActorModifier_Rep> InFragmentClass)
    -> void
{
    // TODO: once this is solidified, the boilerplate will be reduced
    const auto& BasicDetails = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails(InHandle);

    if (BasicDetails.Get_Actor()->GetWorld()->IsNetMode(NM_Client))
    { return; }

    const auto OwningActor = BasicDetails.Get_Actor().Get();
    const auto OutermostActor =  UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    // TODO: ensure here that we do not have an outermost that is Replicated

    auto ReplicatedObject = Cast<UCk_Fragment_ActorModifier_Rep>(UCk_Ecs_ReplicatedObject::Create
    (
        InFragmentClass,
        OutermostActor,
        NAME_None,
        InHandle
    ));

    // HACK: This will be fixed when our Replicated Objects are NEVER created on Clients
    if (ck::Is_NOT_Valid(ReplicatedObject))
    { return; }

    InHandle.Add<TObjectPtr<UCk_Fragment_ActorModifier_Rep>>(ReplicatedObject);

    UCk_Utils_ReplicatedObjects_UE::Request_AddReplicatedObject(InHandle, ReplicatedObject);
}

auto
    UCk_Utils_ActorModifier_UE::
    Request_SetLocation(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetLocation& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    // TODO: once this is solidified, the boilerplate will be reduced
    const auto& BasicDetails = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails(InHandle);

    if (BasicDetails.Get_Actor()->GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        InHandle.Get<TObjectPtr<UCk_Fragment_ActorModifier_Rep>>()->Request_SetLocation(InRequest);
    }

    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_LocationRequests>();
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
    // TODO: once this is solidified, the boilerplate will be reduced
    const auto& BasicDetails = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails(InHandle);

    if (BasicDetails.Get_Actor()->GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        InHandle.Get<TObjectPtr<UCk_Fragment_ActorModifier_Rep>>()->Request_AddLocationOffset(InRequest);
    }

    if (NOT UCk_Utils_ActorInfo_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_LocationRequests>();
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

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_ScaleRequests>();
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

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_RotationRequests>();
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

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_RotationRequests>();
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

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_TransformRequests>();
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

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_SpawnActorRequests>();
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

    auto& requestsComp = InHandle.AddOrGet<ck::FCk_Fragment_ActorModifier_AddActorComponentRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }


    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------
