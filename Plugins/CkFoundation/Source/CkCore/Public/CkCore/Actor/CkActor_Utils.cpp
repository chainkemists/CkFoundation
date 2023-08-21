#include "CkActor_Utils.h"

#include <CoreMinimal.h>
#include <GameFramework/Pawn.h>
#include <Engine/World.h>
#include <Components/SkeletalMeshComponent.h>
#include <Components/StaticMeshComponent.h>

#include "CkEnsure/CkEnsure.h"

#include "CkGame/CkGame_Utils.h"

#include "CkObject/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Actor_SpawnActor_Params::
FCk_Utils_Actor_SpawnActor_Params(
    TWeakObjectPtr<UObject> InOwnerOrWorld,
    TSubclassOf<AActor> InActorClass)
    :_OwnerOrWorld(std::move(InOwnerOrWorld))
    , _ActorClass(InActorClass)
{
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Utils_Actor_UE::DeferredSpawnActor_Params::
DeferredSpawnActor_Params(
    const TSubclassOf<AActor> InActorClass,
    const TObjectPtr<AActor> InArchetype,
    const FTransform& InSpawnTransform,
    const ESpawnActorCollisionHandlingMethod InCollisionHandlingOverride,
    TObjectPtr<UObject> InOwnerOrWorld)
    : _ActorClass(InActorClass)
    , _Archetype(InArchetype)
    , _SpawnTransform(InSpawnTransform)
    , _CollisionHandlingOverride(InCollisionHandlingOverride)
    , _OwnerOrWorld(InOwnerOrWorld)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Actor_UE::Get_PersistentLevelScriptActor(const UObject* InWorldContextObject) -> ALevelScriptActor*
{
    const auto& World = [&]() -> UWorld*
    {
        if (ck::IsValid(InWorldContextObject))
        {
            return UCk_Game_Utils_UE::Get_WorldForObject(InWorldContextObject);
        }

        const auto& gameInstance = UCk_Game_Utils_UE::Get_GameInstance(nullptr);

        if (ck::Is_NOT_Valid(gameInstance))
        { return {}; }

        return UCk_Game_Utils_UE::Get_WorldForObject(gameInstance);
    }();

    if (ck::Is_NOT_Valid(World))
    { return {}; }

    return World->PersistentLevel->GetLevelScriptActor();
}

auto
    UCk_Utils_Actor_UE::
    Get_OutermostActor(UObject* InObject)
    -> AActor*
{
    auto OuterObject = InObject;

    while (ck::IsValid(OuterObject))
    {
        auto MaybeActor = Cast<AActor>(OuterObject);

        if (ck::IsValid(MaybeActor))
        { return MaybeActor; }

        OuterObject = OuterObject->GetOuter();
    }

    return nullptr;
}

auto
    UCk_Utils_Actor_UE::
    Get_OutermostActor_Replicated(UObject* InObject)
    -> AActor*
{
    auto OuterObject = InObject;

    while (ck::IsValid(OuterObject))
    {
        auto MaybeActor = Cast<AActor>(OuterObject);

        if (ck::IsValid(MaybeActor))
        {
            if (MaybeActor->GetIsReplicated())
            { return MaybeActor; }
        }

        OuterObject = OuterObject->GetOuter();
    }

    return nullptr;
}

auto
    UCk_Utils_Actor_UE::
    Get_OutermostActor_RemoteAuthority(UObject* InObject)
    -> AActor*
{
    auto OuterObject = InObject;

    while (ck::IsValid(OuterObject))
    {
        auto MaybeActor = Cast<AActor>(OuterObject);

        if (ck::IsValid(MaybeActor))
        {
            switch(MaybeActor->GetRemoteRole())
            {
            case ROLE_Authority:
            case ROLE_AutonomousProxy:
                return MaybeActor;
            case ROLE_None:
            case ROLE_SimulatedProxy:
            case ROLE_MAX:
            default:
                break;
            }

            OuterObject = MaybeActor->GetOwner();
        }
        else
        {
            OuterObject = OuterObject->GetOuter();
        }
    }

    return nullptr;
}

auto
    UCk_Utils_Actor_UE::
    Request_CloneActor(
        AActor* InWorldContextActor,
        AActor* InOwner,
        AActor* InActorToClone,
        const ESpawnActorCollisionHandlingMethod InCollisionHandlingOverride,
        const ECk_Utils_Actor_SpawnActorPolicy InSpawnPolicy)
    -> AActor*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorToClone), TEXT("Failed to Clone Actor because it is invalid!"))
    { return {}; }

    if (ck::Is_NOT_Valid(InWorldContextActor))
    {
        InWorldContextActor = InOwner;
    }

    return Request_SpawnActor
    (
        SpawnActorParamsType{InOwner, InActorToClone->GetClass()}
        .Set_Archetype(InActorToClone)
        .Set_SpawnTransform(FTransform::Identity)
        .Set_CollisionHandlingOverride(InCollisionHandlingOverride)
        .Set_NetworkingType(ECk_Actor_NetworkingType::Local)
        .Set_SpawnPolicy(InSpawnPolicy)
    );
}

