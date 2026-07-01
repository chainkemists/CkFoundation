#include "CkIskm_BatchedUtils.h"

#include "CkIskm_BatchedClusterComponent.h"
#include "CkIskm_BatchedCrowd_Actor.h"
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
        int32 InSequenceIndex,
        float InRate)
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
            Inst.SequenceIndex = InSequenceIndex;
            Inst.Rate = InRate;
            Inst.Time = static_cast<float>(X * N + Y) * 0.137f; // per-instance phase offset for out-of-sync looping
            Inst.CurFrame = 0; // recomputed each tick from Time
            Inst.PrevFrame = 0;
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

ACk_Iskm_BatchedCrowd_Actor*
    UCk_Utils_IskmBatched_UE::
    Debug_SpawnScatteredCrowd(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        const FTransform& InBaseTransform,
        int32 InNumInstances,
        float InAreaExtent,
        float InTileSize,
        int32 InSequenceIndex,
        float InRate)
{
    if (ck::Is_NOT_Valid(InWorldContextObject) || ck::Is_NOT_Valid(InCollection))
    { return nullptr; }

    UWorld* World = GEngine != nullptr
        ? GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (World == nullptr)
    { return nullptr; }

    if (ck::Is_NOT_Valid(InCollection->Get_DefaultMesh()))
    { return nullptr; }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACk_Iskm_BatchedCrowd_Actor* Crowd = World->SpawnActor<ACk_Iskm_BatchedCrowd_Actor>(
        ACk_Iskm_BatchedCrowd_Actor::StaticClass(), InBaseTransform, SpawnParams);
    if (Crowd == nullptr)
    { return nullptr; }

    Crowd->Initialize(InCollection, InTileSize);

    // Deterministic scatter: an even grid spanning the 2*extent square, so instances fall across many tiles.
    const int32 N = FMath::Max(1, InNumInstances);
    const int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(N))));
    const float Span = 2.0f * InAreaExtent;
    const float Step = (Cols > 1) ? Span / static_cast<float>(Cols - 1) : 0.0f;
    const FVector Base = InBaseTransform.GetLocation();

    int32 Placed = 0;
    for (int32 Row = 0; Row < Cols && Placed < N; ++Row)
    {
        for (int32 Col = 0; Col < Cols && Placed < N; ++Col)
        {
            const FVector Pos = Base + FVector(
                -InAreaExtent + static_cast<float>(Col) * Step,
                -InAreaExtent + static_cast<float>(Row) * Step,
                0.0f);
            const FTransform Xf(FRotator::ZeroRotator, Pos, FVector::OneVector);

            // InSequenceIndex < 0 => cycle idle/walk/jog per instance so per-instance animation is visually obvious
            // (idle alone is too subtle to read the phase offsets). Assumes the AnimCollection_Demo layout
            // (0=Idle, 2=Walk, 3=Jog; index 1 = the non-looping Jump is skipped). >= 0 => that sequence for all.
            int32 Seq = InSequenceIndex;
            if (InSequenceIndex < 0)
            {
                const int32 VariedSeqs[3] = { 0, 2, 3 };
                Seq = VariedSeqs[Placed % 3];
            }

            const float TimeOffset = static_cast<float>(Placed) * 0.137f;
            Crowd->AddInstance(Xf, Seq, InRate, TimeOffset);
            ++Placed;
        }
    }

    Crowd->Finalize();
    return Crowd;
}

int32
    UCk_Utils_IskmBatched_UE::
    Get_CrowdTileCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_TileCount();
}

int32
    UCk_Utils_IskmBatched_UE::
    Get_CrowdInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_InstanceCount();
}

int32
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_MemberCount();
}

FTransform
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberTransform(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return FTransform::Identity; }
    return InCrowd->Get_MemberWorldTransform(InIndex);
}

int32
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberSequenceIndex(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_MemberSequenceIndex(InIndex);
}

void
    UCk_Utils_IskmBatched_UE::
    Set_CrowdMemberVisible(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, bool bInVisible)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_MemberVisible(InIndex, bInVisible);
}

int32
    UCk_Utils_IskmBatched_UE::
    Get_CrowdRenderedInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_RenderedInstanceCount();
}
