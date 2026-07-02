#include "CkIskm_BatchedCrowd_Actor.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_BakedPose.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"

ACk_Iskm_BatchedCrowd_Actor::ACk_Iskm_BatchedCrowd_Actor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
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

FBox
    ACk_Iskm_BatchedCrowd_Actor::
    TileLocalBounds() const
{
    // Conservative fixed bounds, component-relative (component sits at the tile centre): the tile's XY extent
    // padded by the mesh box (instances stand anywhere in the tile; the mesh sticks out at most its own box),
    // Z from the mesh box padded by its own size (animation headroom until C4's baked animated bounds).
    const float Half = _TileSize * 0.5f;
    FBox MeshBox(FVector(-50.0), FVector(50.0));
    if (_Collection != nullptr)
    {
        if (USkeletalMesh* Mesh = _Collection->Get_DefaultMesh())
        { MeshBox = Mesh->GetBounds().GetBox(); }
    }
    const FVector MeshSize = MeshBox.GetSize();
    const FVector Pad(FMath::Max(MeshSize.X, MeshSize.Y));
    return FBox(
        FVector(-Half - Pad.X, -Half - Pad.Y, MeshBox.Min.Z - MeshSize.Z),
        FVector(+Half + Pad.X, +Half + Pad.Y, MeshBox.Max.Z + MeshSize.Z));
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
    Comp->Set_ManagedExternally(true);        // the manager advances animation + pushes per-frame data
    Comp->Set_FixedLocalBounds(TileLocalBounds());

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
    Member.Inst.PrevPushedTransform = Member.Inst.Transform;
    Member.Inst.SequenceIndex = InSequenceIndex;
    Member.Inst.Rate = InRate;
    Member.Inst.Time = InTimeOffset;
    Member.Inst.CurFrame = 0;
    Member.Inst.PrevFrame = 0;

    const int32 MemberIndex = _Members.Add(Member);
    _TileMembers.FindOrAdd(Tile).Add(MemberIndex);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    RebuildTile(const FIntPoint& InTile)
{
    UCk_Iskm_BatchedClusterComponent* Comp = _Tiles.FindRef(InTile);
    if (Comp == nullptr)
    { return; }

    // Members carry LIVE animation state (the manager is the single source of truth), so a rebuild never
    // snaps animation back — this is what fixes the old "whole tile resets to spawn pose on any flip" bug.
    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Visible;
    if (const TArray<int32>* MemberIndices = _TileMembers.Find(InTile))
    {
        Visible.Reserve(MemberIndices->Num());
        for (const int32 MemberIndex : *MemberIndices)
        {
            const FMember& M = _Members[MemberIndex];
            if (M.bVisible)
            { Visible.Add(M.Inst); }
        }
    }
    Comp->Set_Instances(Visible);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    PushTile(const FIntPoint& InTile)
{
    UCk_Iskm_BatchedClusterComponent* Comp = _Tiles.FindRef(InTile);
    if (Comp == nullptr)
    { return; }
    const TArray<int32>* MemberIndices = _TileMembers.Find(InTile);
    if (MemberIndices == nullptr)
    { return; }

    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Visible;
    Visible.Reserve(MemberIndices->Num());
    for (const int32 MemberIndex : *MemberIndices)
    {
        const FMember& M = _Members[MemberIndex];
        if (M.bVisible)
        { Visible.Add(M.Inst); }
    }

    // Count changes must recreate the proxy (Set_Instances); visibility/migration already do that at write
    // time, so a mismatch here means a bookkeeping bug — recover via the heavy path.
    if (Visible.Num() == Comp->Get_Instances().Num())
    { Comp->Push_LiveInstances(MoveTemp(Visible)); }
    else
    {
        CK_TRIGGER_ENSURE(TEXT("[CkIskm] PushTile count mismatch ({} members visible vs {} instances) — tile bookkeeping bug, recovering via Set_Instances"),
            Visible.Num(), Comp->Get_Instances().Num());
        Comp->Set_Instances(Visible);
    }

    // Roll velocity history: next frame's "previous transform" is what we just pushed.
    for (const int32 MemberIndex : *MemberIndices)
    {
        FMember& M = _Members[MemberIndex];
        M.Inst.PrevPushedTransform = M.Inst.Transform;
    }
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Finalize()
{
    for (const auto& Pair : _Tiles)
    {
        RebuildTile(Pair.Key);
    }
    _DirtyTiles.Reset();
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Tick(float InDeltaTime)
{
    Super::Tick(InDeltaTime);

    // Rendering is client-local; a dedicated server has nothing to feed.
    if (GetNetMode() == NM_DedicatedServer)
    { return; }
    if (_Collection == nullptr || _Members.Num() == 0)
    { return; }
    const FCk_Iskm_BakedPose* Baked = _Collection->Get_BakedPose();
    if (Baked == nullptr)
    { return; }

    // Advance ALL members (hidden ones too, so they rejoin in phase after a flip-demote).
    for (FMember& M : _Members)
    {
        if (M.Inst.Rate == 0.0f)
        { continue; }
        M.Inst.Time += InDeltaTime * M.Inst.Rate;
        const int32 NewFrame = Baked->Get_LoopedFrameAtTime(M.Inst.SequenceIndex, M.Inst.Time);
        if (NewFrame != M.Inst.CurFrame)
        {
            M.Inst.PrevFrame = M.Inst.CurFrame;
            M.Inst.CurFrame = NewFrame;
            if (M.bVisible)
            { _DirtyTiles.Add(M.Tile); }
        }
    }

    for (const FIntPoint& Tile : _DirtyTiles)
    {
        PushTile(Tile);
    }
    _DirtyTiles.Reset();
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
    Set_MemberTransform(int32 InIndex, const FTransform& InWorldTransform)
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    FMember& M = _Members[InIndex];
    M.WorldXf = InWorldTransform;

    const FIntPoint NewTile = TileCoordOf(InWorldTransform.GetLocation());
    if (NewTile == M.Tile)
    {
        // In-tile move: transform flows through the light per-frame push. PrevPushedTransform still holds the
        // last pushed value, so this move produces a real motion vector.
        M.Inst.Transform = InWorldTransform.GetRelativeTransform(FTransform(TileCentre(M.Tile)));
        if (M.bVisible)
        { _DirtyTiles.Add(M.Tile); }
        return;
    }

    // Tile migration: rebuild both tiles (instance counts change on each).
    const FIntPoint OldTile = M.Tile;
    if (TArray<int32>* OldList = _TileMembers.Find(OldTile))
    { OldList->RemoveSingleSwap(InIndex); }

    M.Tile = NewTile;
    M.Inst.Transform = InWorldTransform.GetRelativeTransform(FTransform(TileCentre(NewTile)));
    // Different proxy = different motion-vector space; zero this instance's velocity for one frame instead of
    // smearing between two primitives' transforms.
    M.Inst.PrevPushedTransform = M.Inst.Transform;

    if (GetOrCreate_Tile(NewTile) != nullptr)
    { _TileMembers.FindOrAdd(NewTile).Add(InIndex); }

    RebuildTile(OldTile);
    RebuildTile(NewTile);
    _DirtyTiles.Remove(OldTile);
    _DirtyTiles.Remove(NewTile);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberAnimation(int32 InIndex, int32 InSequenceIndex, float InRate, bool bInResetTime)
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    FMember& M = _Members[InIndex];
    M.Inst.SequenceIndex = InSequenceIndex;
    M.Inst.Rate = InRate;
    if (bInResetTime)
    { M.Inst.Time = 0.0f; }
    if (M.bVisible)
    { _DirtyTiles.Add(M.Tile); }
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberCustomData(int32 InIndex, float InA, float InB)
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    FMember& M = _Members[InIndex];
    M.Inst.CustomDataA = InA;
    M.Inst.CustomDataB = InB;
    if (M.bVisible)
    { _DirtyTiles.Add(M.Tile); }
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
    _DirtyTiles.Remove(_Members[InIndex].Tile);
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
