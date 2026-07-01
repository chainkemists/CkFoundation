#include "CkIskm_BatchedCrowd_Actor.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"

ACk_Iskm_BatchedCrowd_Actor::ACk_Iskm_BatchedCrowd_Actor()
{
    PrimaryActorTick.bCanEverTick = false;
    _Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(_Root);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Initialize(UCk_IskmAnimCollection_Data* InCollection, float InTileSize)
{
    _Collection = InCollection;
    _TileSize = FMath::Max(1.0f, InTileSize);
}

FIntPoint
    ACk_Iskm_BatchedCrowd_Actor::
    TileCoordOf(const FVector& InWorldLocation) const
{
    return FIntPoint(FMath::FloorToInt(InWorldLocation.X / _TileSize),
                     FMath::FloorToInt(InWorldLocation.Y / _TileSize));
}

FVector
    ACk_Iskm_BatchedCrowd_Actor::
    TileCentre(const FIntPoint& InTile) const
{
    return FVector((static_cast<double>(InTile.X) + 0.5) * _TileSize,
                   (static_cast<double>(InTile.Y) + 0.5) * _TileSize,
                   0.0);
}

UCk_Iskm_BatchedClusterComponent*
    ACk_Iskm_BatchedCrowd_Actor::
    GetOrCreate_Tile(const FIntPoint& InTile)
{
    if (const TObjectPtr<UCk_Iskm_BatchedClusterComponent>* Found = _Tiles.Find(InTile))
    { return *Found; }

    if (_Collection == nullptr)
    { return nullptr; }
    USkeletalMesh* Mesh = _Collection->Get_DefaultMesh();
    if (Mesh == nullptr)
    { return nullptr; }

    UCk_Iskm_BatchedClusterComponent* Comp = NewObject<UCk_Iskm_BatchedClusterComponent>(this);
    Comp->SetupAttachment(_Root);
    Comp->RegisterComponent();
    Comp->SetWorldLocation(TileCentre(InTile));
    Comp->Setup(_Collection, Mesh);

    _Tiles.Add(InTile, Comp);
    return Comp;
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    AddInstance(const FTransform& InWorldTransform, int32 InSequenceIndex, float InRate, float InTimeOffset)
{
    const FIntPoint Tile = TileCoordOf(InWorldTransform.GetLocation());
    if (GetOrCreate_Tile(Tile) == nullptr)
    { return; }

    // Component-relative transform: the tile component sits at the tile centre with identity rotation/scale.
    const FTransform TileXf(TileCentre(Tile));

    FMember Member;
    Member.WorldXf = InWorldTransform;
    Member.Tile = Tile;
    Member.bVisible = true;
    Member.Inst.Transform = InWorldTransform.GetRelativeTransform(TileXf);
    Member.Inst.SequenceIndex = InSequenceIndex;
    Member.Inst.Rate = InRate;
    Member.Inst.Time = InTimeOffset;
    Member.Inst.CurFrame = 0;
    Member.Inst.PrevFrame = 0;

    _Members.Add(Member);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    RebuildTile(const FIntPoint& InTile)
{
    UCk_Iskm_BatchedClusterComponent* Comp = _Tiles.FindRef(InTile);
    if (Comp == nullptr)
    { return; }

    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Visible;
    for (const FMember& M : _Members)
    {
        if (M.Tile == InTile && M.bVisible)
        { Visible.Add(M.Inst); }
    }
    Comp->Set_Instances(Visible);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Finalize()
{
    for (const auto& Pair : _Tiles)
    {
        RebuildTile(Pair.Key);
    }
}

FTransform
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberWorldTransform(int32 InIndex) const
{
    return _Members.IsValidIndex(InIndex) ? _Members[InIndex].WorldXf : FTransform::Identity;
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberSequenceIndex(int32 InIndex) const
{
    return _Members.IsValidIndex(InIndex) ? _Members[InIndex].Inst.SequenceIndex : 0;
}

bool
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberVisible(int32 InIndex) const
{
    return _Members.IsValidIndex(InIndex) ? _Members[InIndex].bVisible : false;
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberVisible(int32 InIndex, bool bInVisible)
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }
    if (_Members[InIndex].bVisible == bInVisible)
    { return; }
    _Members[InIndex].bVisible = bInVisible;
    RebuildTile(_Members[InIndex].Tile);
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_RenderedInstanceCount() const
{
    int32 Total = 0;
    for (const auto& Pair : _Tiles)
    {
        if (Pair.Value != nullptr)
        { Total += Pair.Value->Get_Instances().Num(); }
    }
    return Total;
}
