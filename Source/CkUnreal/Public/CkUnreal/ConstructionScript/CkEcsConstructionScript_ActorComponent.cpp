#include "CkEcsConstructionScript_ActorComponent.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/CkNet_Fragment.h"
#include "CkNet/CkNet_Log.h"
#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Subsystem.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"

#include "Engine/World.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_EcsConstructionScript_ActorComponent_UE::
    UCk_EcsConstructionScript_ActorComponent_UE()
{
    PrimaryComponentTick.bCanEverTick = false;
    bReplicateUsingRegisteredSubObjectList = true;
    SetIsReplicatedByDefault(true);
}

//auto
//    UCk_EcsConstructionScript_ActorComponent_UE::
//    Request_ReplicateActor_OnServer_Implementation(
//        const FCk_EcsConstructionScript_ConstructionInfo& InRequest)
//    -> void
//{
//    const auto& OutermostActor = InRequest.Get_OutermostActor();
//    const auto& ActorToReplicate = InRequest.Get_ActorToReplicate();
//
//    const auto NewOrExistingActor = [&]() -> TObjectPtr<AActor>
//    {
//        // if Outermost and ActorToReplicate are the same, the Actor is already replicated. No need
//        // to spawn another one. Simply return the Outermost.
//        if (InRequest.Get_OutermostActor()->GetClass() == InRequest.Get_ActorToReplicate())
//        {
//            return InRequest.Get_OutermostActor();
//        }
//
//        return UCk_Utils_Actor_UE::Request_SpawnActor
//        (
//            UCk_Utils_Actor_UE::SpawnActorParamsType{OutermostActor, ActorToReplicate}
//                .Set_SpawnTransform(InRequest.Get_Transform())
//        );
//    }();
//
//    CK_ENSURE_IF_NOT(ck::IsValid(NewOrExistingActor),
//        TEXT("Failed to spawn Actor to Replicate [{}] on SERVER.[{}]"), ActorToReplicate, ck::Context(this))
//    { return; }
//
//    const auto& NewActorBasicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(NewOrExistingActor);
//    const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewActorBasicDetails.Get_Handle());
//
//    auto ReplicatedObjectsData = FCk_EcsConstructionScript_ReplicateObjects_Data{OutermostActor};
//
//    for (const auto& ReplicatedObject : ReplicatedObjects.Get_ReplicatedObjects())
//    {
//        CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObject), TEXT("Invalid Replicated Object encountered for Actor [{}] on the SERVER.[{}]"), NewActorBasicDetails, ck::Context(this))
//        { continue; }
//
//        ReplicatedObjectsData.Get_Objects().Emplace(ReplicatedObject->GetClass());
//        ReplicatedObjectsData.Get_NetStableNames().Emplace(ReplicatedObject->GetFName());
//    }
//
//    _ReplicationData = FCk_EcsConstructionScript_Replication_Data
//    {
//        InRequest,
//        ReplicatedObjectsData
//    };
//
//    ck::net::Verbose(TEXT("Replicating [{}] with outermost [{}]"), OutermostActor, ActorToReplicate);
//}

