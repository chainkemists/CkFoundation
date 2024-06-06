#include "CkActor_Utils.h"

#include <CoreMinimal.h>
#include <GameFramework/Pawn.h>
#include <Engine/World.h>
#include <Components/SkeletalMeshComponent.h>
#include <Components/StaticMeshComponent.h>

#include "CkCore/Ensure/CkEnsure.h"

#include "CkCore/Game/CkGame_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Actor_SpawnActor_Params::
    FCk_Utils_Actor_SpawnActor_Params(
        UObject* InOwnerOrWorld,
        TSubclassOf<AActor> InActorClass)
    :_OwnerOrWorld(std::move(InOwnerOrWorld))
    , _ActorClass(InActorClass)
{
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Utils_Actor_UE::DeferredSpawnActor_Params::
    DeferredSpawnActor_Params(
        const TSubclassOf<AActor> InActorClass,
        UObject* InOwnerOrWorld)
    : _ActorClass(InActorClass)
    , _OwnerOrWorld(InOwnerOrWorld)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Actor_UE::
    Get_PersistentLevelScriptActor(
        const UObject* InWorldContextObject)
    -> ALevelScriptActor*
{
    const auto& World = [&]() -> UWorld*
    {
        if (ck::IsValid(InWorldContextObject))
        {
            return UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);
        }

        const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(nullptr);

        if (ck::Is_NOT_Valid(GameInstance))
        { return {}; }

        return UCk_Utils_Game_UE::Get_WorldForObject(GameInstance);
    }();

    if (ck::Is_NOT_Valid(World))
    { return {}; }

    return World->PersistentLevel->GetLevelScriptActor();
}

auto
    UCk_Utils_Actor_UE::
    Get_OutermostPawn(
        UObject* InObject)
    -> APawn*
{
    auto OuterObject = InObject;

    while (ck::IsValid(OuterObject, ck::IsValid_Policy_IncludePendingKill{}))
    {
        auto MaybePawn = Cast<APawn>(OuterObject);

        if (ck::IsValid(MaybePawn, ck::IsValid_Policy_IncludePendingKill{}))
        { return MaybePawn; }

        OuterObject = OuterObject->GetOuter();
    }

    return nullptr;
}

auto
    UCk_Utils_Actor_UE::
    Get_OutermostActor(
        UObject* InObject)
    -> AActor*
{
    auto OuterObject = InObject;

    while (ck::IsValid(OuterObject, ck::IsValid_Policy_IncludePendingKill{}))
    {
        auto MaybeActor = Cast<AActor>(OuterObject);

        if (ck::IsValid(MaybeActor, ck::IsValid_Policy_IncludePendingKill{}))
        { return MaybeActor; }

        OuterObject = OuterObject->GetOuter();
    }

    return nullptr;
}

