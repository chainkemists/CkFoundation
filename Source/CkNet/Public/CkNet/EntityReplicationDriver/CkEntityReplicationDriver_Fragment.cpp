#include "CkEntityReplicationDriver_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Get_IsReplicationCompleteOnAllDependents()
    -> bool
{
    const auto Entity = Get_AssociatedEntity();
    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(Entity))
    { return true; }

    return Get_ExpectedNumberOfDependentReplicationDrivers() == _NumSyncedDependentReplicationDrivers;
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _ReplicationData);
    DOREPLIFETIME(ThisType, _ReplicationData_ReplicatedActor);
    DOREPLIFETIME(ThisType, _ReplicationData_NonReplicatedActor);
    DOREPLIFETIME(ThisType, _ExpectedNumberOfDependentReplicationDrivers);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Request_Replicate_NonReplicatedActor_Implementation(
        FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor InConstructionInfo)
    -> void
{
    const auto    ActorClass = InConstructionInfo.Get_NonReplicatedActor();
    const AActor* Actor      = UCk_Utils_Actor_UE::Request_SpawnActor(
        UCk_Utils_Actor_UE::SpawnActorParamsType{
            InConstructionInfo.Get_OuterReplicatedActor(), ActorClass
        }.Set_SpawnTransform(FTransform{InConstructionInfo.Get_StartingLocation()}
    ));

    const auto Entity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(Actor);

    const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(Entity);
    InConstructionInfo.Set_ReplicatedObjects(ReplicatedObjects.Get_ReplicatedObjects());

    UCk_Utils_EntityReplicationDriver_UE::Request_ReplicateEntityOnNonReplicatedActor(Entity, InConstructionInfo);
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

        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(OwningEntity);

        if (ck::IsValid(ConstructionInfo.Get_Label()))
        { UCk_Utils_GameplayLabel_UE::Add(NewEntity, ConstructionInfo.Get_Label()); }

        ConstructionScript->GetDefaultObject<UCk_Entity_ConstructionScript_PDA>()->Construct(NewEntity);

        UCk_Utils_ReplicatedObjects_UE::Add(NewEntity, FCk_ReplicatedObjects{}.
            Set_ReplicatedObjects(ReplicationData.Get_ReplicatedObjectsData().Get_Objects()));

        return NewEntity;
    }();

    // --------------------------------------------------------------------------------------------------------------------

    _ReplicationData.Get_OwningEntityDriver()->DoAdd_SyncedDependentReplicationDriver();
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

    const auto CsWithTransform = Cast<UCk_Entity_ConstructionScript_WithTransform_PDA>(
        _ReplicationData_ReplicatedActor.Get_ConstructionScript());

    CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT valid. "
        "Unable to continue replication of Entity."),
        _ReplicationData_ReplicatedActor.Get_ConstructionScript(),
        ReplicatedActor)
    { return; }

    const auto WorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(WorldSubsystem->Get_TransientEntity(), [&](FCk_Handle InEntity)
    {
        InEntity.Add<ck::FFragment_OwningActor_Current>(ReplicatedActor);
        UCk_Utils_Net_UE::Add(InEntity, FCk_Net_ConnectionSettings
            {
                ECk_Net_NetModeType::Client,
                ReplicatedActor->GetLocalRole() == ROLE_AutonomousProxy ? ECk_Net_EntityNetRole::Authority : ECk_Net_EntityNetRole::Proxy
            });
    });

    // TODO: we need the transform
    CsWithTransform->Set_EntityInitialTransform(ReplicatedActor->GetActorTransform());
    CsWithTransform->Construct(NewEntity);

    const auto& ReplicatedObjects = _ReplicationData_ReplicatedActor.Get_ReplicatedObjects();
    UCk_Utils_ReplicatedObjects_UE::Add(NewEntity, FCk_ReplicatedObjects{}.Set_ReplicatedObjects(ReplicatedObjects));


    // --------------------------------------------------------------------------------------------------------------------
    // Setup the EntityOwningActor component

    if (const auto EntityOwningActorComponent = ReplicatedActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
        ck::IsValid(EntityOwningActorComponent))
    {
        EntityOwningActorComponent->_EntityHandle = NewEntity;
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
                InComp->_EntityHandle = NewEntity;
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

    ck::UUtils_Signal_OnReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));

    // --------------------------------------------------------------------------------------------------------------------

    DoAdd_SyncedDependentReplicationDriver();

    // --------------------------------------------------------------------------------------------------------------------

    ReplicatedActor->GetComponentByClass<UCk_EntityBridge_ActorComponent_Base_UE>()->
        TryInvoke_OnReplicationComplete(UCk_EntityBridge_ActorComponent_Base_UE::EInvoke_Caller::ReplicationDriver);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData_NonReplicatedActor() -> void
{
    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData_NonReplicatedActor.Get_ReplicatedObjects(), ck::algo::Is_NOT_Valid{}))
    { return; }

    const auto WorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    auto Entity = [&]() -> FCk_Handle
    {
        if (GetWorld()->GetFirstPlayerController() == _ReplicationData_NonReplicatedActor.Get_OwningPlayerController())
        {
            const auto TransientEntity = WorldSubsystem->Get_TransientEntity();
            const auto PotentiallyValidEntity = TransientEntity->Get_ValidEntity(
                static_cast<entt::entity>(_ReplicationData_NonReplicatedActor.Get_OriginalEntity()));

            CK_ENSURE_IF_NOT(TransientEntity->IsValid(PotentiallyValidEntity),
                TEXT("Expected the Entity [{}] to be valid since we have a valid PlayerController [{}]"),
                _ReplicationData_NonReplicatedActor.Get_OriginalEntity(),
                _ReplicationData_NonReplicatedActor.Get_OwningPlayerController())
            {
                return {};
            }

            return ck::MakeHandle(PotentiallyValidEntity, WorldSubsystem->Get_TransientEntity());
        }

        const auto    ActorClass = _ReplicationData_NonReplicatedActor.Get_NonReplicatedActor();
        const AActor* Actor      = UCk_Utils_Actor_UE::Request_SpawnActor(
            UCk_Utils_Actor_UE::SpawnActorParamsType{
                _ReplicationData_NonReplicatedActor.Get_OuterReplicatedActor(),
                ActorClass
            }.Set_SpawnTransform(
                FTransform{_ReplicationData_NonReplicatedActor.Get_StartingLocation()}
            ));
        return UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(Actor);
    }();

    const auto& ReplicatedObjects = _ReplicationData_NonReplicatedActor.Get_ReplicatedObjects();
    UCk_Utils_ReplicatedObjects_UE::Add(Entity, FCk_ReplicatedObjects{}.Set_ReplicatedObjects(ReplicatedObjects));

    // --------------------------------------------------------------------------------------------------------------------

    // It's possible that some children are waiting on the parent to fully replicate
    for (const auto ChildRepDriver : _PendingChildEntityConstructions)
    {
        ChildRepDriver->OnRep_ReplicationData();
    }

    _PendingChildEntityConstructions.Reset();

    // --------------------------------------------------------------------------------------------------------------------

    ck::UUtils_Signal_OnReplicationComplete::Broadcast(Entity, ck::MakePayload(Entity));

    // --------------------------------------------------------------------------------------------------------------------

    DoAdd_SyncedDependentReplicationDriver();

    // --------------------------------------------------------------------------------------------------------------------

    const auto Actor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(Entity);

    // TODO: Ensure check this ptr
    Actor->GetComponentByClass<UCk_EntityBridge_ActorComponent_Base_UE>()->
        TryInvoke_OnReplicationComplete(UCk_EntityBridge_ActorComponent_Base_UE::EInvoke_Caller::ReplicationDriver);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ExpectedNumberOfDependentReplicationDrivers()
    -> void
{
    if (_NumSyncedDependentReplicationDrivers == Get_ExpectedNumberOfDependentReplicationDrivers())
    {
        const auto AssociatedEntity = Get_AssociatedEntity();
        ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(AssociatedEntity, ck::MakePayload(AssociatedEntity));
    }
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    DoAdd_SyncedDependentReplicationDriver()
    -> void
{
    _NumSyncedDependentReplicationDrivers++;
    if (_ExpectedNumberOfDependentReplicationDrivers == _NumSyncedDependentReplicationDrivers)
    {
        const auto AssociatedEntity = Get_AssociatedEntity();
        ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(AssociatedEntity, ck::MakePayload(AssociatedEntity));
    }
}

// --------------------------------------------------------------------------------------------------------------------
