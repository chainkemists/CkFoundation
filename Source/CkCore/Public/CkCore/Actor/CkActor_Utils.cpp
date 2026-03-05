#include "CkActor_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/CkCoreLog.h"
#include "CkCore/ObjectReplication/CkReplicatedObject.h"

#include <CoreMinimal.h>
#include <EngineUtils.h>

#include <Components/InstancedStaticMeshComponent.h>
#include <Components/SkeletalMeshComponent.h>
#include <Components/StaticMeshComponent.h>

#include <Engine/BlueprintGeneratedClass.h>
#include <Engine/NetDriver.h>
#include <Engine/World.h>

#include <GameFramework/Pawn.h>

#include <HAL/IConsoleManager.h>
#include <Kismet/GameplayStatics.h>
#include <UObject/UObjectIterator.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto
        ForEachGameOrPIEWorld(
            const TFunction<void(UWorld*)>& InFunc)
        -> void
    {
        for (TObjectIterator<UWorld> It; It; ++It)
        {
            auto* World = *It;

            if (NOT ck::IsValid(World))
            { continue; }

            if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
            { continue; }

            InFunc(World);
        }
    }
}

namespace ck::actor_utils_net_dump
{
    static TAutoConsoleVariable<int32> CVar_TopClassCount(
        TEXT("ck.net.dump.TopClassCount"),
        20,
        TEXT("Top class count to print for replicated actor/component/object dump commands."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_TopOwnerCount(
        TEXT("ck.net.dump.TopOwnerCount"),
        20,
        TEXT("Top owner count to print for replicated component/object/summary dump commands."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_LogEachObject(
        TEXT("ck.net.dump.LogEachObject"),
        1,
        TEXT("Whether object dump command logs each object row.\n0 = Summary only, 1 = Include per-object rows"),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_LogEachActor(
        TEXT("ck.net.dump.LogEachActor"),
        1,
        TEXT("Whether actor dump command logs each actor row.\n0 = Summary only, 1 = Include per-actor rows"),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_LogEachComponent(
        TEXT("ck.net.dump.LogEachComponent"),
        1,
        TEXT("Whether component dump command logs each component row.\n0 = Summary only, 1 = Include per-component rows"),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandReplicatedObjects(
        TEXT("ck.net.dump.CommandReplicatedObjects"),
        0,
        TEXT("Set to 1 to dump replicated objects in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            const auto TopClassCount = FMath::Clamp(CVar_TopClassCount.GetValueOnGameThread(), 1, 1024);
            const auto TopOwnerCount = FMath::Clamp(CVar_TopOwnerCount.GetValueOnGameThread(), 1, 1024);
            const auto bLogEachObject = CVar_LogEachObject.GetValueOnGameThread() != 0;

            ForEachGameOrPIEWorld([&](UWorld* InWorld)
            {
                UCk_Utils_Actor_UE::Request_LogAllReplicatedObjects(InWorld, bLogEachObject, TopClassCount, TopOwnerCount);
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.dump.CommandReplicatedObjects"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandReplicatedActors(
        TEXT("ck.net.dump.CommandReplicatedActors"),
        0,
        TEXT("Set to 1 to dump replicated actors in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            const auto TopClassCount = FMath::Clamp(CVar_TopClassCount.GetValueOnGameThread(), 1, 1024);
            const auto bLogEachActor = CVar_LogEachActor.GetValueOnGameThread() != 0;

            ForEachGameOrPIEWorld([&](UWorld* InWorld)
            {
                UCk_Utils_Actor_UE::Request_LogAllReplicatedActors(InWorld, bLogEachActor, TopClassCount);
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.dump.CommandReplicatedActors"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandReplicatedComponents(
        TEXT("ck.net.dump.CommandReplicatedComponents"),
        0,
        TEXT("Set to 1 to dump replicated components in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            const auto TopClassCount = FMath::Clamp(CVar_TopClassCount.GetValueOnGameThread(), 1, 1024);
            const auto TopOwnerCount = FMath::Clamp(CVar_TopOwnerCount.GetValueOnGameThread(), 1, 1024);
            const auto bLogEachComponent = CVar_LogEachComponent.GetValueOnGameThread() != 0;

            ForEachGameOrPIEWorld([&](UWorld* InWorld)
            {
                UCk_Utils_Actor_UE::Request_LogAllReplicatedComponents(InWorld, bLogEachComponent, TopClassCount, TopOwnerCount);
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.dump.CommandReplicatedComponents"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVar_CommandReplicationSummary(
        TEXT("ck.net.dump.CommandReplicationSummary"),
        0,
        TEXT("Set to 1 to dump replicated actor/component/object summary in all active game worlds."),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* InVar)
        {
            if (InVar->GetInt() <= 0)
            { return; }

            const auto TopClassCount = FMath::Clamp(CVar_TopClassCount.GetValueOnGameThread(), 1, 1024);
            const auto TopOwnerCount = FMath::Clamp(CVar_TopOwnerCount.GetValueOnGameThread(), 1, 1024);

            ForEachGameOrPIEWorld([&](UWorld* InWorld)
            {
                UCk_Utils_Actor_UE::Request_LogReplicationSummary(InWorld, TopClassCount, TopClassCount, TopOwnerCount);
            });

            if (auto* ResetVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.net.dump.CommandReplicationSummary"));
                ResetVar != nullptr)
            {
                ResetVar->Set(0, ECVF_SetByCode);
            }
        }),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Actor_SpawnActor_Params::
    FCk_Utils_Actor_SpawnActor_Params(
        UObject* InOwnerOrWorld,
        // ReSharper disable once CppPassValueParameterByConstReference
        TSubclassOf<AActor> InActorClass)
    :_OwnerOrWorld(std::move(InOwnerOrWorld))
    , _ActorClass(InActorClass)
{
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Utils_Actor_UE::DeferredSpawnActor_Params::
    DeferredSpawnActor_Params(
        // ReSharper disable once CppPassValueParameterByConstReference
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
        if (auto MaybePawn = Cast<APawn>(OuterObject);
            ck::IsValid(MaybePawn, ck::IsValid_Policy_IncludePendingKill{}))
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
        if (auto MaybeActor = Cast<AActor>(OuterObject);
            ck::IsValid(MaybeActor, ck::IsValid_Policy_IncludePendingKill{}))
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
        if (auto MaybeActor = Cast<AActor>(OuterObject);
            ck::IsValid(MaybeActor, ck::IsValid_Policy_IncludePendingKill{}))
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
    Get_ComponentsByClassFromClass(
        TSubclassOf<AActor> InActorClass,
        TSubclassOf<UActorComponent> ComponentClass)
    -> TArray<UActorComponent*>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorClass), TEXT("ActorClass [{}] supplied is Invalid"), InActorClass)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(ComponentClass), TEXT("ComponentClass [{}] supplied is Invalid"), ComponentClass)
    { return {}; }

    auto ActorClass = Cast<UBlueprintGeneratedClass>(InActorClass);

    if (ck::Is_NOT_Valid(ActorClass))
    { return {}; }

    auto Components = TArray<const UActorComponent*>{};
    AActor::GetActorClassDefaultComponents(InActorClass, ComponentClass, Components);

    auto NonConstComponents = TArray<UActorComponent*>{};
    NonConstComponents.Reserve(Components.Num());

    for (const auto Component : Components)
    {
        NonConstComponents.Add(const_cast<UActorComponent*>(Component));
    }

    return NonConstComponents;
}

auto
    UCk_Utils_Actor_UE::
    Request_CloneActor(
        const UObject* InWorldContextObject,
        AActor* InOwner,
        AActor* InActorToClone,
        const ESpawnActorCollisionHandlingMethod InCollisionHandlingOverride,
        const ECk_Utils_Actor_SpawnActorPolicy InSpawnPolicy)
    -> AActor*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorToClone), TEXT("Failed to Clone Actor because it is invalid!"))
    { return {}; }

    if (ck::Is_NOT_Valid(InWorldContextObject))
    {
        InWorldContextObject = InOwner;
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
    Get_HasComponent_ByClass(
        AActor* InActor,
        // ReSharper disable once CppPassValueParameterByConstReference
        TSubclassOf<UActorComponent> InComponent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to UCk_Utils_Actor_UE::Get_HasComponent_ByClass"))
    { return {}; }

    if (ck::IsValid(InActor->FindComponentByClass(InComponent)))
    { return true;}

    auto HasBlueprintComponent = false;
    AActor::ForEachComponentOfActorClassDefault(InActor->GetClass(), InComponent, [&](const UActorComponent* TemplateComponent)
    {
        if (TemplateComponent->IsA(InComponent))
        {
            HasBlueprintComponent = true;
            return HasBlueprintComponent;
        }

        return false;
    });

    return HasBlueprintComponent;
}

auto
    UCk_Utils_Actor_UE::
    Get_HasComponent_ByInterface(
        AActor* InActor,
        // ReSharper disable once CppPassValueParameterByConstReference
        TSubclassOf<UInterface> InInterface)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to UCk_Utils_Actor_UE::Get_HasComponent_ByInterface"))
    { return {}; }

    if (ck::IsValid(InActor->FindComponentByInterface(InInterface)))
    { return true;}

    auto HasBlueprintComponent = false;
    AActor::ForEachComponentOfActorClassDefault(InActor->GetClass(), UActorComponent::StaticClass(), [&](const UActorComponent* TemplateComponent)
    {
        if (TemplateComponent->GetClass()->ImplementsInterface(InInterface))
        {
            HasBlueprintComponent = true;
            return HasBlueprintComponent;
        }

        return false;
    });

    return HasBlueprintComponent;
}

auto
    UCk_Utils_Actor_UE::
    Get_HasBegunPlay(
        AActor* InActor)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to UCk_Utils_Actor_UE::Get_HasBegunPlay"))
    { return {}; }

    return InActor->HasActorBegunPlay();
}

auto
    UCk_Utils_Actor_UE::
    Get_DoesBoneExistInSkeletalMesh(
        AActor* InActor,
        FName InBoneName)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to UCk_Utils_Actor_UE::Get_HasBoneInSkeletonMesh"))
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
    Get_SocketTransform(
        AActor* InActor,
        FName InSocketName,
        ERelativeTransformSpace InTransformSpace,
        ECk_ActorTraversalPolicy InTraversalPolicy)
    -> TArray<FCk_Utils_Actor_SocketTransforms>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to UCk_Utils_Actor_UE::Get_SocketTransform"))
    { return {}; }

    auto Ret = TArray<FCk_Utils_Actor_SocketTransforms>{};

    const auto& GatherSocketTransforms = [&](AActor* Actor)
    {
        auto MeshComponents = TArray<UMeshComponent*>{};
        Actor->GetComponents<UMeshComponent>(MeshComponents);

        for (const auto MeshComponent : MeshComponents)
        {
            if (NOT MeshComponent->DoesSocketExist(InSocketName))
            { continue; }

            if (const auto InstancedStaticMesh = Cast<UInstancedStaticMeshComponent>(MeshComponent))
            {
                const auto& SocketTransform = InstancedStaticMesh->GetSocketTransform(InSocketName, InTransformSpace);

                for (auto Index = 0; Index < InstancedStaticMesh->GetInstanceCount(); ++Index)
                {
                    auto InstanceTransform = FTransform{};
                    InstancedStaticMesh->GetInstanceTransform(Index, InstanceTransform);

                    Ret.Emplace(FCk_Utils_Actor_SocketTransforms{Actor, MeshComponent, InstanceTransform * SocketTransform, InSocketName});
                }

                continue;
            }

            Ret.Emplace(FCk_Utils_Actor_SocketTransforms{Actor, MeshComponent, MeshComponent->GetSocketTransform(InSocketName, InTransformSpace), InSocketName});
        }
    };

    GatherSocketTransforms(InActor);

    if (InTraversalPolicy == ECk_ActorTraversalPolicy::RootActorAndAllAttachedActors)
    {
        auto AttachedActors = TArray<AActor*>{};
        InActor->GetAttachedActors(AttachedActors);

        for (const auto& AttachedActor : AttachedActors)
        {
            if (ck::Is_NOT_Valid(AttachedActor))
            { continue; }

            GatherSocketTransforms(AttachedActor);
        }
    }

    return Ret;
}

auto
    UCk_Utils_Actor_UE::
    Get_SocketTransform_Exec(
        AActor* InActor,
        FName InSocketName,
        ERelativeTransformSpace InTransformSpace,
        ECk_ActorTraversalPolicy InTraversalPolicy)
    -> TArray<FCk_Utils_Actor_SocketTransforms>
{
    return Get_SocketTransform(InActor, InSocketName, InTransformSpace, InTraversalPolicy);
}

auto
    UCk_Utils_Actor_UE::
    Get_SocketTransform_UsingPrefix(
        AActor* InActor,
        FName InSocketNamePrefix,
        TEnumAsByte<ESearchCase::Type> InSearchCase,
        ERelativeTransformSpace InTransformSpace,
        ECk_ActorTraversalPolicy InTraversalPolicy)
    -> TArray<FCk_Utils_Actor_SocketTransforms>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to UCk_Utils_Actor_UE::Get_SocketTransform_UsingPrefix"))
    { return {}; }

    auto Ret = TArray<FCk_Utils_Actor_SocketTransforms>{};
    const auto& SocketPrefixStr = InSocketNamePrefix.ToString();

    const auto& GatherSocketTransformsWithPrefix = [&](AActor* Actor)
    {
        auto MeshComponents = TArray<UMeshComponent*>{};
        Actor->GetComponents<UMeshComponent>(MeshComponents);

        for (const auto MeshComponent : MeshComponents)
        {
            for (const auto& SocketName : MeshComponent->GetAllSocketNames())
            {
                if (NOT SocketName.ToString().StartsWith(SocketPrefixStr, InSearchCase))
                { continue; }

                if (const auto InstancedStaticMesh = Cast<UInstancedStaticMeshComponent>(MeshComponent))
                {
                    const auto& SocketTransform = InstancedStaticMesh->GetSocketTransform(SocketName, InTransformSpace);

                    for (auto Index = 0; Index < InstancedStaticMesh->GetInstanceCount(); ++Index)
                    {
                        auto InstanceTransform = FTransform{};
                        InstancedStaticMesh->GetInstanceTransform(Index, InstanceTransform);

                        Ret.Emplace(FCk_Utils_Actor_SocketTransforms{Actor, MeshComponent, InstanceTransform * SocketTransform, SocketName});
                    }

                    continue;
                }

                Ret.Emplace(FCk_Utils_Actor_SocketTransforms{Actor, MeshComponent, MeshComponent->GetSocketTransform(SocketName, InTransformSpace), SocketName});
            }
        }
    };

    GatherSocketTransformsWithPrefix(InActor);

    if (InTraversalPolicy == ECk_ActorTraversalPolicy::RootActorAndAllAttachedActors)
    {
        auto AttachedActors = TArray<AActor*>{};
        InActor->GetAttachedActors(AttachedActors);

        for (const auto& AttachedActor : AttachedActors)
        {
            if (ck::Is_NOT_Valid(AttachedActor))
            { continue; }

            GatherSocketTransformsWithPrefix(AttachedActor);
        }
    }

    return Ret;
}

auto
    UCk_Utils_Actor_UE::
    Get_SocketTransform_UsingPrefix_Exec(
        AActor* InActor,
        FName InSocketNamePrefix,
        TEnumAsByte<ESearchCase::Type> InSearchCase,
        ERelativeTransformSpace InTransformSpace,
        ECk_ActorTraversalPolicy InTraversalPolicy)
    -> TArray<FCk_Utils_Actor_SocketTransforms>
{
    return Get_SocketTransform_UsingPrefix(InActor, InSocketNamePrefix, InSearchCase, InTransformSpace, InTraversalPolicy);
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
    Request_RerunConstructionScript(
        AActor* InActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Actor [{}] is INVALID. Unable to Rerun ConstructionScript"), InActor)
    { return; }

#if WITH_EDITOR
    InActor->RerunConstructionScripts();
#endif
}

auto
    UCk_Utils_Actor_UE::
    Get_AllReplicatedActors(
        const UObject* InWorldContextObject)
    -> TArray<AActor*>
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to gather replicated actors because world is invalid for context [{}]."), InWorldContextObject)
    { return {}; }

    auto ReplicatedActors = TArray<AActor*>{};

    for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
    {
        auto* Actor = *ActorIt;

        if (ck::IsValid(Actor) && Actor->GetIsReplicated())
        {
            ReplicatedActors.Add(Actor);
        }
    }

    return ReplicatedActors;
}

auto
    UCk_Utils_Actor_UE::
    Get_AllReplicatedObjects(
        const UObject* InWorldContextObject)
    -> TArray<UCk_ReplicatedObject_UE*>
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to gather replicated objects because world is invalid for context [{}]."), InWorldContextObject)
    { return {}; }

    auto ReplicatedObjects = TArray<UCk_ReplicatedObject_UE*>{};

    for (TObjectIterator<UCk_ReplicatedObject_UE> It; It; ++It)
    {
        auto* ReplicatedObject = *It;

        if (NOT ck::IsValid(ReplicatedObject))
        { continue; }

        if (ReplicatedObject->IsTemplate())
        { continue; }

        if (ReplicatedObject->GetWorld() != World)
        { continue; }

        ReplicatedObjects.Add(ReplicatedObject);
    }

    return ReplicatedObjects;
}

auto
    UCk_Utils_Actor_UE::
    Get_AllReplicatedComponents(
        const UObject* InWorldContextObject)
    -> TArray<UActorComponent*>
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to gather replicated components because world is invalid for context [{}]."), InWorldContextObject)
    { return {}; }

    auto ReplicatedComponents = TArray<UActorComponent*>{};

    for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
    {
        auto* Actor = *ActorIt;
        if (NOT ck::IsValid(Actor))
        { continue; }

        auto Components = TArray<UActorComponent*>{};
        Actor->GetComponents(Components);

        for (auto* Component : Components)
        {
            if (ck::IsValid(Component) && Component->GetIsReplicated())
            {
                ReplicatedComponents.Add(Component);
            }
        }
    }

    return ReplicatedComponents;
}

auto
    UCk_Utils_Actor_UE::
    Request_LogAllReplicatedObjects(
        const UObject* InWorldContextObject,
        bool bInLogEachObject,
        int32 InTopClassCount,
        int32 InTopOwnerCount)
    -> void
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to log replicated objects because world is invalid for context [{}]."), InWorldContextObject)
    { return; }

    const auto ReplicatedObjects = Get_AllReplicatedObjects(InWorldContextObject);

    auto ObjectsByClass = TMap<UClass*, int32>{};
    auto ObjectsByOwner = TMap<AActor*, int32>{};
    auto ObjectsGroupedByOwner = TMap<AActor*, TArray<UCk_ReplicatedObject_UE*>>{};

    for (auto* ReplicatedObject : ReplicatedObjects)
    {
        if (NOT ck::IsValid(ReplicatedObject))
        { continue; }

        auto* Owner = ReplicatedObject->GetOwningActor();
        ObjectsByClass.FindOrAdd(ReplicatedObject->GetClass()) += 1;
        ObjectsByOwner.FindOrAdd(Owner) += 1;
        ObjectsGroupedByOwner.FindOrAdd(Owner).Add(ReplicatedObject);
    }

    auto ClassCounts = TArray<TPair<UClass*, int32>>{};
    ClassCounts.Reserve(ObjectsByClass.Num());
    for (const auto& Pair : ObjectsByClass)
    {
        ClassCounts.Add(Pair);
    }
    ClassCounts.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    auto OwnerCounts = TArray<TPair<AActor*, int32>>{};
    OwnerCounts.Reserve(ObjectsByOwner.Num());
    for (const auto& Pair : ObjectsByOwner)
    {
        OwnerCounts.Add(Pair);
    }
    OwnerCounts.Sort([](const TPair<AActor*, int32>& Lhs, const TPair<AActor*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    ck::core::Warning(TEXT("----- [Ck Replication] Replicated Object Dump BEGIN -----"));
    ck::core::Warning(TEXT("World=[{}] TotalReplicatedObjects=[{}]"), World, ReplicatedObjects.Num());

    const auto TopClassCount = FMath::Clamp(InTopClassCount, 1, 1024);
    const auto TopOwnerCount = FMath::Clamp(InTopOwnerCount, 1, 1024);

    for (auto Index = 0; Index < FMath::Min(TopClassCount, ClassCounts.Num()); ++Index)
    {
        const auto& [Class, Count] = ClassCounts[Index];
        ck::core::Warning(TEXT("TopObjectClass[{}] Count=[{}] Class=[{}]"), Index, Count, Class);
    }

    for (auto Index = 0; Index < FMath::Min(TopOwnerCount, OwnerCounts.Num()); ++Index)
    {
        const auto& [Owner, Count] = OwnerCounts[Index];
        ck::core::Warning(TEXT("TopObjectOwner[{}] Count=[{}] Owner=[{}]"), Index, Count, Owner);
    }

    if (bInLogEachObject)
    {
        ck::core::Warning(TEXT("[Hierarchy] Owners -> Replicated Objects"));

        auto PrintedOwnerCount = 0;
        auto PrintedObjectCount = 0;

        for (const auto& [Owner, Count] : OwnerCounts)
        {
            const auto* GroupPtr = ObjectsGroupedByOwner.Find(Owner);
            if (GroupPtr == nullptr)
            { continue; }

            auto OwnerObjects = *GroupPtr;
            OwnerObjects.Sort([](const UCk_ReplicatedObject_UE& Lhs, const UCk_ReplicatedObject_UE& Rhs)
            {
                const auto LhsPath = GetPathNameSafe(&Lhs);
                const auto RhsPath = GetPathNameSafe(&Rhs);
                return LhsPath.Compare(RhsPath, ESearchCase::IgnoreCase) < 0;
            });

            ck::core::Warning(TEXT("Owner[{}] Count=[{}] Owner=[{}]"), PrintedOwnerCount, Count, Owner);
            PrintedOwnerCount += 1;

            for (auto ObjectIndex = 0; ObjectIndex < OwnerObjects.Num(); ++ObjectIndex)
            {
                const auto* ReplicatedObject = OwnerObjects[ObjectIndex];

                ck::core::Warning(
                    TEXT("  Object[{}] Object=[{}] Class=[{}] Outer=[{}]"),
                    ObjectIndex,
                    ReplicatedObject,
                    ReplicatedObject ? ReplicatedObject->GetClass() : nullptr,
                    ReplicatedObject ? ReplicatedObject->GetOuter() : nullptr);

                PrintedObjectCount += 1;
            }
        }

        ck::core::Warning(TEXT("[Hierarchy] PrintedOwnerGroups=[{}] PrintedObjects=[{}]"), PrintedOwnerCount, PrintedObjectCount);
    }

    ck::core::Warning(TEXT("----- [Ck Replication] Replicated Object Dump END -----"));
}

auto
    UCk_Utils_Actor_UE::
    Request_LogAllReplicatedActors(
        const UObject* InWorldContextObject,
        bool bInLogEachActor,
        int32 InTopClassCount)
    -> void
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to log replicated actors because world is invalid for context [{}]."), InWorldContextObject)
    { return; }

    const auto ReplicatedActors = Get_AllReplicatedActors(InWorldContextObject);

    auto ActorsByClass = TMap<UClass*, int32>{};
    for (auto* ReplicatedActor : ReplicatedActors)
    {
        if (ck::IsValid(ReplicatedActor))
        {
            ActorsByClass.FindOrAdd(ReplicatedActor->GetClass()) += 1;
        }
    }

    auto ClassCounts = TArray<TPair<UClass*, int32>>{};
    ClassCounts.Reserve(ActorsByClass.Num());
    for (const auto& Pair : ActorsByClass)
    {
        ClassCounts.Add(Pair);
    }
    ClassCounts.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    ck::core::Warning(TEXT("----- [Ck Replication] Replicated Actor Dump BEGIN -----"));
    ck::core::Warning(TEXT("World=[{}] TotalReplicatedActors=[{}]"), World, ReplicatedActors.Num());

    const auto TopClassCount = FMath::Clamp(InTopClassCount, 1, 1024);

    for (auto Index = 0; Index < FMath::Min(TopClassCount, ClassCounts.Num()); ++Index)
    {
        const auto& [Class, Count] = ClassCounts[Index];
        ck::core::Warning(TEXT("TopActorClass[{}] Count=[{}] Class=[{}]"), Index, Count, Class);
    }

    if (bInLogEachActor)
    {
        for (auto Index = 0; Index < ReplicatedActors.Num(); ++Index)
        {
            const auto* ReplicatedActor = ReplicatedActors[Index];

            ck::core::Warning(
                TEXT("ReplicatedActor[{}] Actor=[{}] Class=[{}] Owner=[{}] NetDormancy=[{}]"),
                Index,
                ReplicatedActor,
                ReplicatedActor ? ReplicatedActor->GetClass() : nullptr,
                ReplicatedActor ? ReplicatedActor->GetOwner() : nullptr,
                ReplicatedActor ? static_cast<int32>(ReplicatedActor->NetDormancy) : -1);
        }
    }

    ck::core::Warning(TEXT("----- [Ck Replication] Replicated Actor Dump END -----"));
}

auto
    UCk_Utils_Actor_UE::
    Request_LogAllReplicatedComponents(
        const UObject* InWorldContextObject,
        bool bInLogEachComponent,
        int32 InTopClassCount,
        int32 InTopOwnerCount)
    -> void
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to log replicated components because world is invalid for context [{}]."), InWorldContextObject)
    { return; }

    const auto ReplicatedComponents = Get_AllReplicatedComponents(InWorldContextObject);

    auto ComponentsByClass = TMap<UClass*, int32>{};
    auto ComponentsByOwner = TMap<AActor*, int32>{};
    auto ComponentsGroupedByOwner = TMap<AActor*, TArray<UActorComponent*>>{};

    for (auto* ReplicatedComponent : ReplicatedComponents)
    {
        if (NOT ck::IsValid(ReplicatedComponent))
        { continue; }

        auto* Owner = ReplicatedComponent->GetOwner();
        ComponentsByClass.FindOrAdd(ReplicatedComponent->GetClass()) += 1;
        ComponentsByOwner.FindOrAdd(Owner) += 1;
        ComponentsGroupedByOwner.FindOrAdd(Owner).Add(ReplicatedComponent);
    }

    auto ClassCounts = TArray<TPair<UClass*, int32>>{};
    ClassCounts.Reserve(ComponentsByClass.Num());
    for (const auto& Pair : ComponentsByClass)
    {
        ClassCounts.Add(Pair);
    }
    ClassCounts.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    auto OwnerCounts = TArray<TPair<AActor*, int32>>{};
    OwnerCounts.Reserve(ComponentsByOwner.Num());
    for (const auto& Pair : ComponentsByOwner)
    {
        OwnerCounts.Add(Pair);
    }
    OwnerCounts.Sort([](const TPair<AActor*, int32>& Lhs, const TPair<AActor*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    ck::core::Warning(TEXT("----- [Ck Replication] Replicated Component Dump BEGIN -----"));
    ck::core::Warning(TEXT("World=[{}] TotalReplicatedComponents=[{}]"), World, ReplicatedComponents.Num());

    const auto TopClassCount = FMath::Clamp(InTopClassCount, 1, 1024);
    const auto TopOwnerCount = FMath::Clamp(InTopOwnerCount, 1, 1024);

    for (auto Index = 0; Index < FMath::Min(TopClassCount, ClassCounts.Num()); ++Index)
    {
        const auto& [Class, Count] = ClassCounts[Index];
        ck::core::Warning(TEXT("TopComponentClass[{}] Count=[{}] Class=[{}]"), Index, Count, Class);
    }

    for (auto Index = 0; Index < FMath::Min(TopOwnerCount, OwnerCounts.Num()); ++Index)
    {
        const auto& [Owner, Count] = OwnerCounts[Index];
        ck::core::Warning(TEXT("TopComponentOwner[{}] Count=[{}] Owner=[{}]"), Index, Count, Owner);
    }

    if (bInLogEachComponent)
    {
        ck::core::Warning(TEXT("[Hierarchy] Owners -> Replicated Components"));

        auto PrintedOwnerCount = 0;
        auto PrintedComponentCount = 0;

        for (const auto& [Owner, Count] : OwnerCounts)
        {
            const auto* GroupPtr = ComponentsGroupedByOwner.Find(Owner);
            if (GroupPtr == nullptr)
            { continue; }

            auto OwnerComponents = *GroupPtr;
            OwnerComponents.Sort([](const UActorComponent& Lhs, const UActorComponent& Rhs)
            {
                const auto LhsPath = GetPathNameSafe(&Lhs);
                const auto RhsPath = GetPathNameSafe(&Rhs);
                return LhsPath.Compare(RhsPath, ESearchCase::IgnoreCase) < 0;
            });

            ck::core::Warning(TEXT("Owner[{}] Count=[{}] Owner=[{}]"), PrintedOwnerCount, Count, Owner);
            PrintedOwnerCount += 1;

            for (auto ComponentIndex = 0; ComponentIndex < OwnerComponents.Num(); ++ComponentIndex)
            {
                const auto* ReplicatedComponent = OwnerComponents[ComponentIndex];

                ck::core::Warning(
                    TEXT("  Component[{}] Component=[{}] Class=[{}] Owner=[{}]"),
                    ComponentIndex,
                    ReplicatedComponent,
                    ReplicatedComponent ? ReplicatedComponent->GetClass() : nullptr,
                    ReplicatedComponent ? ReplicatedComponent->GetOwner() : nullptr);

                PrintedComponentCount += 1;
            }
        }

        ck::core::Warning(TEXT("[Hierarchy] PrintedOwnerGroups=[{}] PrintedComponents=[{}]"), PrintedOwnerCount, PrintedComponentCount);
    }

    ck::core::Warning(TEXT("----- [Ck Replication] Replicated Component Dump END -----"));
}

auto
    UCk_Utils_Actor_UE::
    Request_LogReplicationSummary(
        const UObject* InWorldContextObject,
        int32 InTopActorClassCount,
        int32 InTopObjectClassCount,
        int32 InTopObjectOwnerCount)
    -> void
{
    const auto World = UCk_Utils_Game_UE::Get_WorldForObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Unable to log replication summary because world is invalid for context [{}]."), InWorldContextObject)
    { return; }

    const auto ReplicatedActors     = Get_AllReplicatedActors(InWorldContextObject);
    const auto ReplicatedComponents = Get_AllReplicatedComponents(InWorldContextObject);
    const auto ReplicatedObjects    = Get_AllReplicatedObjects(InWorldContextObject);
    const auto* NetDriver           = World->GetNetDriver();

    auto ActorsByClass = TMap<UClass*, int32>{};
    auto ComponentsByClass = TMap<UClass*, int32>{};
    auto ObjectsByClass = TMap<UClass*, int32>{};
    auto ComponentsByOwner = TMap<AActor*, int32>{};
    auto ObjectsByOwner = TMap<AActor*, int32>{};

    for (auto* ReplicatedActor : ReplicatedActors)
    {
        if (ck::IsValid(ReplicatedActor))
        {
            ActorsByClass.FindOrAdd(ReplicatedActor->GetClass()) += 1;
        }
    }

    for (auto* ReplicatedObject : ReplicatedObjects)
    {
        if (NOT ck::IsValid(ReplicatedObject))
        { continue; }

        ObjectsByClass.FindOrAdd(ReplicatedObject->GetClass()) += 1;
        ObjectsByOwner.FindOrAdd(ReplicatedObject->GetOwningActor()) += 1;
    }

    for (auto* ReplicatedComponent : ReplicatedComponents)
    {
        if (NOT ck::IsValid(ReplicatedComponent))
        { continue; }

        ComponentsByClass.FindOrAdd(ReplicatedComponent->GetClass()) += 1;
        ComponentsByOwner.FindOrAdd(ReplicatedComponent->GetOwner()) += 1;
    }

    auto ActorClassCounts = TArray<TPair<UClass*, int32>>{};
    ActorClassCounts.Reserve(ActorsByClass.Num());
    for (const auto& Pair : ActorsByClass)
    {
        ActorClassCounts.Add(Pair);
    }
    ActorClassCounts.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    auto ObjectClassCounts = TArray<TPair<UClass*, int32>>{};
    ObjectClassCounts.Reserve(ObjectsByClass.Num());
    for (const auto& Pair : ObjectsByClass)
    {
        ObjectClassCounts.Add(Pair);
    }
    ObjectClassCounts.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    auto ComponentClassCounts = TArray<TPair<UClass*, int32>>{};
    ComponentClassCounts.Reserve(ComponentsByClass.Num());
    for (const auto& Pair : ComponentsByClass)
    {
        ComponentClassCounts.Add(Pair);
    }
    ComponentClassCounts.Sort([](const TPair<UClass*, int32>& Lhs, const TPair<UClass*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    auto ComponentOwnerCounts = TArray<TPair<AActor*, int32>>{};
    ComponentOwnerCounts.Reserve(ComponentsByOwner.Num());
    for (const auto& Pair : ComponentsByOwner)
    {
        ComponentOwnerCounts.Add(Pair);
    }
    ComponentOwnerCounts.Sort([](const TPair<AActor*, int32>& Lhs, const TPair<AActor*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    auto ObjectOwnerCounts = TArray<TPair<AActor*, int32>>{};
    ObjectOwnerCounts.Reserve(ObjectsByOwner.Num());
    for (const auto& Pair : ObjectsByOwner)
    {
        ObjectOwnerCounts.Add(Pair);
    }
    ObjectOwnerCounts.Sort([](const TPair<AActor*, int32>& Lhs, const TPair<AActor*, int32>& Rhs)
    {
        return Lhs.Value > Rhs.Value;
    });

    ck::core::Warning(TEXT("----- [Ck Replication] Replication Summary BEGIN -----"));
    ck::core::Warning(TEXT("World=[{}] NetMode=[{}] NetDriver=[{}] ClientConnections=[{}]"),
        World,
        static_cast<int32>(World->GetNetMode()),
        NetDriver,
        NetDriver ? NetDriver->ClientConnections.Num() : 0);
    ck::core::Warning(TEXT("TotalReplicatedActors=[{}] TotalReplicatedComponents=[{}] TotalReplicatedObjects=[{}]"),
        ReplicatedActors.Num(),
        ReplicatedComponents.Num(),
        ReplicatedObjects.Num());

    const auto TopActorClassCount  = FMath::Clamp(InTopActorClassCount, 1, 1024);
    const auto TopObjectClassCount = FMath::Clamp(InTopObjectClassCount, 1, 1024);
    const auto TopOwnerCount       = FMath::Clamp(InTopObjectOwnerCount, 1, 1024);

    for (auto Index = 0; Index < FMath::Min(TopActorClassCount, ActorClassCounts.Num()); ++Index)
    {
        const auto& [Class, Count] = ActorClassCounts[Index];
        ck::core::Warning(TEXT("TopActorClass[{}] Count=[{}] Class=[{}]"), Index, Count, Class);
    }

    for (auto Index = 0; Index < FMath::Min(TopObjectClassCount, ObjectClassCounts.Num()); ++Index)
    {
        const auto& [Class, Count] = ObjectClassCounts[Index];
        ck::core::Warning(TEXT("TopObjectClass[{}] Count=[{}] Class=[{}]"), Index, Count, Class);
    }

    for (auto Index = 0; Index < FMath::Min(TopObjectClassCount, ComponentClassCounts.Num()); ++Index)
    {
        const auto& [Class, Count] = ComponentClassCounts[Index];
        ck::core::Warning(TEXT("TopComponentClass[{}] Count=[{}] Class=[{}]"), Index, Count, Class);
    }

    for (auto Index = 0; Index < FMath::Min(TopOwnerCount, ComponentOwnerCounts.Num()); ++Index)
    {
        const auto& [Owner, Count] = ComponentOwnerCounts[Index];
        ck::core::Warning(TEXT("TopComponentOwner[{}] Count=[{}] Owner=[{}]"), Index, Count, Owner);
    }

    for (auto Index = 0; Index < FMath::Min(TopOwnerCount, ObjectOwnerCounts.Num()); ++Index)
    {
        const auto& [Owner, Count] = ObjectOwnerCounts[Index];
        ck::core::Warning(TEXT("TopObjectOwner[{}] Count=[{}] Owner=[{}]"), Index, Count, Owner);
    }

    ck::core::Warning(TEXT("----- [Ck Replication] Replication Summary END -----"));
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

    // If we want a replicated actor, the actor must be set to Replicate, and vice versa.
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