//auto
//    UCk_EcsConstructionScript_ActorComponent_UE::
//    Request_ReplicateActor_OnClients(
//        const FCk_EcsConstructionScript_ReplicateObjects_Data& InData,
//        const FCk_EcsConstructionScript_ConstructionInfo& InRequest)
//    -> void
//{
//    // --------------------------------------------------------------------------------------------------------------------
//
//    FCk_ReplicatedObjects ROs;
//
//    CK_ENSURE_IF_NOT(InData.Get_Objects().Num() == InData.Get_NetStableNames().Num(),
//        TEXT("Expected Objects Array to be the same size as the associated Names Array. "
//            "Unable to proceed with construction of Replicated Objects.[{}]"), ck::Context(this))
//    { return; }
//
//    CK_ENSURE_IF_NOT(ck::IsValid(InData.Get_Owner()),
//        TEXT("The ReplicatedOwner is [{}]. Unable to proceed with construction of Replicated Objects.[{}]"), ck::Context(this))
//    { return; }
//
//    if (InData.Get_Owner()->IsNetMode(NM_DedicatedServer))
//    { return; }
//
//    ck::algo::ForEachView(InData.Get_Objects(), InData.Get_NetStableNames(),
//    [&](TSubclassOf<UCk_Ecs_ReplicatedObject_UE> InRepObjectClass, FName InNetStableName)
//    {
//        CK_ENSURE_IF_NOT(ck::IsValid(InData.Get_Owner()),
//            TEXT("Invalid Replicated Owner Actor.[{}]"), ck::Context(this))
//        { return; }
//
//        // TODO: Sending garbage entity handle until we manage to link it up properly
//        ROs.Update_ReplicatedObjects([&](TArray<UCk_ReplicatedObject_UE*>& InROs)
//        {
//            if (InRepObjectClass == UCk_Fragment_EntityReplicationDriver_Rep::StaticClass())
//            {
//                auto ExistingObject = GetWorld()->GetSubsystem<UCk_EntityReplicationDriver_Subsystem_UE>()
//                    ->Get_ReplicationDriverWithName(InNetStableName);
//                InROs.Emplace(ExistingObject);
//                return;
//            }
//
//            InROs.Emplace(UCk_Ecs_ReplicatedObject_UE::Create(InRepObjectClass, InData.Get_Owner(), InNetStableName, FCk_Handle{}));
//        });
//    });
//
//    // --------------------------------------------------------------------------------------------------------------------
//
//    const auto& OutermostActor = InRequest.Get_OutermostActor();
//    CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor), TEXT("Invalid Outermose Actor on CLIENT.[{}]"), ck::Context(this))
//    { return; }
//
//    // TODO: Fix this so that we don't need this check
//    if (OutermostActor->IsNetMode(NM_DedicatedServer))
//    { return; }
//
//    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
//    { return; }
//
//    const auto WorldSubsystem = Cast<UCk_EcsWorld_Subsystem_UE>(GetWorld()->GetSubsystemBase(_EcsWorldSubsystem));
//
//    // TODO: this hits at least once because the BP Subsystem is not loaded. Fix this.
//    CK_ENSURE_IF_NOT(ck::IsValid(WorldSubsystem),
//        TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"),
//        _EcsWorldSubsystem, ck::Context(this))
//    { return; }
//
//    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
//    {
//        const auto& PlayerController = It->Get();
//
//        if (ck::IsValid(PlayerController) && PlayerController != InRequest.Get_OwningPlayerController())
//        {
//            const auto NewOrExistingActor = [&]() -> TObjectPtr<AActor>
//            {
//                // if Outermost and ActorToReplicate are the same, the Actor is already replicated. No need
//                // to spawn another one. Simply return the Outermost.
//                if (InRequest.Get_OutermostActor()->GetClass() == InRequest.Get_ActorToReplicate())
//                {
//                    return InRequest.Get_OutermostActor();
//                }
//
//                return UCk_Utils_Actor_UE::Request_SpawnActor
//                (
//                    UCk_Utils_Actor_UE::SpawnActorParamsType{OutermostActor, InRequest.Get_ActorToReplicate()}
//                        .Set_SpawnTransform(InRequest.Get_Transform())
//                );
//            }();
//
//            const auto& NewActorBasicInfo = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(NewOrExistingActor);
//
//            UCk_Utils_Net_UE::Add(NewActorBasicInfo.Get_Handle(),
//                FCk_Net_ConnectionSettings{ECk_Net_NetRoleType::Client, ECk_Net_EntityNetRole::Proxy});
//            UCk_Utils_ReplicatedObjects_UE::Add(NewActorBasicInfo.Get_Handle(), ROs);
//        }
//        else
//        {
//            const auto OriginalOwnerHandle = WorldSubsystem->Get_TransientEntity().Get_ValidHandle
//            (
//                static_cast<FCk_Entity::IdType>(InRequest.Get_OriginalOwnerEntity())
//            );
//
//            UCk_Utils_Net_UE::Add(OriginalOwnerHandle,
//                FCk_Net_ConnectionSettings{ECk_Net_NetRoleType::Client, ECk_Net_EntityNetRole::Authority});
//            UCk_Utils_ReplicatedObjects_UE::Add(OriginalOwnerHandle, ROs);
//        }
//
//        ck::net::Verbose(TEXT("Replicating [{}] with outermost [{}]"),
//            InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate(), PlayerController);
//    }
//}

auto
    UCk_EcsConstructionScript_ActorComponent_UE::
    OnUnregister()
    -> void
{
    [this]()
    {
        if (IsTemplate())
        { return; }

        if (_Replication != ECk_Replication::Replicates)
        { return; }

        if (ck::Is_NOT_Valid(_Entity))
        { return; }

        if (NOT UCk_Utils_ReplicatedObjects_UE::Has(_Entity))
        { return; }

        const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(_Entity);

        ck::algo::ForEachIsValid(ReplicatedObjects.Get_ReplicatedObjects(), [&](auto& InRO)
        {
            auto EcsRO = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO);
            UCk_Ecs_ReplicatedObject_UE::Destroy(EcsRO);
        });
    }();

    Super::OnUnregister();
}

