#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SpawnActor(
        const FCk_Handle& InHandle,
        const FCk_Request_ActorModifier_SpawnActor& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorSpawned& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ActorModifier_SpawnActorRequests>()._Requests.Add(InRequest);
    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_OnActorSpawned, RequestEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_AddActorComponent(
        const FCk_Handle& InHandle,
        const FCk_Request_ActorModifier_AddActorComponent& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ActorModifier_AddActorComponentRequests>()._Requests.Add(InRequest);
    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_OnActorComponentAdded, RequestEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_RemoveActorComponent(
        const FCk_Handle& InHandle,
        const FCk_Request_ActorModifier_RemoveActorComponent& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorComponentRemoved& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_ActorModifier_RemoveActorComponentRequests>()._Request = InRequest;
    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_OnActorComponentRemoved, RequestEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
