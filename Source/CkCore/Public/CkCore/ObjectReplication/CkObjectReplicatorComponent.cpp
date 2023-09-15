#include "CkObjectReplicatorComponent.h"

#include "CkReplicatedObject.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_ObjectReplicator_ActorComponent_UE::
UCk_ObjectReplicator_ActorComponent_UE()
{
    PrimaryComponentTick.bCanEverTick = true;
    bReplicateUsingRegisteredSubObjectList = true;
    SetIsReplicatedByDefault(true);
}

auto UCk_ObjectReplicator_ActorComponent_UE::
Request_RegisterObjectForReplication(UCk_ReplicatedObject_UE* InObject) -> void
{
    // TODO: add some checks
    _ReplicatedObjects.Add(InObject);
    AddReplicatedSubObject(InObject);
}

auto UCk_ObjectReplicator_ActorComponent_UE::
Request_UnregisterObjectForReplication(
    UCk_ReplicatedObject_UE* InObject) -> void
{
    // TODO: add some checks
    _ReplicatedObjects.Remove(InObject);
    RemoveReplicatedSubObject(InObject);
}

// --------------------------------------------------------------------------------------------------------------------
