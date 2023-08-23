#include "CkReplicatedObjects_Fragment_Params.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ReplicatedObject::
    Create(
        TSubclassOf<UCk_Ecs_ReplicatedObject> InReplicatedObject,
        AActor* InTopmostOwningActor,
        FName InName,
        FCk_Handle InAssociatedEntity)
    -> UCk_Ecs_ReplicatedObject*
{
    // TODO: this should be hidden in the base class
    auto* ObjectReplicator = InTopmostOwningActor->GetComponentByClass<UCk_ObjectReplicator_Component>();

    CK_ENSURE_IF_NOT(ck::IsValid(ObjectReplicator),
        TEXT("Expected ObjectReplicator to exist on [{}]. This component is automatically added on Replicated Actors that are ECS ready. Are you sure you added the EcsConstructionScript to the aforementioned Actor?"),
        InTopmostOwningActor)
    { return {}; }

    auto* Obj = NewObject<UCk_Ecs_ReplicatedObject>(InTopmostOwningActor, InReplicatedObject, InName, RF_Public);
    Obj->_AssociatedEntity = InAssociatedEntity;
    Obj->_ReplicatedActor = InTopmostOwningActor;

    ObjectReplicator->Request_RegisterObjectForReplication(Obj);

    return Obj;
}

auto UCk_Ecs_ReplicatedObject::
    OnRep_ReplicatedActor(AActor* InActor)
    -> void
{
    // 1 - Replicate Both the Actor and the Entity ID
    // 2 - The Actor is used to differentiate between an existing Object on the original Client vs other Clients
    // 3 - The Entity ID is then used to match this object with the original Object on the Client
    // 4 - All other clients create their respective Entities OR attach to an existing one
    // 5 - The remote clients use the original client's Entity ID to match Entity on their side OR create a new one
    // 6 - You'll probably need Tags on Entities such as FTag_Replicated, FTag_Authority (where a Client can also
    // have Authority over its Entity but Server and remote Clients do not)
    // 7 - Because we have multiple replicated objects, it would be a good idea to start replication using a
    // construction script
    // IN FACT THATS WHERE ALL OF THIS SHOULD HAPPEN i.e. when a Replicated Entity on a Client is created, an
    // RPC is sent to the Server with the Construction Script and the Client's Entity ID. This should allow us
    // to keep everything in Sync. We don't add replicated Fragments AFTER the fact.
}

auto UCk_Ecs_ReplicatedObject::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisType, _ReplicatedActor, COND_None, REPNOTIFY_Always);
}

auto
    FCk_ReplicatedObjects::
    DoRequest_LinkAssociatedEntity(FCk_Handle InEntity)
    -> void
{
    for (const auto RO : _ReplicatedObjects)
    {
        const auto EcsReplicatedObject = Cast<UCk_Ecs_ReplicatedObject>(RO);
        EcsReplicatedObject->_AssociatedEntity = InEntity;
    }
}

// --------------------------------------------------------------------------------------------------------------------

