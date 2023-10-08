#include "CkObjectReplicatorComponent.h"

#include "CkCore/CkCore.h"

#include "CkReplicatedObject.h"

#include "CkCore/CkCoreLog.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_ObjectReplicator_ActorComponent_UE::
    UCk_ObjectReplicator_ActorComponent_UE()
{
    PrimaryComponentTick.bCanEverTick = true;
    bReplicateUsingRegisteredSubObjectList = true;
    SetIsReplicatedByDefault(true);
}

auto
    UCk_ObjectReplicator_ActorComponent_UE::
    Request_RegisterObjectForReplication(
        UCk_ReplicatedObject_UE* InObject)
    -> void
{
    // TODO: add some checks
    _ReplicatedObjects.Add(InObject);
    AddReplicatedSubObject(InObject);
}

auto
    UCk_ObjectReplicator_ActorComponent_UE::
    Request_UnregisterObjectForReplication(
        UCk_ReplicatedObject_UE* InObject)
    -> void
{
    // TODO: add some checks
    _ReplicatedObjects.Remove(InObject);
    RemoveReplicatedSubObject(InObject);
}

auto
    UCk_ObjectReplicator_ActorComponent_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _ReplicatedObjects);
}

auto
    UCk_ObjectReplicator_ActorComponent_UE::
    OnRep_ReplicatedObject()
    -> void
{
    UE_LOG(CkCore, Warning, TEXT("Object"));
}

// --------------------------------------------------------------------------------------------------------------------