auto
    UCk_Utils_Actor_UE::
    Get_OutermostActor_RemoteAuthority(
        UObject* InObject)
    -> AActor*
{
    auto OuterObject = InObject;

    while (ck::IsValid(OuterObject, ck::IsValid_Policy_IncludePendingKill{}))
    {
        auto MaybeActor = Cast<AActor>(OuterObject);

        if (ck::IsValid(MaybeActor, ck::IsValid_Policy_IncludePendingKill{}))
        {
            if (MaybeActor->GetRemoteRole() != ROLE_None)
            {
                if (MaybeActor->GetLocalRole() == ROLE_Authority || MaybeActor->GetLocalRole() == ROLE_AutonomousProxy)
                { return MaybeActor; }
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

auto
    UCk_Utils_Actor_UE::
    Get_HasComponentByClass(
        AActor* InActor,
        TSubclassOf<UActorComponent> InComponent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_HasComponentByClass"))
    { return {}; }

    return ck::IsValid(InActor->FindComponentByClass(InComponent));
}

auto
    UCk_Utils_Actor_UE::
    Get_DoesBoneExistInSkeletalMesh(
        AActor* InActor,
        FName   InBoneName)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_HasBoneInSkeletonMesh"))
    { return {}; }

    const auto& SkeletalMeshComp = InActor->FindComponentByClass<USkeletalMeshComponent>();

    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Cannot find Bone [{}] on Actor [{}] because it does NOT have a SkeletalMesh Component"),
        InBoneName,
        InActor)
    { return {}; }

    const auto& SocketBoneName = SkeletalMeshComp->GetSocketBoneName(InBoneName);
    const auto& BoneIndex      = SkeletalMeshComp->GetBoneIndex(SocketBoneName);

    return BoneIndex != INDEX_NONE;
}

auto
    UCk_Utils_Actor_UE::
    Request_SetActorLabel(
        AActor* InActor,
        const FString& InNewActorLabel,
        bool InMarkDirty)
    -> void
{
#if WITH_EDITOR
    InActor->SetActorLabel(InNewActorLabel, InMarkDirty);
#endif
}

auto
    UCk_Utils_Actor_UE::
    Request_SpawnActor(
        const SpawnActorParamsType& InSpawnActorParams,
        const TFunction<void(AActor*)>& InPreFinishSpawningFunc)
    -> AActor*
{
    const auto& SpawnedActor = DoRequest_SpawnActor_Begin(InSpawnActorParams);

    if (ck::Is_NOT_Valid(SpawnedActor))
    { return {}; }

    if (InPreFinishSpawningFunc)
    {
        InPreFinishSpawningFunc(SpawnedActor);
    }

    return DoRequest_SpawnActor_Finish(InSpawnActorParams, SpawnedActor);
}

auto
    UCk_Utils_Actor_UE::
    Request_RemoveActorComponent(
        UActorComponent* InComponentToRemove,
        bool InPromoteChildrenComponents)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComponentToRemove), TEXT("Invalid Actor Component to REMOVE"))
    { return; }

    InComponentToRemove->UnregisterComponent();
    InComponentToRemove->DestroyComponent(InPromoteChildrenComponents);
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_BeginDeferredSpawnActor(
        const DeferredSpawnActor_Params& InDeferredSpawnActorParams)
    -> AActor*
{
    const auto& ActorClass   = InDeferredSpawnActorParams.Get_ActorClass();
    const auto& OwnerOrWorld = InDeferredSpawnActorParams.Get_OwnerOrWorld();

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
    SpawnInfo.Template                       = Archetype.Get();
    SpawnInfo.SpawnCollisionHandlingOverride = InDeferredSpawnActorParams.Get_CollisionHandlingOverride();
    SpawnInfo.Owner                          = Cast<AActor>(OwnerOrWorld);
    SpawnInfo.bDeferConstruction             = true;

    if (ck::IsValid(InDeferredSpawnActorParams.Get_NonUniqueName()))
    {
        SpawnInfo.Name = UCk_Utils_Object_UE::Get_GeneratedUniqueName(
            OwnerOrWorld.Get(), ActorClass, InDeferredSpawnActorParams.Get_NonUniqueName());
    }

    if (ck::Is_NOT_Valid(ActorClass))
    { return {}; }

    const auto& SpawnedActor = World->SpawnActor(ActorClass, &InDeferredSpawnActorParams.Get_SpawnTransform(), SpawnInfo);

    if (NOT InDeferredSpawnActorParams.Get_Label().IsEmpty())
    {
        Request_SetActorLabel(SpawnedActor, InDeferredSpawnActorParams.Get_Label());
    }

    return SpawnedActor;
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_SpawnActor_Begin(
        const SpawnActorParamsType& InSpawnActorParams)
    -> AActor*
{
    const auto& OwnerOrWorld = InSpawnActorParams.Get_OwnerOrWorld().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(OwnerOrWorld), TEXT("Unable to continue with SpawnActor. OwnerOrWorld is [{}]"), OwnerOrWorld)
    { return {}; }

    const auto& World = [&]() -> UWorld*
    {
        if (const auto MaybeWorld = Cast<UWorld>(OwnerOrWorld);
            ck::IsValid(MaybeWorld))
        {
            return MaybeWorld;
        }

        const auto Actor = Cast<AActor>(OwnerOrWorld);

        CK_ENSURE_IF_NOT(ck::IsValid(Actor), TEXT("OwnerOrWorld [{}] MUST be an Actor OR World. Unable to spawn [{}]."),
            OwnerOrWorld, InSpawnActorParams.Get_ActorClass())
        { return nullptr; }

        return Actor->GetWorld();
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("World of WorldContextActor [{}] is [{}]!"), OwnerOrWorld, World)
    { return {}; }

    const auto& Replication  = InSpawnActorParams.Get_NetworkingType();
    const auto& IsReplicated = Replication == ECk_Actor_NetworkingType::Replicated;

    const auto IgnoreCertainChecksIfNotInGame = UCk_Utils_Game_UE::Get_GameStatus(World) == ECk_GameStatus::NotInGame;

    if (IsReplicated && NOT IgnoreCertainChecksIfNotInGame)
    {
        CK_ENSURE_IF_NOT(World->GetNetMode() != ENetMode::NM_Client, TEXT("Clients are not allowed to call SpawnActors"))
        { return {}; }
    }

    const auto DeferredSpawnActorParams = DeferredSpawnActor_Params{
        InSpawnActorParams.Get_ActorClass(), InSpawnActorParams.Get_OwnerOrWorld().Get()}
        .Set_Archetype(InSpawnActorParams.Get_Archetype().Get())
        .Set_CollisionHandlingOverride(InSpawnActorParams.Get_CollisionHandlingOverride())
        .Set_Label(InSpawnActorParams.Get_Label())
        .Set_NonUniqueName(InSpawnActorParams.Get_NonUniqueName())
        .Set_SpawnTransform(InSpawnActorParams.Get_SpawnTransform());

    const auto& SpawningActor = DoRequest_BeginDeferredSpawnActor(DeferredSpawnActorParams);

    CK_ENSURE_IF_NOT(ck::IsValid(SpawningActor),
        TEXT("Failed to deferred spawn Actor of Class [{}] and Archetype [{}]!"),
        InSpawnActorParams.Get_ActorClass(),
        DeferredSpawnActorParams.Get_Archetype())
    { return {}; }

    // If we want a replicated actor, the actor must be set to Replicate, and vice-versa.
    const auto& ReplicationMatches = (IsReplicated && SpawningActor->GetIsReplicated()) ||
        (NOT IsReplicated && NOT SpawningActor->GetIsReplicated());

    if (NOT IgnoreCertainChecksIfNotInGame)
    {
        CK_ENSURE_IF_NOT(ReplicationMatches,
            TEXT("Requested NetworkingType [{}] does not match the spawningActor [{}] replication settings"),
            Replication,
            SpawningActor)
        {
            SpawningActor->Destroy();
            return {};
        }
    }

    return SpawningActor;
}

auto
    UCk_Utils_Actor_UE::
    DoRequest_SpawnActor_Finish(
        const SpawnActorParamsType& InSpawnActorParams,
        AActor* InNewlySpawnedActor)
    -> AActor*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNewlySpawnedActor),
        TEXT("Newly spawned Actor [{}] is INVALID (or about to be destroyed"), InNewlySpawnedActor)
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
    DoRequest_CopyAllActorComponentProperties(
        AActor* InSourceActor,
        AActor* InDestinationActor)
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
                FCk_Utils_Object_CopyAllProperties_Params{}
                .Set_Destination(InDestinationActorComponent)
                .Set_Source(InSourceActorComponent)
            );
        });
    });
}

// --------------------------------------------------------------------------------------------------------------------
