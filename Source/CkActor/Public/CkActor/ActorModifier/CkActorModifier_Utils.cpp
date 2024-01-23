#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

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

    RequestEntity.AddOrGet<ck::FFragment_ActorModifier_SpawnActorRequests>()._Requests.Add(InRequest);

    ck::UUtils_Signal_OnActorSpawned_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_AddActorComponent(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_AddActorComponent& InRequest,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ActorModifier_AddActorComponentRequests>()._Requests.Add(InRequest);

    ck::UUtils_Signal_OnActorComponentAdded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------
