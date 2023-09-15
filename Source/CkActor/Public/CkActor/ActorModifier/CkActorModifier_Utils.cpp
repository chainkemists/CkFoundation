#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SpawnActor(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SpawnActor& InRequest,
        const FCk_Delegate_ActorModifier_OnActorSpawned& InDelegate)
    -> void
{
    if (NOT UCk_Utils_OwningActor_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.AddOrGet<ck::FFragment_ActorModifier_SpawnActorRequests>();
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
    if (NOT UCk_Utils_OwningActor_UE::Ensure(InHandle))
    { return; }

    auto& requestsComp = InHandle.AddOrGet<ck::FFragment_ActorModifier_AddActorComponentRequests>();
    requestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }


    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------