auto
    UCk_EcsConstructionScript_ActorComponent_UE::
    Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams)
    -> void
{
    // --------------------------------------------------------------------------------------------------------------------

    const auto& OwningActor = GetOwner();

    if (OwningActor->bIsEditorOnlyActor)
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    _DoConstructCalled = true;

    Super::Do_Construct_Implementation(InParams);

    CK_ENSURE_IF_NOT(ck::IsValid(_UnrealEntity),
        TEXT("UnrealEntity is [{}]. Did you forget to set the default value in the component?.[{}]"), _UnrealEntity, ck::Context(this))
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    const auto WorldSubsystem = Cast<UCk_EcsWorld_Subsystem_UE>(GetWorld()->GetSubsystemBase(_EcsWorldSubsystem));

    // TODO: this hits at least once because the BP Subsystem is not loaded. Fix this.
    CK_ENSURE_IF_NOT(ck::IsValid(WorldSubsystem),
        TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"), _EcsWorldSubsystem, ck::Context(this))
    { return; }

    _Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(WorldSubsystem->Get_TransientEntity());

    _Entity.Add<ck::FFragment_OwningActor_Current>(OwningActor);

    // --------------------------------------------------------------------------------------------------------------------
    // LINK TO ACTOR
    // EcsConstructionScript is a bit special in that it readies everything immediately instead of deferring the operation

    // We need the EntityOwningActor ActorComponent to exist before building the Unreal Entity

    if (const auto EntityOwningActorComponent = OwningActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
        ck::IsValid(EntityOwningActorComponent))
    {
        EntityOwningActorComponent->Set_EntityHandle(_Entity);
    }
    else
    {
        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
            {
                OwningActor,
                true
            },
            [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
            {
                InComp->_EntityHandle = _Entity;
            }
        );
    }

    if (OwningActor->HasAuthority())
    {
        if (ck::Is_NOT_Valid(OwningActor->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>()))
        {
            UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ObjectReplicator_ActorComponent_UE>
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ObjectReplicator_ActorComponent_UE>
                {
                    OwningActor,
                    true
                },
                [&](UCk_ObjectReplicator_ActorComponent_UE* InComp) { }
            );
        }
    }

    // --------------------------------------------------------------------------------------------------------------------
    // Add Net Connection Settings

    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        UCk_Utils_Net_UE::Add(_Entity, FCk_Net_ConnectionSettings{ECk_Net_NetRoleType::Host, ECk_Net_EntityNetRole::Authority});
    }
    else if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    {
        if (OwningActor->GetLocalRole() == ROLE_Authority)
        {
            UCk_Utils_Net_UE::Add(_Entity, FCk_Net_ConnectionSettings{ECk_Net_NetRoleType::Host, ECk_Net_EntityNetRole::Authority});
        }
    }
    else if (GetWorld()->IsNetMode(NM_Client))
    {
        if (OwningActor->GetLocalRole() == ROLE_Authority)
        {
            UCk_Utils_Net_UE::Add(_Entity, FCk_Net_ConnectionSettings{ECk_Net_NetRoleType::Client, ECk_Net_EntityNetRole::Authority});
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        // Add the replication driver here
        UCk_Utils_EntityReplicationDriver_UE::Add(_Entity);

        // --------------------------------------------------------------------------------------------------------------------
        // Build Entity

        const auto CsWithTransform = Cast<UCk_UnrealEntity_ConstructionScript_WithTransform_PDA>(_UnrealEntity->Get_EntityConstructionScript());

        CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT one with Transform. "
            "Entity Construction Scripts that have an Actor attached MUST use [{}]."), _UnrealEntity->Get_EntityConstructionScript(), OwningActor,
            ck::TypeToString<UCk_UnrealEntity_ConstructionScript_WithTransform_PDA>)
        { return; }

        CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());

        _UnrealEntity->Build(_Entity);

        const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(_Entity);
        UCk_Utils_EntityReplicationDriver_UE::Request_ReplicateEntityOnReplicatedActor(_Entity,
            FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor
            {
                OwningActor, CsWithTransform
            }.Set_ReplicatedObjects(ReplicatedObjects.Get_ReplicatedObjects()));
    }

    //// --------------------------------------------------------------------------------------------------------------------
    //// Call RPC if EcsConstructionScript is Replicated

    //if (_Replication == ECk_Replication::DoesNotReplicate)
    //{ return; }

    //CK_ENSURE_IF_NOT(OwningActor->IsSupportedForNetworking(),
    //    TEXT("The Owning Actor [{}] of [{}] is NOT stably named. Cannot proceed with replication. Did you create this Actor/ConstructionScript at runtime?[{}]"),
    //    OwningActor, this, ck::Context(this))
    //{ return; }

    //const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    //// In this case, we are one of the clients and we do NOT need to replicate further
    //if (ck::Is_NOT_Valid(OutermostActor))
    //{ return; }

    //const auto& ConstructionScript = OutermostActor->GetComponentByClass<ThisType>();

    //CK_ENSURE_IF_NOT(ck::IsValid(ConstructionScript),
    //    TEXT("Found a REPLICATED with AUTHORITY Actor [{}] BUT it does NOT have [{}]. Are you sure this Actor's construction involved a Replicated Actor?[{}]"),
    //    this, ck::TypeToString<ThisType>, ck::Context(this))
    //{ return; }

    //// We only want to go INTO this if statement, IF we are the Client that has a non-replicated Actor with replicated Entities
    //// (i.e. replicated construction script). So we make sure that IF we have already handled the replicated data
    //// we do not need to continue the 'chain' of replication
    //// TODO: along with everything in this file, this needs refactoring
    //if (GetWorld()->IsNetMode(NM_Client) && NOT _ReplicationDataReplicated)
    //{
    //    ConstructionScript->Request_ReplicateActor_OnServer
    //    (
    //        FCk_EcsConstructionScript_ConstructionInfo{}
    //        .Set_OutermostActor(OutermostActor)
    //        .Set_ActorToReplicate(OwningActor->GetClass())
    //        .Set_OwningPlayerController(GetWorld()->GetFirstPlayerController())
    //        .Set_OriginalOwnerEntity(static_cast<int32>(_Entity.Get_Entity().Get_ID()))
    //        .Set_Transform(OwningActor->GetActorTransform()) // TODO: maybe only send Location and Rotation to reduce bandwidth requirements
    //    );
    //}
    //else if (OutermostActor->GetRemoteRole() != ROLE_AutonomousProxy)
    //{
    //    const auto& NewActorBasicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(OwningActor);
    //    const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewActorBasicDetails.Get_Handle());

    //    auto ReplicatedObjectsData = FCk_EcsConstructionScript_ReplicateObjects_Data{OutermostActor};

    //    for (const auto& ReplicatedObject : ReplicatedObjects.Get_ReplicatedObjects())
    //    {
    //        CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObject), TEXT("Invalid Replicated Object encountered for Actor [{}] on the SERVER.[{}]"), NewActorBasicDetails, ck::Context(this))
    //        { continue; }

    //        ReplicatedObjectsData.Get_Objects().Emplace(ReplicatedObject->GetClass());
    //        ReplicatedObjectsData.Get_NetStableNames().Emplace(ReplicatedObject->GetFName());
    //    }

    //    _ReplicationData = FCk_EcsConstructionScript_Replication_Data
    //    {
    //        FCk_EcsConstructionScript_ConstructionInfo{}
    //        .Set_OutermostActor(OutermostActor)
    //        .Set_ActorToReplicate(OwningActor->GetClass())
    //        .Set_Transform(OwningActor->GetActorTransform()),
    //        ReplicatedObjectsData
    //    };
    //}
}

//auto
//    UCk_EcsConstructionScript_ActorComponent_UE::
//    OnRep_ReplicationData()
//    -> void
//{
//    _ReplicationDataReplicated = true;
//
//    if (NOT _DoConstructCalled)
//    { return; }
//
//    ck::net::Log(TEXT("Starting to replicate [{}]"), _ReplicationData.Get_ConstructionInfo().Get_ActorToReplicate());
//    Request_ReplicateActor_OnClients(_ReplicationData.Get_ReplicatedObjects(), _ReplicationData.Get_ConstructionInfo());
//}

//void
//    UCk_EcsConstructionScript_ActorComponent_UE::GetLifetimeReplicatedProps(
//        TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//    DOREPLIFETIME(ThisType, _ReplicationData);
//}

// --------------------------------------------------------------------------------------------------------------------
