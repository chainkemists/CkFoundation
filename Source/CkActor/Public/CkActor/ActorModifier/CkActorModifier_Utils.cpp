#pragma once

#include "CkActorModifier_Utils.h"

#include "CkActorModifier_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_SpawnActor_Replicated(
        UObject* InWorldContextObject,
        TSubclassOf<AActor> InActorClass,
        const FTransform& InSpawnTransform,
        ESpawnActorCollisionHandlingMethod InCollisionHandling,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorSpawned& OnActorSpawned,
        FName InNonUniqueActorName,
        FString InActorLabel)
    -> void
{
    const auto& TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorldContextObject->GetWorld());

    const auto Request = FCk_Request_ActorModifier_SpawnActor
    {
        FCk_Utils_Actor_SpawnActor_Params{InWorldContextObject, InActorClass}
        .Set_CollisionHandlingOverride(InCollisionHandling)
        .Set_SpawnTransform(InSpawnTransform)
        .Set_Label(InActorLabel)
        .Set_NonUniqueName(InNonUniqueActorName)
        .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated)
    };

    Request_SpawnActor(TransientEntity, Request, InOptionalPayload, OnActorSpawned);
}

auto
    UCk_Utils_ActorModifier_UE::
    Request_SpawnActor_NotReplicated(
        UObject* InWorldContextObject,
        TSubclassOf<AActor> InActorClass,
        const FTransform& InSpawnTransform,
        ESpawnActorCollisionHandlingMethod InCollisionHandling,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorSpawned& OnActorSpawned,
        FName InNonUniqueActorName,
        FString InActorLabel)
    -> void
{
    const auto& TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorldContextObject->GetWorld());

    const auto Request = FCk_Request_ActorModifier_SpawnActor
    {
        FCk_Utils_Actor_SpawnActor_Params{InWorldContextObject, InActorClass}
        .Set_CollisionHandlingOverride(InCollisionHandling)
        .Set_SpawnTransform(InSpawnTransform)
        .Set_Label(InActorLabel)
        .Set_NonUniqueName(InNonUniqueActorName)
        .Set_NetworkingType(ECk_Actor_NetworkingType::Local)
    };

    Request_SpawnActor(TransientEntity, Request, InOptionalPayload, OnActorSpawned);
}

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

    ck::UUtils_Signal_OnActorSpawned_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorModifier_UE::
    Request_AddActorComponent(
        UObject* InWorldContextObject,
        TSubclassOf<UActorComponent> InComponentClass,
        USceneComponent* InComponentParent,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& OnActorComponentAdded,
        FName InAttachmentSocket,
        ECk_ActorComponent_AttachmentPolicy InAttachmentPolicy,
        bool InIsUnique)
    -> void
{
    const auto& TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorldContextObject->GetWorld());

    const auto ComponentParams = FCk_AddActorComponent_Params{InComponentParent}
        .Set_AttachmentSocket(InAttachmentSocket)
        .Set_AttachmentType(InAttachmentPolicy);

    const auto Request = FCk_Request_ActorModifier_AddActorComponent{InComponentClass}
        .Set_IsUnique(InIsUnique)
        .Set_ComponentParams(ComponentParams);

    Request_AddActorComponent(TransientEntity, Request, InOptionalPayload, OnActorComponentAdded);
}

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

    ck::UUtils_Signal_OnActorComponentAdded_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------
