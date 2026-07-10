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

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Initialize(UCk_IskmAnimCollection_Data* InCollection, float InTileSize)
    -> void
{
    _Collection = InCollection;
    _TileSize = FMath::Max(1.0f, InTileSize);

    // Bake up front (CPU-only, headless-safe, idempotent): tile fixed bounds derive from the baked ANIMATED
    // pose extent — tiles created pre-bake would otherwise freeze the smaller static mesh box.
    if (_Collection != nullptr && _Collection->Get_IsBaked() == false)
    { _Collection->Build_BakedPoseData(); }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    TileCoordOf(const FVector& InWorldLocation) const
    -> FIntPoint
{
    return FIntPoint(FMath::FloorToInt(InWorldLocation.X / _TileSize),
                     FMath::FloorToInt(InWorldLocation.Y / _TileSize));
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    TileCentre(const FIntPoint& InTile) const
    -> FVector
{
    return FVector((static_cast<double>(InTile.X) + 0.5) * _TileSize,
                   (static_cast<double>(InTile.Y) + 0.5) * _TileSize,
                   0.0);
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    TileLocalBounds() const
    -> FBox
{
    // Conservative fixed bounds, component-relative (component sits at the tile centre): the tile's XY extent
    // padded by the ANIMATED pose box (instances stand anywhere in the tile; an animated mesh sticks out at
    // most its baked animated bounds), Z straight from the animated box.
    const float Half = _TileSize * 0.5f;
    const FBox MeshBox = (_Collection != nullptr) ? _Collection->Get_AnimatedMeshBounds() : FBox(FVector(-50.0), FVector(50.0));
    const FVector MeshSize = MeshBox.GetSize();
    const float PadXY = FMath::Max(MeshSize.X, MeshSize.Y);
    return FBox(
        FVector(-Half - PadXY, -Half - PadXY, MeshBox.Min.Z),
        FVector(+Half + PadXY, +Half + PadXY, MeshBox.Max.Z));
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    GetOrCreate_Tile(const FIntPoint& InTile)
    -> UCk_Iskm_BatchedClusterComponent*
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

    for (int32 Idx = 0; Idx < _DefaultTileCustomPrimitiveData.Num(); ++Idx)
    { Comp->SetCustomPrimitiveDataFloat(Idx, _DefaultTileCustomPrimitiveData[Idx]); }

    if (_OverrideMaterial != nullptr)
    { Comp->Set_OverrideMaterial(_OverrideMaterial); }

    if (_SlotOverrideMaterials.Num() > 0)
    {
        TArray<UMaterialInterface*> Slots;
        Slots.Reserve(_SlotOverrideMaterials.Num());
        for (const auto& M : _SlotOverrideMaterials) { Slots.Add(M.Get()); }
        Comp->Set_SlotOverrideMaterials(Slots);
    }

    _Tiles.Add(InTile, Comp);
    return Comp;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_OverrideMaterial(UMaterialInterface* InMaterial)
    -> void
{
    _OverrideMaterial = InMaterial;

    for (const auto& Pair : _Tiles)
    {
        if (Pair.Value == nullptr)
        { continue; }
        Pair.Value->Set_OverrideMaterial(InMaterial);
    }
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_SlotOverrideMaterials(const TArray<UMaterialInterface*>& InMaterials)
{
    _SlotOverrideMaterials.Reset(InMaterials.Num());
    for (UMaterialInterface* M : InMaterials)
    { _SlotOverrideMaterials.Add(M); }

    for (const auto& Pair : _Tiles)
    {
        if (Pair.Value == nullptr)
        { continue; }
        Pair.Value->Set_SlotOverrideMaterials(InMaterials);
    }
}

UMaterialInterface*
    ACk_Iskm_BatchedCrowd_Actor::
    Get_SlotOverrideMaterial(int32 InSlotIndex) const
{
    return _SlotOverrideMaterials.IsValidIndex(InSlotIndex) ? _SlotOverrideMaterials[InSlotIndex].Get() : nullptr;
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_DefaultTileCustomPrimitiveData(const TArray<float>& InFloats)
{
    _DefaultTileCustomPrimitiveData = InFloats;

    for (const auto& Pair : _Tiles)
    {
        if (Pair.Value == nullptr)
        { continue; }
        for (int32 Idx = 0; Idx < _DefaultTileCustomPrimitiveData.Num(); ++Idx)
        { Pair.Value->SetCustomPrimitiveDataFloat(Idx, _DefaultTileCustomPrimitiveData[Idx]); }
    }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    AddInstance(const FTransform& InWorldTransform, int32 InSequenceIndex, float InRate, float InTimeOffset)
    -> void
{
    const FIntPoint Tile = TileCoordOf(InWorldTransform.GetLocation());
    if (GetOrCreate_Tile(Tile) == nullptr)
    { return; }

    // Component-relative transform: the tile component sits at the tile centre with identity rotation/scale.
    const FTransform TileXf(TileCentre(Tile));

    FMember Member;
    Member.WorldXf = InWorldTransform;
    Member.Tile = Tile;
    Member.Visible = true;
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

auto
    ACk_Iskm_BatchedCrowd_Actor::
    RebuildTile(const FIntPoint& InTile)
    -> void
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
            if (M.Visible)
            { Visible.Add(M.Inst); }
        }
    }
    Comp->Set_Instances(Visible);
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    PushTile(const FIntPoint& InTile)
    -> void
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
        if (M.Visible)
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

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Finalize()
    -> void
{
    for (const auto& Pair : _Tiles)
    {
        RebuildTile(Pair.Key);
    }
    _DirtyTiles.Reset();
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Tick(float InDeltaTime)
    -> void
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
            if (M.Visible)
            { _DirtyTiles.Add(M.Tile); }
        }
    }

    for (const FIntPoint& Tile : _DirtyTiles)
    {
        PushTile(Tile);
    }
    _DirtyTiles.Reset();
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberWorldTransform(int32 InIndex) const
    -> FTransform
{
    return _Members.IsValidIndex(InIndex) ? _Members[InIndex].WorldXf : FTransform::Identity;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberSequenceIndex(int32 InIndex) const
    -> int32
{
    return _Members.IsValidIndex(InIndex) ? _Members[InIndex].Inst.SequenceIndex : 0;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberVisible(int32 InIndex) const
    -> bool
{
    return _Members.IsValidIndex(InIndex) ? _Members[InIndex].Visible : false;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberTransform(int32 InIndex, const FTransform& InWorldTransform)
    -> void
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
        if (M.Visible)
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

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberAnimation(int32 InIndex, int32 InSequenceIndex, float InRate, bool InResetTime)
    -> void
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    FMember& M = _Members[InIndex];
    M.Inst.SequenceIndex = InSequenceIndex;
    M.Inst.Rate = InRate;
    if (InResetTime)
    { M.Inst.Time = 0.0f; }
    if (M.Visible)
    { _DirtyTiles.Add(M.Tile); }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberCustomData(int32 InIndex, float InA, float InB)
    -> void
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    FMember& M = _Members[InIndex];
    M.Inst.UserData[0] = InA;
    M.Inst.UserData[1] = InB;
    if (M.Visible)
    { _DirtyTiles.Add(M.Tile); }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberCustomData(int32 InIndex, int32 InFirstFloat, const TArray<float>& InValues)
    -> void
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    CK_ENSURE_IF_NOT(InFirstFloat >= 2 &&
        InFirstFloat + InValues.Num() <= UCk_Iskm_BatchedClusterComponent::NumCustomDataFloats,
        TEXT("[CkIskm] Member custom-data write [{}..{}) is outside the game floats [2..{}) — "
             "floats [0]/[1] are the animation frame bits"),
        InFirstFloat, InFirstFloat + InValues.Num(), UCk_Iskm_BatchedClusterComponent::NumCustomDataFloats)
    { return; }

    FMember& M = _Members[InIndex];
    for (int32 K = 0; K < InValues.Num(); ++K)
    { M.Inst.UserData[InFirstFloat - 2 + K] = InValues[K]; }
    if (M.Visible)
    { _DirtyTiles.Add(M.Tile); }
}

float
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberCustomData(int32 InIndex, int32 InFloatIndex) const
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InFloatIndex >= 2 && InFloatIndex < UCk_Iskm_BatchedClusterComponent::NumCustomDataFloats,
        TEXT("[CkIskm] Member custom-data read [{}] is outside the game floats [2..{})"),
        InFloatIndex, UCk_Iskm_BatchedClusterComponent::NumCustomDataFloats)
    { return 0.0f; }

    return _Members[InIndex].Inst.UserData[InFloatIndex - 2];
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberVisible(int32 InIndex, bool InVisible)
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }
    if (_Members[InIndex].Visible == InVisible)
    { return; }
    _Members[InIndex].Visible = InVisible;
    RebuildTile(_Members[InIndex].Tile);
    _DirtyTiles.Remove(_Members[InIndex].Tile);
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_RenderedInstanceCount() const
    -> int32
{
    int32 Total = 0;
    for (const auto& Pair : _Tiles)
    {
        if (Pair.Value != nullptr)
        { Total += Pair.Value->Get_Instances().Num(); }
    }
    return Total;
}
