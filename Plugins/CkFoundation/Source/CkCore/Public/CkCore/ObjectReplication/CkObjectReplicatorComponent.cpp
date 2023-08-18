#include "CkObjectReplicatorComponent.h"

#include "CkReplicatedObject.h"

UCk_ObjectReplicator_Component::
UCk_ObjectReplicator_Component()
{
    PrimaryComponentTick.bCanEverTick = true;
    bReplicateUsingRegisteredSubObjectList = true;
    SetIsReplicatedByDefault(true);
}

auto UCk_ObjectReplicator_Component::
Request_RegisterObjectForReplication(UCk_ReplicatedObject* InObject) -> void
{
    // TODO: add some checks
    _ReplicatedObjects.Add(InObject);
    AddReplicatedSubObject(InObject);
}

auto UCk_ObjectReplicator_Component::
Request_UnregisterObjectForReplication(
    UCk_ReplicatedObject* InObject) -> void
{
    // TODO: add some checks
    _ReplicatedObjects.Remove(InObject);
    RemoveReplicatedSubObject(InObject);
}
