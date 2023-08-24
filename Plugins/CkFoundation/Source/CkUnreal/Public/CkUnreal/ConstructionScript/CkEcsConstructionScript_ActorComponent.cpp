#include "CkEcsConstructionScript_ActorComponent.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"
#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

#include "CkUnreal/CkUnreal_Log.h"
#include "Engine/World.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_EcsConstructionScript_ActorComponent::
    UCk_EcsConstructionScript_ActorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bReplicateUsingRegisteredSubObjectList = true;
    SetIsReplicatedByDefault(true);
}

auto
    UCk_EcsConstructionScript_ActorComponent::
    Request_ReplicateActor_OnServer_Implementation(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest)
    -> void
{
    const auto& OutermostActor = InRequest.Get_OutermostActor();
    const auto& ActorToReplicate = InRequest.Get_ActorToReplicate();

    const auto NewActor = UCk_Utils_Actor_UE::Request_SpawnActor(UCk_Utils_Actor_UE::SpawnActorParamsType{OutermostActor, ActorToReplicate});

    CK_ENSURE_IF_NOT(ck::IsValid(NewActor), TEXT("Failed to spawn Actor to Replicate [{}] on SERVER.[{}]"), ActorToReplicate, ck::Context(this))
    { return; }

    const auto& NewActorBasicDetails = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails_FromActor(NewActor);
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
    UCk_EcsConstructionScript_ActorComponent::
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
            const auto NewActor = UCk_Utils_Actor_UE::Request_SpawnActor(
                UCk_Utils_Actor_UE::SpawnActorParamsType{InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate()});

            const auto& NewActorBasicInfo = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails_FromActor(NewActor);

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
    UCk_EcsConstructionScript_ActorComponent::
    Request_ReplicateObject_Implementation(
        AActor* InReplicatedOwner,
        TSubclassOf<UCk_Ecs_ReplicatedObject> InObject,
        FName InReplicatedName)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InReplicatedOwner), TEXT("Invalid Replicated Owner Actor.[{}]"), ck::Context(this))
    { return; }

    if (InReplicatedOwner->IsNetMode(NM_DedicatedServer))
    { return; }

    // TODO: Sending garbage entity handle until we manage to link it up properly
    UCk_Ecs_ReplicatedObject::Create(InObject, InReplicatedOwner, InReplicatedName, FCk_Handle{});
}

auto
    UCk_EcsConstructionScript_ActorComponent::
    BeginDestroy()
    -> void
{
    Super::BeginDestroy();
}

auto
    UCk_EcsConstructionScript_ActorComponent::
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

    _Entity.Add<ck::FCk_Fragment_ActorInfo_Params>(FCk_Fragment_ActorInfo_ParamsData
    {
        OwningActor->GetClass(),
        OwningActor->GetActorTransform(),
        OwningActor->GetOwner(),
        OwningActor->GetIsReplicated() ? ECk_Actor_NetworkingType::Replicated : ECk_Actor_NetworkingType::Local
    });
    _Entity.Add<ck::FCk_Fragment_ActorInfo_Current>(OwningActor);

    if (OwningActor->IsNetMode(NM_DedicatedServer))
    { _Entity.Add<ck::FCk_Tag_NetMode_DedicatedServer>(); }

    // --------------------------------------------------------------------------------------------------------------------
    // LINK TO ACTOR
    // EcsConstructionScript is a bit special in that it readies everything immediately instead of deferring the operation

    // We need the ActorInfo ActorComponent to exist before building the Unreal Entity
    UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ActorInfo_ActorComponent_UE>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ActorInfo_ActorComponent_UE>
        {
            OwningActor,
            true
        },
        [&](UCk_ActorInfo_ActorComponent_UE* InComp)
        {
            InComp->_EntityHandle = _Entity;
        }
    );

    if (OwningActor->HasAuthority())
    {
        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ObjectReplicator_Component>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ObjectReplicator_Component>
            {
                OwningActor,
                true
            },
            [&](UCk_ObjectReplicator_Component* InComp) { }
        );
    }

    // --------------------------------------------------------------------------------------------------------------------
    // Build Entity

    _UnrealEntity->Build(_Entity);

    // --------------------------------------------------------------------------------------------------------------------
    // Call RPC if EcsConstructionScript is Replicated

    if (_Replication == ECk_Replication::DoesNotReplicate)
    { return; }

    // Replicated Actors will run this construction script automatically
    if (OwningActor->GetIsReplicated())
    { return; }

    CK_ENSURE_IF_NOT(OwningActor->IsSupportedForNetworking(),
        TEXT("The Owning Actor [{}] of [{}] is NOT stably named. Cannot proceed with replication. Did you create this Actor/ConstructionScript at runtime?"),
        OwningActor, this)
    { return; }

    const auto OutermostActor =  UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    // In this case, we are one of the clients and we do NOT need to replicate further
    if (ck::Is_NOT_Valid(OutermostActor))
    { return; }

    //CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor),
    //    TEXT("Could not find a REPLICATED with AUTHORITY Outermost Actor for [{}]. Are you sure you want this ConstructionScript/Entity to be Replicated?"),
    //    this)
    //{ return; }

    const auto& ConstructionScript = OutermostActor->GetComponentByClass<ThisType>();

    CK_ENSURE_IF_NOT(ck::IsValid(ConstructionScript),
        TEXT("Found a REPLICATED with AUTHORITY Actor [{}] BUT it does NOT have [{}]. Are you sure this Actor's construction involved a Replicated Actor?"),
        this, ctti::nameof_v<ThisType>)
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        ConstructionScript->Request_ReplicateActor_OnServer
        (
            FCk_EcsConstructionScript_ConstructionInfo{}
            .Set_OutermostActor(OutermostActor)
            .Set_ActorToReplicate(OwningActor->GetClass())
            .Set_OwningPlayerController(GetWorld()->GetFirstPlayerController())
            .Set_OriginalOwnerEntity(static_cast<int32>(_Entity.Get_Entity().Get_ID()))
            .Set_Transform(OwningActor->GetActorTransform()) // maybe only send Location and Rotation to reduce bandwidth requirements
        );
    }
    else if (OutermostActor->GetRemoteRole() != ROLE_AutonomousProxy)
    {
        ConstructionScript->Request_ReplicateActor_OnClients
        (
            FCk_EcsConstructionScript_ConstructionInfo{}
            .Set_OutermostActor(OutermostActor)
            .Set_ActorToReplicate(OwningActor->GetClass())
            .Set_ReplicatedObjects(UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(_Entity))
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
