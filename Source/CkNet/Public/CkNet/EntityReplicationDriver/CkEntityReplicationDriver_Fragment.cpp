#include "CkEntityReplicationDriver_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/CkNet_Log.h"
#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _ReplicationData);
    DOREPLIFETIME(ThisType, _ReplicationData_ReplicatedActor);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData()
    -> void
{
    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData.Get_ReplicatedObjectsData().Get_Objects(), ck::algo::Is_NOT_Valid{}))
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    const auto OwningEntity = _ReplicationData.Get_OwningEntityDriver()->Get_AssociatedEntity();

    // wait on the owning entity to fully replicate
    if (ck::Is_NOT_Valid(OwningEntity))
    {
        _ReplicationData.Get_OwningEntityDriver()->_PendingChildEntityConstructions.Emplace(this);
        return;
    }

    // --------------------------------------------------------------------------------------------------------------------

    const auto& ReplicationData = _ReplicationData;
    const auto& ConstructionInfo = ReplicationData.Get_ConstructionInfo();

    const auto& ConstructionScript = ConstructionInfo.Get_ConstructionScript();
    CK_ENSURE_IF_NOT(ck::IsValid(ConstructionScript),
        TEXT("ConstructionScript is [{}]. Unable to proceed with replicating the Entity"), ConstructionScript)
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    const auto NewOrExistingEntity = [&]()
    {
        if (OwningEntity->IsValid(ConstructionInfo.Get_OriginalEntity()))
        {
            return ck::MakeHandle(ConstructionInfo.Get_OriginalEntity(), OwningEntity);
        }

        return UCk_Utils_EntityLifetime_UE::Request_CreateEntity(OwningEntity);
    }();

    ConstructionScript->Construct(NewOrExistingEntity);

    UCk_Utils_ReplicatedObjects_UE::Add(NewOrExistingEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(ReplicationData.Get_ReplicatedObjectsData().Get_Objects()));
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData_ReplicatedActor()
    -> void
{
    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData_ReplicatedActor.Get_ReplicatedObjects(), ck::algo::Is_NOT_Valid{}))
    { return; }

    auto ReplicatedActor = _ReplicationData_ReplicatedActor.Get_ReplicatedActor();

    const auto CsWithTransform = _ReplicationData_ReplicatedActor.Get_ConstructionScript();

    CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT valid. "
        "Unable to continue replication of Entity."),
        _ReplicationData_ReplicatedActor.Get_ConstructionScript(),
        ReplicatedActor)
    { return; }

    const auto WorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    auto       Entity         = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(WorldSubsystem->Get_TransientEntity());

    Entity.Add<ck::FFragment_OwningActor_Current>(ReplicatedActor);

    // TODO: we need the transform
    // CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());
    CsWithTransform->Construct(Entity);

    const auto& ReplicatedObjects = _ReplicationData_ReplicatedActor.Get_ReplicatedObjects();
    UCk_Utils_ReplicatedObjects_UE::Add(Entity, FCk_ReplicatedObjects{}.Set_ReplicatedObjects(ReplicatedObjects));


    // --------------------------------------------------------------------------------------------------------------------
    // Setup the EntityOwningActor component

    if (const auto EntityOwningActorComponent = ReplicatedActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
        ck::IsValid(EntityOwningActorComponent))
    {
        EntityOwningActorComponent->_EntityHandle = Entity;
    }
    else
    {
        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
            {
                ReplicatedActor,
                true
            },
            [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
            {
                InComp->_EntityHandle = Entity;
            }
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    // It's possible that some children are waiting on the parent to fully replicate
    for (const auto ChildRepDriver : _PendingChildEntityConstructions)
    {
        ChildRepDriver->OnRep_ReplicationData();
    }

    _PendingChildEntityConstructions.Reset();

    // --------------------------------------------------------------------------------------------------------------------

    ck::UUtils_Signal_OnReplicationComplete::Broadcast(Entity, ck::MakePayload(Entity));
}

// --------------------------------------------------------------------------------------------------------------------
