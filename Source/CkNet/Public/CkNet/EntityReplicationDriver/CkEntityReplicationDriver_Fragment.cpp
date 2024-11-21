#include "CkEntityReplicationDriver_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "Net/Core/PushModel/PushModel.h"

#include <Engine/World.h>

#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_Fragment_EntityReplicationDriver_Rep::
    UCk_Fragment_EntityReplicationDriver_Rep(
        const FObjectInitializer& InObjInitializer)
    : Super(InObjInitializer)
{
    if (IsTemplate())
    { return; }

    auto World = UObject::GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return; }

    auto EcsSubsystem = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(EcsSubsystem), TEXT("Ecs World Subsystem is NOT valid for world [{}]"),
        World)
    { return; }

    _AssociatedEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(**EcsSubsystem->Get_TransientEntity());
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Get_IsReplicationCompleteOnAllDependents() const
        -> bool
{
    if (const auto Entity = Get_AssociatedEntity();
        UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(Entity))
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

    constexpr auto Params = FDoRepLifetimeParams{COND_Custom, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData_Ability, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData_ReplicatedActor, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData_NonReplicatedActor, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ExpectedNumberOfDependentReplicationDrivers, Params);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Request_Replicate_NonReplicatedActor_Implementation(
        FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor InConstructionInfo)
    -> void
{
    const auto& ActorClass = InConstructionInfo.Get_NonReplicatedActor();
    const auto& Actor = UCk_Utils_Actor_UE::Request_SpawnActor(
        UCk_Utils_Actor_UE::SpawnActorParamsType{
            InConstructionInfo.Get_OuterReplicatedActor(), ActorClass
        }.Set_SpawnTransform(FTransform{InConstructionInfo.Get_StartingLocation()}
    ));

    auto Entity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(Actor);

    const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(Entity);
    InConstructionInfo.Set_ReplicatedObjects(ReplicatedObjects.Get_ReplicatedObjects());

    UCk_Utils_EntityReplicationDriver_UE::Request_ReplicateEntityOnNonReplicatedActor(Entity, InConstructionInfo);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    GetReplicatedCustomConditionState(
        FCustomPropertyConditionState& OutActiveState) const
    -> void
{
    Super::GetReplicatedCustomConditionState(OutActiveState);

    DOREPCUSTOMCONDITION_ACTIVE_FAST(ThisType, _ReplicationData, ck::IsValid(Get_AssociatedEntity()));
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

    CK_ENSURE_IF_NOT(ck::IsValid(_ReplicationData.Get_OwningEntityDriver()),
        TEXT("OwningEntityDriver is NOT valid. Somehow the ReplicationDriver was NOT added to the OwningEntity but WAS "
            "added to the child Entity.{}"), ck::Context(this))
    { return; }

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

    _AssociatedEntity._ReplicationDriver = this;

    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, OwningEntity);

    UCk_Utils_Net_UE::Add(_AssociatedEntity, FCk_Net_ConnectionSettings
    {
        ECk_Replication::Replicates,
        ECk_Net_NetModeType::Client,
        ECk_Net_EntityNetRole::Proxy
    });

    ConstructionScript->GetDefaultObject<UCk_Entity_ConstructionScript_PDA>()->Construct(
        _AssociatedEntity, ConstructionInfo.Get_OptionalParams());

    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(ReplicationData.Get_ReplicatedObjectsData().Get_Objects()));

    // --------------------------------------------------------------------------------------------------------------------

    // Make sure to call this on "self" since the # of dependent rep driver include "self" as well
    DoAdd_SyncedDependentReplicationDriver();

    // NOTE: The OwningEntity (and its driver) is only used when calling BuildAndReplicateEntity.
    // This implies that the owning entity is typically already replicated and should NOT have "this" replication driver as a dependent.
    // However, incrementing the count on the owning entity does NOT trigger the ensure check where #ExpectedDrivers > #SyncedDrivers.
    // This behavior may indicate a potential bug elsewhere in the system.
    _ReplicationData.Get_OwningEntityDriver()->DoAdd_SyncedDependentReplicationDriver();
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData_Ability()
    -> void
{
    // TODO: This is a temporary fix. We need to find a better way to handle this
    if (_AssociatedEntity.Has<ck::FFragment_LifetimeOwner>())
    { return; }

    // wait for the data to be fully replicated
    if (ck::Is_NOT_Valid(_ReplicationData_Ability.Get_AbilityScriptClass()))
    { return; }

    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData_Ability.Get_ReplicatedObjectsData().Get_Objects(), ck::algo::Is_NOT_Valid{}))
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    const auto OwningEntity = _ReplicationData_Ability.Get_OwningEntityDriver()->Get_AssociatedEntity();

    // wait on the owning entity to fully replicate
    if (ck::Is_NOT_Valid(OwningEntity))
    {
        _ReplicationData_Ability.Get_OwningEntityDriver()->_PendingChildAbilityEntityConstructions.Emplace(this);
        return;
    }

    ck::ecs::Verbose(TEXT("Adding Ability [{}] to [{}] with Owning Entity [{}] on Client [{}].{}"),
        _ReplicationData_Ability.Get_AbilityScriptClass(), Get_AssociatedEntity(), _ReplicationData_Ability.Get_OwningEntityDriver()->Get_AssociatedEntity(),
        GetWorld()->GetFirstLocalPlayerFromController(), this);

    // --------------------------------------------------------------------------------------------------------------------

    _AssociatedEntity._ReplicationDriver = this;

    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, OwningEntity);

    UCk_Utils_Net_UE::Add(_AssociatedEntity, FCk_Net_ConnectionSettings
    {
        ECk_Replication::Replicates,
        ECk_Net_NetModeType::Client,
        ECk_Net_EntityNetRole::Proxy
    });

    // For Abilities, we have to pass the information for construction to the Ability Processor. This will be removed once
    // the processor has had the chance to construct the Entity correctly
    _AssociatedEntity.Add<FCk_EntityReplicationDriver_AbilityData>(_ReplicationData_Ability);

    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(_ReplicationData_Ability.Get_ReplicatedObjectsData().Get_Objects()));

    // --------------------------------------------------------------------------------------------------------------------

    // NOTE: The #SyncedDrivers count is NOT incremented here. Instead, it is handled in the FProcessor_AbilityOwner_HandleRequests processor.
    // This ensures that the increment occurs only after the replicated ability has been created and assigned.
    // Incrementing prematurely would cause the ReplicationComplete and ReplicationCompleteAllDependents signals to fire too early.
    // If the replicated ability is added as an EntityExtension, any attempts to manipulate extended features (e.g., Attributes)
    // would fail because those features would not yet exist.
    //_ReplicationData_Ability.Get_OwningEntityDriver()->DoAdd_SyncedDependentReplicationDriver();
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

    _AssociatedEntity._ReplicationDriver = this;

    const auto WorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, WorldSubsystem->Get_TransientEntity());

    UCk_Utils_Handle_UE::Set_DebugName(_AssociatedEntity, UCk_Utils_Debug_UE::Get_DebugName(ReplicatedActor, ECk_DebugNameVerbosity_Policy::Compact));
    _AssociatedEntity.Add<ck::FFragment_OwningActor_Current>(ReplicatedActor);
    UCk_Utils_Net_UE::Add(_AssociatedEntity, FCk_Net_ConnectionSettings
        {
            ECk_Replication::Replicates,
            ECk_Net_NetModeType::Client,
            ReplicatedActor->GetLocalRole() == ROLE_AutonomousProxy ? ECk_Net_EntityNetRole::Authority : ECk_Net_EntityNetRole::Proxy
        });

    // TODO: we need the transform
    CsWithTransform->Set_EntityInitialTransform(ReplicatedActor->GetActorTransform());

    const auto& EntityBridgeActorComp = ReplicatedActor->GetComponentByClass<UCk_EntityBridge_ActorComponent_Base_UE>();

    EntityBridgeActorComp->TryInvoke_OnPreConstruct(_AssociatedEntity, UCk_EntityBridge_ActorComponent_Base_UE::EInvoke_Caller::ReplicationDriver);
    CsWithTransform->Construct(_AssociatedEntity, EntityBridgeActorComp->Get_EntityConstructionParamsToInject(), ReplicatedActor);

    const auto& ReplicatedObjects = _ReplicationData_ReplicatedActor.Get_ReplicatedObjects();
    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.Set_ReplicatedObjects(ReplicatedObjects));


    // --------------------------------------------------------------------------------------------------------------------
    // Setup the EntityOwningActor component

    if (const auto EntityOwningActorComponent = ReplicatedActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
        ck::IsValid(EntityOwningActorComponent))
    {
        EntityOwningActorComponent->_EntityHandle = _AssociatedEntity;
    }
    else
    {
        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
            {
                ReplicatedActor,
            },
            [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
            {
                InComp->_EntityHandle = _AssociatedEntity;
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

    // It's possible that some children are waiting on the parent to fully replicate
    for (const auto ChildRepDriver : _PendingChildAbilityEntityConstructions)
    {
        ChildRepDriver->OnRep_ReplicationData_Ability();
    }

    _PendingChildAbilityEntityConstructions.Reset();

    // --------------------------------------------------------------------------------------------------------------------

    ck::UUtils_Signal_OnReplicationComplete::Broadcast(_AssociatedEntity, ck::MakePayload(_AssociatedEntity));

    // --------------------------------------------------------------------------------------------------------------------

    DoAdd_SyncedDependentReplicationDriver();

    // --------------------------------------------------------------------------------------------------------------------

    EntityBridgeActorComp->TryInvoke_OnReplicationComplete(UCk_EntityBridge_ActorComponent_Base_UE::EInvoke_Caller::ReplicationDriver);
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

    // It's possible that some children are waiting on the parent to fully replicate
    for (const auto ChildRepDriver : _PendingChildAbilityEntityConstructions)
    {
        ChildRepDriver->OnRep_ReplicationData_Ability();
    }

    _PendingChildEntityConstructions.Reset();

    // --------------------------------------------------------------------------------------------------------------------

    ck::UUtils_Signal_OnReplicationComplete::Broadcast(Entity, ck::MakePayload(Entity));

    // --------------------------------------------------------------------------------------------------------------------

    DoAdd_SyncedDependentReplicationDriver();

    // --------------------------------------------------------------------------------------------------------------------

    const auto Actor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(Entity);

    // TODO: Ensure check this ptr
    const auto& EntityBridgeActorComp = Actor->GetComponentByClass<UCk_EntityBridge_ActorComponent_Base_UE>();
    EntityBridgeActorComp->TryInvoke_OnReplicationComplete(UCk_EntityBridge_ActorComponent_Base_UE::EInvoke_Caller::ReplicationDriver);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ExpectedNumberOfDependentReplicationDrivers() const
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
    ++_NumSyncedDependentReplicationDrivers;

    CK_ENSURE_IF_NOT(_NumSyncedDependentReplicationDrivers <= _ExpectedNumberOfDependentReplicationDrivers,
        TEXT("The number of Synced Dependent Replication Drivers [{}] is greater than the expected amount of [{}]. This is likely due to faulty logic."),
        _NumSyncedDependentReplicationDrivers,
        _ExpectedNumberOfDependentReplicationDrivers)
    { return; }

    if (_ExpectedNumberOfDependentReplicationDrivers == _NumSyncedDependentReplicationDrivers)
    {
        const auto AssociatedEntity = Get_AssociatedEntity();
        ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(AssociatedEntity, ck::MakePayload(AssociatedEntity));
    }
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData(
        const FCk_EntityReplicationDriver_ReplicationData& InReplicationData)
    -> void
{
    _ReplicationData = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData_Ability(
        const FCk_EntityReplicationDriver_AbilityData& InReplicationData)
    -> void
{
    _ReplicationData_Ability = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData_Ability, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData_ReplicatedActor(
        const FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor& InReplicationData)
    -> void
{
    _ReplicationData_ReplicatedActor = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData_ReplicatedActor, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData_NonReplicatedActor(
        const FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor& InReplicationData)
    -> void
{
    _ReplicationData_NonReplicatedActor = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData_NonReplicatedActor, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ExpectedNumberOfDependentReplicationDrivers(
        int32 InNumOfDependents)
    -> void
{
    _ExpectedNumberOfDependentReplicationDrivers = InNumOfDependents;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ExpectedNumberOfDependentReplicationDrivers, this);
}

// --------------------------------------------------------------------------------------------------------------------