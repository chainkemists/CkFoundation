#include "CkEcsConstructionScript_ActorComponent.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkNet/CkNet_Fragment.h"
#include "CkNet/CkNet_Utils.h"

#include "CkUnreal/CkUnreal_Log.h"
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

auto
    UCk_EcsConstructionScript_ActorComponent_UE::
    Request_ReplicateActor_OnServer_Implementation(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest)
    -> void
{
    const auto& OutermostActor = InRequest.Get_OutermostActor();
    const auto& ActorToReplicate = InRequest.Get_ActorToReplicate();

    const auto NewOrExistingActor = [&]() -> TObjectPtr<AActor>
    {
        // if Outermost and ActorToReplicate are the same, the Actor is already replicated. No need
        // to spawn another one. Simply return the Outermost.
        if (InRequest.Get_OutermostActor()->GetClass() == InRequest.Get_ActorToReplicate())
        {
            return InRequest.Get_OutermostActor();
        }

        return UCk_Utils_Actor_UE::Request_SpawnActor
        (
            UCk_Utils_Actor_UE::SpawnActorParamsType{OutermostActor, ActorToReplicate}
                .Set_SpawnTransform(InRequest.Get_Transform())
        );
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(NewOrExistingActor),
        TEXT("Failed to spawn Actor to Replicate [{}] on SERVER.[{}]"), ActorToReplicate, ck::Context(this))
    { return; }

    const auto& NewActorBasicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(NewOrExistingActor);
    const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewActorBasicDetails.Get_Handle());

    for (const auto& ReplicatedObject : ReplicatedObjects.Get_ReplicatedObjects())
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObject), TEXT("Invalid Replicated Object encountered for Actor [{}] on the SERVER.[{}]"), NewActorBasicDetails, ck::Context(this))
        { continue; }

        Request_ReplicateObject(OutermostActor, ReplicatedObject->GetClass(), ReplicatedObject->GetFName());
    }

    auto RequestToForward = InRequest;
    RequestToForward.Set_ReplicatedObjects(ReplicatedObjects);

    Request_ReplicateActor_OnClients(RequestToForward);

    ck::unreal::Verbose(TEXT("Replicating [{}] with outermost [{}] on SERVER"), OutermostActor, ActorToReplicate);
}

auto
    UCk_EcsConstructionScript_ActorComponent_UE::
    Request_ReplicateActor_OnClients_Implementation(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest)
    -> void
{
    const auto& OutermostActor = InRequest.Get_OutermostActor();
    CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor), TEXT("Invalid Outermose Actor on CLIENT.[{}]"), ck::Context(this))
    { return; }

    // TODO: Fix this so that we don't need this check
    if (OutermostActor->IsNetMode(NM_DedicatedServer))
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    const auto WorldSubsystem = Cast<UCk_EcsWorld_Subsystem_UE>(GetWorld()->GetSubsystemBase(_EcsWorldSubsystem));

    // TODO: this hits at least once because the BP Subsystem is not loaded. Fix this.
    CK_ENSURE_IF_NOT(ck::IsValid(WorldSubsystem),
        TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"),
        _EcsWorldSubsystem, ck::Context(this))
    { return; }

    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const auto& PlayerController = It->Get();

        if (ck::IsValid(PlayerController) && PlayerController != InRequest.Get_OwningPlayerController())
        {
            const auto NewOrExistingActor = [&]() -> TObjectPtr<AActor>
            {
                // if Outermost and ActorToReplicate are the same, the Actor is already replicated. No need
                // to spawn another one. Simply return the Outermost.
                if (InRequest.Get_OutermostActor()->GetClass() == InRequest.Get_ActorToReplicate())
                {
                    return InRequest.Get_OutermostActor();
                }

                return UCk_Utils_Actor_UE::Request_SpawnActor
                (
                    UCk_Utils_Actor_UE::SpawnActorParamsType{OutermostActor, InRequest.Get_ActorToReplicate()}
                        .Set_SpawnTransform(InRequest.Get_Transform())
                );
            }();

            const auto& NewActorBasicInfo = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(NewOrExistingActor);

            UCk_Utils_ReplicatedObjects_UE::Add(NewActorBasicInfo.Get_Handle(), InRequest.Get_ReplicatedObjects());
        }
        else
        {
            const auto OriginalOwnerHandle = WorldSubsystem->Get_TransientEntity().Get_ValidHandle
            (
                static_cast<FCk_Entity::IdType>(InRequest.Get_OriginalOwnerEntity())
            );

            UCk_Utils_ReplicatedObjects_UE::Add(OriginalOwnerHandle, InRequest.Get_ReplicatedObjects());
        }

        ck::unreal::Verbose(TEXT("Replicating [{}] with outermost [{}] on CLIENT with PC [{}]"),
            InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate(), PlayerController);
    }
}

