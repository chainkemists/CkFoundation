#include "CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkCore/Format/CkFormat_Defaults.h"

#include <Engine/World.h>
#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ReplicatedObject_UE::
    Setup(
        UCk_Ecs_ReplicatedObject_UE* InExistingReplicatedObject,
        AActor*                      InTopmostOwningActor,
        const FCk_Handle&            InAssociatedEntity) -> UCk_Ecs_ReplicatedObject_UE*
{
    InExistingReplicatedObject->_AssociatedEntity = InAssociatedEntity;
    InExistingReplicatedObject->_ReplicatedActor = InTopmostOwningActor;
    return InExistingReplicatedObject;
}

auto
    UCk_Ecs_ReplicatedObject_UE::
    Create(
        TSubclassOf<UCk_Ecs_ReplicatedObject_UE> InReplicatedObject,
        AActor* InTopmostOwningActor,
        FName InName,
        FCk_Handle InAssociatedEntity)
    -> UCk_Ecs_ReplicatedObject_UE*
{
    // TODO: this should be hidden in the base class
    auto* ObjectReplicator = InTopmostOwningActor->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(ObjectReplicator),
        TEXT("Expected ObjectReplicator to exist on [{}]. This component is automatically added on Replicated Actors that are ECS ready. "
            "Are you sure you added the EcsConstructionScript to the aforementioned Actor?"),
        InTopmostOwningActor)
    { return {}; }

    auto* Obj = NewObject<UCk_Ecs_ReplicatedObject_UE>(InTopmostOwningActor, InReplicatedObject, InName, RF_Public);
    Obj->_AssociatedEntity = InAssociatedEntity;
    Obj->_ReplicatedActor = InTopmostOwningActor;

    ObjectReplicator->Request_RegisterObjectForReplication(Obj);

    return Obj;
}

auto
    UCk_Ecs_ReplicatedObject_UE::
    Destroy(UCk_Ecs_ReplicatedObject_UE* InRo)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRo), TEXT("ReplicatedObject is [{}]. Unable to Destroy."), InRo)
    { return; }

    if (ck::Is_NOT_Valid(InRo))
    { return; }

    if (ck::Is_NOT_Valid(InRo->_ReplicatedActor))
    { return; }

    auto* ObjectReplicator = InRo->_ReplicatedActor->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>();

    if (ck::Is_NOT_Valid(ObjectReplicator))
    { return; }

    ObjectReplicator->Request_UnregisterObjectForReplication(InRo);
}

auto UCk_Ecs_ReplicatedObject_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _ReplicatedActor);
}

auto
    UCk_Ecs_ReplicatedObject_UE::
    BeginDestroy()
    -> void
{
    if (ck::IsValid(Get_ReplicatedActor()) && ck::IsValid(Get_ReplicatedActor()->GetWorld()))
    {
        if (NOT Get_ReplicatedActor()->GetWorld()->IsNetMode(NM_Client))
        {
            Destroy(this);
        }
    }

    Super::BeginDestroy();
}

void
    UCk_Ecs_ReplicatedObject_UE::
    PreDestroyFromReplication()
{
    Super::PreDestroyFromReplication();

    if (NOT ck::IsValid(Get_AssociatedEntity()))
    { return; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Get_AssociatedEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ReplicatedObjects::
    DoRequest_LinkAssociatedEntity(FCk_Handle InEntity)
    -> void
{
    for (const auto RO : _ReplicatedObjects)
    {
        const auto EcsReplicatedObject = Cast<UCk_Ecs_ReplicatedObject_UE>(RO);

        CK_ENSURE_IF_NOT(ck::IsValid(EcsReplicatedObject), TEXT("ReplicatedObject is not valid. Unable to Link with Entity [{}]"),
            InEntity)
        { continue; }

        EcsReplicatedObject->_AssociatedEntity = InEntity;
        EcsReplicatedObject->OnLink();
    }
}

// --------------------------------------------------------------------------------------------------------------------

