#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
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
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    auto& RequestsComp = RequestEntity.AddOrGet<ck::FFragment_ActorModifier_SpawnActorRequests>();
    RequestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }

    ck::UUtils_Signal_OnActorSpawned_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
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

    auto& RequestsComp = InHandle.AddOrGet<ck::FFragment_ActorModifier_AddActorComponentRequests>();
    RequestsComp._Requests.Add(InRequest);

    if (NOT InDelegate.IsBound())
    { return; }


    // TODO: Allow binding to the internal signal
}

// --------------------------------------------------------------------------------------------------------------------