auto
    UCk_EcsConstructionScript_ActorComponent_UE::
    Request_ReplicateObject_Implementation(
        AActor* InReplicatedOwner,
        TSubclassOf<UCk_Ecs_ReplicatedObject_UE> InObject,
        FName InReplicatedName)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InReplicatedOwner), TEXT("Invalid Replicated Owner Actor.[{}]"), ck::Context(this))
    { return; }

    if (InReplicatedOwner->IsNetMode(NM_DedicatedServer))
    { return; }

    // TODO: Sending garbage entity handle until we manage to link it up properly
    UCk_Ecs_ReplicatedObject_UE::Create(InObject, InReplicatedOwner, InReplicatedName, FCk_Handle{});
}

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
    const auto& OwningActor = GetOwner();

    _Entity.Add<ck::FFragment_OwningActor_Current>(OwningActor);

    if (OwningActor->IsNetMode(NM_DedicatedServer))
    {
        UCk_Utils_Net_UE::Request_MarkEntityAs_DedicatedServer(_Entity);
    }

    // --------------------------------------------------------------------------------------------------------------------
    // LINK TO ACTOR
    // EcsConstructionScript is a bit special in that it readies everything immediately instead of deferring the operation

    // We need the EntityOwningActor ActorComponent to exist before building the Unreal Entity
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

    if (OwningActor->HasAuthority())
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

    // --------------------------------------------------------------------------------------------------------------------
    // Build Entity

    const auto CsWithTransform = Cast<UCk_UnrealEntity_ConstructionScript_WithTransform_PDA>(_UnrealEntity->Get_EntityConstructionScript());

    CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT one with Transform. "
        "Entity Construction Scripts that have an Actor attached MUST use [{}]."), _UnrealEntity->Get_EntityConstructionScript(), OwningActor,
        ctti::nameof_v<UCk_UnrealEntity_ConstructionScript_WithTransform_PDA>)
    { return; }

    CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());

    _UnrealEntity->Build(_Entity);

    // --------------------------------------------------------------------------------------------------------------------
    // Call RPC if EcsConstructionScript is Replicated

    if (_Replication == ECk_Replication::DoesNotReplicate)
    { return; }

    CK_ENSURE_IF_NOT(OwningActor->IsSupportedForNetworking(),
        TEXT("The Owning Actor [{}] of [{}] is NOT stably named. Cannot proceed with replication. Did you create this Actor/ConstructionScript at runtime?[{}]"),
        OwningActor, this, ck::Context(this))
    { return; }

    const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    // In this case, we are one of the clients and we do NOT need to replicate further
    if (ck::Is_NOT_Valid(OutermostActor))
    { return; }

    const auto& ConstructionScript = OutermostActor->GetComponentByClass<ThisType>();

    CK_ENSURE_IF_NOT(ck::IsValid(ConstructionScript),
        TEXT("Found a REPLICATED with AUTHORITY Actor [{}] BUT it does NOT have [{}]. Are you sure this Actor's construction involved a Replicated Actor?[{}]"),
        this, ctti::nameof_v<ThisType>, ck::Context(this))
    { return; }

    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        _Entity.Add<ck::FTag_HasAuthority>(OwningActor);
        return;
    }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        _Entity.Add<ck::FTag_HasAuthority>(OwningActor);

        ConstructionScript->Request_ReplicateActor_OnServer
        (
            FCk_EcsConstructionScript_ConstructionInfo{}
            .Set_OutermostActor(OutermostActor)
            .Set_ActorToReplicate(OwningActor->GetClass())
            .Set_OwningPlayerController(GetWorld()->GetFirstPlayerController())
            .Set_OriginalOwnerEntity(static_cast<int32>(_Entity.Get_Entity().Get_ID()))
            .Set_Transform(OwningActor->GetActorTransform()) // TODO: maybe only send Location and Rotation to reduce bandwidth requirements
        );
    }
    else if (OutermostActor->GetRemoteRole() != ROLE_AutonomousProxy)
    {
        _Entity.Add<ck::FTag_HasAuthority>(OwningActor);

        ConstructionScript->Request_ReplicateActor_OnClients
        (
            FCk_EcsConstructionScript_ConstructionInfo{}
            .Set_OutermostActor(OutermostActor)
            .Set_ActorToReplicate(OwningActor->GetClass())
            .Set_ReplicatedObjects(UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(_Entity))
            .Set_Transform(OwningActor->GetActorTransform())
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
