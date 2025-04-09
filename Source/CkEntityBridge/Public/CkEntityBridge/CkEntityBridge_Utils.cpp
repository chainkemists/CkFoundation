#include "CkEntityBridge_Utils.h"

#include "CkEntityBridge_ConstructionScript.h"
#include "CkEntityBridge_Fragment.h"
#include "CkNet_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkVariables/CkUnrealVariables_Utils.h"

UE_DISABLE_OPTIMIZATION

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityBridge_UE::
    Request_Spawn(
        const FCk_Handle& InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_EntityBridge_OnEntitySpawned& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_EntityBridge_Requests>()._Request = InRequest;
    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_OnEntitySpawned, RequestEntity, InDelegate);
}

auto
    UCk_Utils_EntityBridge_UE::
    Get_DoesActorEntityReplicate(
        AActor* InActor)
    -> ECk_Utils_Net_ActorEntityReplicationStatus
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Actor [{}] is INVALID"), InActor)
    { return ECk_Utils_Net_ActorEntityReplicationStatus::IsNotEcsReady; }

    const auto& ActorComponent = InActor->GetComponentByClass<UCk_EntityBridge_ActorComponent_UE>();
    if (ck::Is_NOT_Valid(ActorComponent))
    { return ECk_Utils_Net_ActorEntityReplicationStatus::IsNotEcsReady; }

    switch(ActorComponent->Get_Replication())
    {
        case ECk_Replication::Replicates: return ECk_Utils_Net_ActorEntityReplicationStatus::Replicates;
        case ECk_Replication::DoesNotReplicate: return ECk_Utils_Net_ActorEntityReplicationStatus::DoesNotReplicate;
    }

    return ECk_Utils_Net_ActorEntityReplicationStatus::IsNotEcsReady;
}

// --------------------------------------------------------------------------------------------------------------------