auto UCk_Utils_Actor_UE::
    Request_SpawnActor(
        const SpawnActorParamsType& InSpawnActorParams,
        const TFunction<void(AActor*)>& InPreFinishSpawningFunc) -> AActor*
{
    const auto& spawnedActor = DoRequest_SpawnActor_Begin(InSpawnActorParams);

    if (ck::Is_NOT_Valid(spawnedActor))
    { return {}; }

    if (InPreFinishSpawningFunc)
    {
        InPreFinishSpawningFunc(spawnedActor);
    }

    return DoRequest_SpawnActor_Finish(InSpawnActorParams, spawnedActor);
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_BeginDeferredSpawnActor(
        const DeferredSpawnActor_Params& InDeferredSpawnActorParams) -> AActor*
{
    const auto& ActorClass              = InDeferredSpawnActorParams.Get_ActorClass();
    const auto& OwnerOrWorld            = InDeferredSpawnActorParams.Get_OwnerOrWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld), TEXT("SpawnActor Request MUST have an Owner OR World. Unable to spawn [{}]."), ActorClass)
    { return {}; }

    const auto& World = [&]() -> UWorld*
    {
        const auto MaybeWorld = Cast<UWorld>(OwnerOrWorld);
        if (ck::IsValid(MaybeWorld))
        {
            return MaybeWorld;
        }

        const auto Actor = Cast<AActor>(OwnerOrWorld);

        CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld), TEXT("OwnerOrWorld [{}] MUST be an Owner OR World. Unable to spawn [{}]."),
            OwnerOrWorld, ActorClass)
        { return nullptr; }

        return Actor->GetWorld();
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("No World found while trying to spawn Actor of type [{}] with OwnerOrWorld [{}]!"),
        ActorClass,
        OwnerOrWorld)
    { return {}; }

    const auto& Archetype = InDeferredSpawnActorParams.Get_Archetype();

    if (ck::IsValid(Archetype))
    {
        CK_ENSURE_IF_NOT(Archetype->IsA(ActorClass),
            TEXT("Failed to defer SpawnActor of Class [{}] because the Archetype supplied [{}] does not match the class"),
            ActorClass,
            Archetype)
        { return {}; }
    }

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Template                       = Archetype;
    SpawnInfo.SpawnCollisionHandlingOverride = InDeferredSpawnActorParams.Get_CollisionHandlingOverride();
    SpawnInfo.Owner                          = Cast<AActor>(OwnerOrWorld);
    SpawnInfo.bDeferConstruction             = true;

    if (ck::Is_NOT_Valid(ActorClass))
    { return {}; }

    const auto& SpawnedActor = World->SpawnActor(ActorClass, &InDeferredSpawnActorParams.Get_SpawnTransform(), SpawnInfo);

    return SpawnedActor;
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_SpawnActor_Begin(
        const SpawnActorParamsType& InSpawnActorParams) -> AActor*
{
    const auto& OwnerOrWorld = InSpawnActorParams.Get_OwnerOrWorld().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld), TEXT("Unable to continue with SpawnActor. OwnerOrWorld is [{}]"), OwnerOrWorld)
    { return {}; }

    const auto& World = [&]() -> UWorld*
    {
        const auto MaybeWorld = Cast<UWorld>(OwnerOrWorld);
        if (ck::IsValid(MaybeWorld))
        {
            return MaybeWorld;
        }

        const auto Actor = Cast<AActor>(OwnerOrWorld);

        CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld), TEXT("OwnerOrWorld [{}] MUST be an Owner OR World. Unable to spawn [{}]."),
            OwnerOrWorld, InSpawnActorParams.Get_ActorClass())
        { return nullptr; }

        return Actor->GetWorld();
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("World of WorldContextActor [{}] is [{}]!"), OwnerOrWorld, World)
    { return {}; }

    const auto& Replication  = InSpawnActorParams.Get_NetworkingType();
    const auto& IsReplicated = Replication == ECk_Actor_NetworkingType::Replicated;

    if (IsReplicated)
    {
        CK_ENSURE_IF_NOT(World->GetNetMode() != ENetMode::NM_Client, TEXT("Clients are not allowed to call SpawnActors"))
        { return {}; }
    }

    const auto& DeferredSpawnActorParams = DeferredSpawnActor_Params
    {
        InSpawnActorParams.Get_ActorClass(),
        InSpawnActorParams.Get_Archetype().Get(),
        InSpawnActorParams.Get_SpawnTransform(),
        InSpawnActorParams.Get_CollisionHandlingOverride(),
        InSpawnActorParams.Get_OwnerOrWorld().Get()
    };

    const auto& SpawningActor = DoRequest_BeginDeferredSpawnActor(DeferredSpawnActorParams);

    CK_ENSURE_IF_NOT(ck::IsValid(SpawningActor),
        TEXT("Failed to deferred spawn Actor of Class [{}] and Archetype [{}]!"),
        InSpawnActorParams.Get_ActorClass(),
        DeferredSpawnActorParams.Get_Archetype())
    { return {}; }

    // If we want a replicated actor, the actor must be set to Replicate, and vice-versa.
    const auto& ReplicationMatches = (IsReplicated && SpawningActor->GetIsReplicated()) ||
        (NOT IsReplicated && NOT SpawningActor->GetIsReplicated());

    CK_ENSURE_IF_NOT(ReplicationMatches,
        TEXT("Requested NetworkingType [{}] does not match the spawningActor [{}] replication settings"),
        Replication,
        SpawningActor)
    {
        SpawningActor->Destroy();
        return {};
    }

    return SpawningActor;
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_SpawnActor_Finish(
        const SpawnActorParamsType& InSpawnActorParams,
        AActor* InNewlySpawnedActor) -> AActor*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNewlySpawnedActor), TEXT("Newly spawned Actor is [{}]"), InNewlySpawnedActor)
    { return {}; }

    const auto& FinishedSpawningActor = UGameplayStatics::FinishSpawningActor(InNewlySpawnedActor, InSpawnActorParams.Get_SpawnTransform());

    CK_ENSURE_IF_NOT(ck::IsValid(FinishedSpawningActor), TEXT("Finished spawning Actor is [{}]"), FinishedSpawningActor)
    { return {}; }

    if (const auto& Archetype = InSpawnActorParams.Get_Archetype().Get(); ck::IsValid(Archetype))
    {
        DoRequest_CopyAllActorComponentProperties(Archetype, FinishedSpawningActor);
    }

    return FinishedSpawningActor;
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_CopyAllActorComponentProperties(AActor* InSourceActor, AActor* InDestinationActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSourceActor), TEXT("Source spawned Actor is [{}]"), InSourceActor)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDestinationActor), TEXT("Destination spawned Actor is [{}]"), InDestinationActor)
    { return; }

    InDestinationActor->ForEachComponent(false, [&](UActorComponent* InDestinationActorComponent)
    {
        InSourceActor->ForEachComponent(false, [&](UActorComponent* InSourceActorComponent)
        {
            if (NOT InDestinationActorComponent->IsA(InSourceActorComponent->GetClass()) || NOT InSourceActorComponent->IsA(InDestinationActorComponent->GetClass()))
            { return; }

            UCk_Utils_Object_UE::Request_CopyAllProperties
            (
                FCk_Utils_Object_CopyAllProperties_Params
                {
                    InDestinationActorComponent,
                    InSourceActorComponent
                }
            );
        });
    });
}

// --------------------------------------------------------------------------------------------------------------------
