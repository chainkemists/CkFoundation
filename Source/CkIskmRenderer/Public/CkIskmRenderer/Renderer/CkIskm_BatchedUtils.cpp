#include "CkIskm_BatchedUtils.h"

#include "CkIskm_BatchedClusterComponent.h"
#include "CkIskm_BatchedCrowd_Actor.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

auto
    UCk_Utils_IskmBatched_UE::
    Debug_SpawnCluster(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        const FTransform& InBaseTransform,
        int32 InGridSize,
        float InSpacing,
        int32 InSequenceIndex,
        float InRate)
    -> UCk_Iskm_BatchedClusterComponent*
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

auto
    UCk_Utils_IskmBatched_UE::
    Get_InstanceCount(const UCk_Iskm_BatchedClusterComponent* InCluster)
    -> int32
{
    if (ck::Is_NOT_Valid(InCluster))
    { return 0; }
    return InCluster->Get_Instances().Num();
}

auto
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
    -> ACk_Iskm_BatchedCrowd_Actor*
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

auto
    UCk_Utils_IskmBatched_UE::
    Get_CrowdTileCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
    -> int32
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_TileCount();
}

auto
    UCk_Utils_IskmBatched_UE::
    Get_CrowdInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
    -> int32
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_InstanceCount();
}

auto
    UCk_Utils_IskmBatched_UE::
    Create_Crowd(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        float InTileSize)
    -> ACk_Iskm_BatchedCrowd_Actor*
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
        ACk_Iskm_BatchedCrowd_Actor::StaticClass(), FTransform::Identity, SpawnParams);
    if (Crowd == nullptr)
    { return nullptr; }

    Crowd->Initialize(InCollection, InTileSize);
    return Crowd;
}

auto
    UCk_Utils_IskmBatched_UE::
    Add_CrowdMember(
        ACk_Iskm_BatchedCrowd_Actor* InCrowd,
        const FTransform& InWorldTransform,
        int32 InSequenceIndex,
        float InRate,
        float InTimeOffset)
    -> int32
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return INDEX_NONE; }

    InCrowd->AddInstance(InWorldTransform, InSequenceIndex, InRate, InTimeOffset);
    return InCrowd->Get_MemberCount() - 1;
}

auto
    UCk_Utils_IskmBatched_UE::
    Finalize_Crowd(
        ACk_Iskm_BatchedCrowd_Actor* InCrowd)
    -> void
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }

    InCrowd->Finalize();
}

auto
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
    -> int32
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_MemberCount();
}

auto
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberTransform(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex)
    -> FTransform
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return FTransform::Identity; }
    return InCrowd->Get_MemberWorldTransform(InIndex);
}

auto
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberSequenceIndex(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex)
    -> int32
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_MemberSequenceIndex(InIndex);
}

auto
    UCk_Utils_IskmBatched_UE::
    Set_CrowdMemberVisible(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, bool InVisible)
    -> void
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_MemberVisible(InIndex, InVisible);
}

auto
    UCk_Utils_IskmBatched_UE::
    Get_CrowdRenderedInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd)
    -> int32
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0; }
    return InCrowd->Get_RenderedInstanceCount();
}

auto
    UCk_Utils_IskmBatched_UE::
    Set_CrowdMemberTransform(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, const FTransform& InWorldTransform)
    -> void
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_MemberTransform(InIndex, InWorldTransform);
}

auto
    UCk_Utils_IskmBatched_UE::
    Set_CrowdMemberAnimation(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, int32 InSequenceIndex, float InRate, bool InResetTime)
    -> void
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_MemberAnimation(InIndex, InSequenceIndex, InRate, InResetTime);
}

auto
    UCk_Utils_IskmBatched_UE::
    Set_CrowdMemberCustomData(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, float InA, float InB)
    -> void
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_MemberCustomData(InIndex, InA, InB);
}

void
    UCk_Utils_IskmBatched_UE::
    Set_CrowdMemberCustomDataFloats(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, int32 InFirstFloat, const TArray<float>& InValues)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_MemberCustomData(InIndex, InFirstFloat, InValues);
}

float
    UCk_Utils_IskmBatched_UE::
    Get_CrowdMemberCustomDataFloat(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, int32 InFloatIndex)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return 0.0f; }
    return InCrowd->Get_MemberCustomData(InIndex, InFloatIndex);
}

void
    UCk_Utils_IskmBatched_UE::
    Set_CrowdDefaultCustomPrimitiveData(ACk_Iskm_BatchedCrowd_Actor* InCrowd, const TArray<float>& InFloats)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_DefaultTileCustomPrimitiveData(InFloats);
}

void
    UCk_Utils_IskmBatched_UE::
    Set_CrowdOverrideMaterial(ACk_Iskm_BatchedCrowd_Actor* InCrowd, UMaterialInterface* InMaterial)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_OverrideMaterial(InMaterial);
}

void
    UCk_Utils_IskmBatched_UE::
    Set_CrowdSlotOverrideMaterials(ACk_Iskm_BatchedCrowd_Actor* InCrowd, const TArray<UMaterialInterface*>& InMaterials)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return; }
    InCrowd->Set_SlotOverrideMaterials(InMaterials);
}

UMaterialInterface*
    UCk_Utils_IskmBatched_UE::
    Get_CrowdSlotOverrideMaterial(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InSlotIndex)
{
    if (ck::Is_NOT_Valid(InCrowd))
    { return nullptr; }
    return InCrowd->Get_SlotOverrideMaterial(InSlotIndex);
}
