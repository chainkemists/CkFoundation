#include "CkIskm_BatchedUtils.h"

#include "CkIskm_BatchedClusterComponent.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

UCk_Iskm_BatchedClusterComponent*
    UCk_Utils_IskmBatched_UE::
    Debug_SpawnCluster(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        const FTransform& InBaseTransform,
        int32 InGridSize,
        float InSpacing,
        int32 InFrameIndex)
{
    if (ck::Is_NOT_Valid(InWorldContextObject) || ck::Is_NOT_Valid(InCollection))
    { return nullptr; }

    UWorld* World = GEngine != nullptr
        ? GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (World == nullptr)
    { return nullptr; }

    USkeletalMesh* Mesh = InCollection->Get_DefaultMesh();
    if (ck::Is_NOT_Valid(Mesh))
    { return nullptr; }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), InBaseTransform, SpawnParams);
    if (Actor == nullptr)
    { return nullptr; }

    USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
    Actor->SetRootComponent(Root);
    Root->RegisterComponent();
    Root->SetWorldTransform(InBaseTransform);

    UCk_Iskm_BatchedClusterComponent* Cluster = NewObject<UCk_Iskm_BatchedClusterComponent>(Actor, TEXT("BatchedCluster"));
    Cluster->SetupAttachment(Root);
    Cluster->RegisterComponent();
    Cluster->Setup(InCollection, Mesh);

    const int32 N = FMath::Max(1, InGridSize);
    const float Center = static_cast<float>(N - 1) * 0.5f;

    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Instances;
    Instances.Reserve(N * N);
    for (int32 X = 0; X < N; ++X)
    {
        for (int32 Y = 0; Y < N; ++Y)
        {
            UCk_Iskm_BatchedClusterComponent::FInstance Inst;
            Inst.Transform = FTransform(FVector((static_cast<float>(X) - Center) * InSpacing,
                                                (static_cast<float>(Y) - Center) * InSpacing, 0.0f));
            Inst.CurFrame = InFrameIndex;
            Inst.PrevFrame = InFrameIndex;
            Instances.Add(Inst);
        }
    }
    Cluster->Set_Instances(Instances);

    return Cluster;
}

int32
    UCk_Utils_IskmBatched_UE::
    Get_InstanceCount(const UCk_Iskm_BatchedClusterComponent* InCluster)
{
    if (ck::Is_NOT_Valid(InCluster))
    { return 0; }
    return InCluster->Get_Instances().Num();
}
