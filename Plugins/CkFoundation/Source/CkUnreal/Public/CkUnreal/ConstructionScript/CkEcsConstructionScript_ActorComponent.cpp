#include "CkEcsConstructionScript_ActorComponent.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkUnreal/CkUnreal_Log.h"

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
    UCk_Utils_Actor_UE::Request_SpawnActor(
        UCk_Utils_Actor_UE::SpawnActorParamsType{InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate()});

    Request_ReplicateActor_OnClients(InRequest);

    ck::unreal::Verbose(TEXT("Replicating [{}] with outermost [{}] on SERVER"), InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate());
}

auto
    UCk_EcsConstructionScript_ActorComponent::
    Request_ReplicateActor_OnClients_Implementation(
        const FCk_EcsConstructionScript_ConstructionInfo& InRequest)
    -> void
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        auto PlayerController = It->Get();
        if (ck::IsValid(PlayerController) && PlayerController != InRequest.Get_OwningPlayerController())
        {
            ck::unreal::Verbose(TEXT("Replicating [{}] with outermost [{}] on CLIENT with PC [{}]"),
                InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate(), PlayerController);

            UCk_Utils_Actor_UE::Request_SpawnActor(
                UCk_Utils_Actor_UE::SpawnActorParamsType{InRequest.Get_OutermostActor(), InRequest.Get_ActorToReplicate()});
        }
    }
}

void UCk_EcsConstructionScript_ActorComponent::BeginDestroy()
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

    const auto WorldSubsystem = Cast<UCk_EcsWorld_Subsystem_UE>(GetWorld()->GetSubsystemBase(_EcsWorldSubsystem));

    // TODO: this hits at least once because the BP Subsystem is not loaded. Fix this.
    CK_ENSURE_IF_NOT(ck::IsValid(WorldSubsystem),
        TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"), _EcsWorldSubsystem, ck::Context(this))
    { return; }

    _Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(WorldSubsystem->Get_TransientEntity());
    _UnrealEntity->Build(_Entity);

    // --------------------------------------------------------------------------------------------------------------------
    // LINK TO ACTOR
    // EcsConstructionScript is a bit special in that it readies everything immediately instead of deferring the operation

    auto InHandle = _Entity;
    auto InActor = GetOwner();

    InHandle.Add<ck::FCk_Fragment_ActorInfo_Params>(FCk_Fragment_ActorInfo_ParamsData
    {
        InActor->GetClass(),
        InActor->GetActorTransform(),
        InActor->GetOwner(),
        InActor->GetIsReplicated() ? ECk_Actor_NetworkingType::Replicated : ECk_Actor_NetworkingType::Local
    });

    UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ActorInfo_ActorComponent_UE>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ActorInfo_ActorComponent_UE>
        {
            InActor,
            true
        },
        [&](UCk_ActorInfo_ActorComponent_UE* InComp)
        {
            InComp->_EntityHandle = InHandle;
        }
    );

    if (InActor->HasAuthority())
    {
        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ObjectReplicator_Component>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ObjectReplicator_Component>
            {
                InActor,
                true
            },
            [&](UCk_ObjectReplicator_Component* InComp) { }
        );
    }

    InHandle.Add<ck::FCk_Fragment_ActorInfo_Current>(InActor);

    // --------------------------------------------------------------------------------------------------------------------
    // Call RPC if Replicated

    if (_Replication == ECk_Replication::DoesNotReplicate)
    { return; }

    auto OwningActor = this->GetOwner();

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

    const auto ConstructionScript = OutermostActor->GetComponentByClass<ThisType>();

    CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor),
        TEXT("Found a REPLICATED with AUTHORITY Actor [{}] BUT it does NOT have [{}]. Are you sure this Actor's construction involved a Replicated Actor?"),
        this, ctti::nameof_v<ThisType>)
    { return; }

    if (NOT GetWorld()->IsNetMode(NM_Client) && OutermostActor->GetRemoteRole() != ROLE_AutonomousProxy)
    {
        ConstructionScript->Request_ReplicateActor_OnClients(FCk_EcsConstructionScript_ConstructionInfo{}
            .Set_OutermostActor(OutermostActor)
            .Set_ActorToReplicate(OwningActor->GetClass()));
    }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        ConstructionScript->Request_ReplicateActor_OnServer(FCk_EcsConstructionScript_ConstructionInfo{}
            .Set_OutermostActor(OutermostActor)
            .Set_ActorToReplicate(OwningActor->GetClass())
            .Set_OwningPlayerController(GetWorld()->GetFirstPlayerController()));
    }
}
