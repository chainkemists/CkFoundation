#include "CkIskm_BatchedCrowd_Actor.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_BakedPose.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Processor.h" // FFragment_IskmCrowd_Controller

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"        // Get_TransientEntity
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h" // Request_CreateEntity

#include "CkEcsExt/Transform/CkTransform_Utils.h"         // Request_SetTransform (far cosmetic placement)

#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

ACk_Iskm_BatchedCrowd_Actor::ACk_Iskm_BatchedCrowd_Actor()
{
    // the actor no longer self-ticks — FProcessor_IskmCrowd_Advance drives AdvanceAnimation on
    // the ECS clock so member render + far cosmetics resolve the same frame.
    PrimaryActorTick.bCanEverTick = false;
    _Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(_Root);

#if WITH_EDITORONLY_DATA
    // Runtime cache actor derived from an AnimCollection — never user-facing (renderer-actor parity).
    bListedInSceneOutliner = false;
#endif
}

#if WITH_EDITOR
auto
    ACk_Iskm_BatchedCrowd_Actor::
    IsSelectionChild() const
    -> bool
{
    return _EditorSelectionOwner.IsValid();
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    GetSelectionParent() const
    -> AActor*
{
    return _EditorSelectionOwner.Get();
}
#endif

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Initialize(UCk_IskmAnimCollection_Data* InCollection, float InTileSize)
    -> void
{
    _Collection = InCollection;
    _TileSize = FMath::Max(1.0f, InTileSize);

    // Bake up front (CPU-only, headless-safe, idempotent): tile fixed bounds derive from the baked ANIMATED
    // pose extent — tiles created pre-bake would otherwise freeze the smaller static mesh box. A stale bake
    // (sequences mutated after it was cached) rebuilds, releasing any stale render data along the way.
    if (ck::IsValid(_Collection) && (NOT _Collection->Get_IsBaked() || _Collection->Get_IsBakeStale()))
    { _Collection->Build_BakedPoseData(); }

    // stand up this crowd's ECS presence so FProcessor_IskmCrowd_Advance can drive it on the
    // ECS clock. Owned by the transient entity (session lifetime — the crowd is never destroyed mid-
    // session; the world teardown clears it). No ECS world (edge case) => no controller; the crowd simply
    // won't self-advance, and the ensure below flags it.
    if (ck::Is_NOT_Valid(_ControllerEntity))
    {
        auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());
        CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
            TEXT("Iskm crowd [{}]: no ECS transient entity — crowd will not advance (no controller)"), this)
        { return; }

        _ControllerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(TransientEntity);
        _ControllerEntity.Add<ck::FFragment_IskmCrowd_Controller>(this);
    }
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
    AdvanceAnimation(float InDeltaTime)
    -> void
{
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

    // Highlight clusters mirror their members' live pose every tick (groups are tiny — a handful of
    // outlined members at most; per-tick push is cheaper than tracking per-group dirtiness).
    for (auto& Pair : _OutlineGroups)
    {
        PushOutlineGroup(Pair.Value);
    }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    DriveCosmetics()
    -> void
{
    if (_MemberCosmetics.Num() == 0)
    { return; }

    // Runs right after AdvanceAnimation in the same FGroup_Transform_SyncFrom processor tick, reading the
    // SAME _Members snapshot that PushTile just rendered — so each cosmetic is queued at the member's
    // CURRENT frame/world. Request_SetTransform is the framework's deferred write (safe cross-entity — the
    // same call the old flip driver used): HandleRequests applies + tags it later THIS tick, so the
    // PostTransform render flush picks it up the same frame the member is PushTiled — lockstep, no trail.
    for (auto It = _MemberCosmetics.CreateIterator(); It; ++It)
    {
        const int32              MemberIndex = It.Key();
        TArray<FMemberCosmetic>& Cosmetics   = It.Value();

        for (int32 Idx = Cosmetics.Num() - 1; Idx >= 0; --Idx)
        {
            FMemberCosmetic& C = Cosmetics[Idx];
            if (ck::Is_NOT_Valid(C.Cosmetic))
            { Cosmetics.RemoveAtSwap(Idx); continue; }   // cosmetic destroyed — prune

            FTransform SocketWorld;
            if (TryGet_MemberSocketTransform(MemberIndex, C.Socket, SocketWorld) == false)
            { continue; }   // no baked socket / bad index — leave it parked

            UCk_Utils_Transform_UE::Request_SetTransform(
                C.Cosmetic, FCk_Request_Transform_SetTransform{C.RelOffset * SocketWorld});
        }

        if (Cosmetics.Num() == 0)
        { It.RemoveCurrent(); }
    }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Register_MemberCosmetic(int32 InIndex, const FCk_Handle_Transform& InCosmetic, FName InSocket, const FTransform& InRelOffset)
    -> void
{
    if (_Members.IsValidIndex(InIndex) == false || ck::Is_NOT_Valid(InCosmetic))
    { return; }

    TArray<FMemberCosmetic>& List = _MemberCosmetics.FindOrAdd(InIndex);
    for (FMemberCosmetic& C : List)
    {
        if (C.Cosmetic == InCosmetic)
        { C.Socket = InSocket; C.RelOffset = InRelOffset; return; }   // replace-if-same-entity
    }

    FMemberCosmetic New;
    New.Cosmetic  = InCosmetic;
    New.Socket    = InSocket;
    New.RelOffset = InRelOffset;
    List.Add(New);
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Clear_MemberCosmetics(int32 InIndex)
    -> void
{
    _MemberCosmetics.Remove(InIndex);
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

        // Outlined member walking out of its highlight cluster's fixed bounds → rebuild (recomputes bounds).
        if (const auto* OutlinePreset = _MemberOutlines.Find(InIndex);
            OutlinePreset != nullptr)
        {
            if (const auto* Group = _OutlineGroups.Find(*OutlinePreset);
                Group != nullptr && NOT Group->PaddedBounds.IsInsideOrOn(InWorldTransform.GetLocation()))
            { RebuildOutlineGroup(OutlinePreset->Get()); }
        }
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

    if (const auto* OutlinePreset = _MemberOutlines.Find(InIndex);
        OutlinePreset != nullptr)
    {
        if (const auto* Group = _OutlineGroups.Find(*OutlinePreset);
            Group != nullptr && NOT Group->PaddedBounds.IsInsideOrOn(InWorldTransform.GetLocation()))
        { RebuildOutlineGroup(OutlinePreset->Get()); }
    }
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

auto
    ACk_Iskm_BatchedCrowd_Actor::
    TryGet_MemberSocketTransform(int32 InIndex, FName InSocket, FTransform& OutWorld) const
    -> bool
{
    if (_Members.IsValidIndex(InIndex) == false || _Collection == nullptr)
    { return false; }
    const FCk_Iskm_BakedPose* Baked = _Collection->Get_BakedPose();
    if (Baked == nullptr)
    { return false; }
    const FCk_Iskm_BakedSocket* Socket = Baked->Find_Socket(InSocket);
    if (Socket == nullptr || Socket->FrameTransforms.Num() == 0)
    { return false; }

    const FMember& M = _Members[InIndex];
    const int32 Frame = FMath::Clamp(M.Inst.CurFrame, 0, Socket->FrameTransforms.Num() - 1);
    OutWorld = static_cast<FTransform>(Socket->FrameTransforms[Frame]) * M.WorldXf;
    return true;
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

    // Hidden members leave their highlight cluster too (their Plan-1 stand-in gets outlined via the
    // entity API); re-shown members rejoin.
    if (const auto* OutlinePreset = _MemberOutlines.Find(InIndex);
        OutlinePreset != nullptr)
    { RebuildOutlineGroup(OutlinePreset->Get()); }
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

// --------------------------------------------------------------------------------------------------------------------
// Entity outline (member-indexed) — see CkUsf/DESIGN_EntityOutlines.md.

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberOutline(int32 InIndex, UCkUsf_OutlinePreset* InPreset)
{
    if (_Members.IsValidIndex(InIndex) == false)
    { return; }

    if (InPreset == nullptr)
    {
        Clear_MemberOutline(InIndex);
        return;
    }

    if (const auto* Existing = _MemberOutlines.Find(InIndex);
        Existing != nullptr)
    {
        if (*Existing == InPreset)
        { return; }
        Clear_MemberOutline(InIndex); // preset change: leave the old group first
    }

    auto* Group = _OutlineGroups.Find(InPreset);
    if (Group == nullptr)
    {
        // First member with this preset: allocate the preset's stencil (one refcount per group) and
        // stand up the highlight cluster at the world origin (instance transforms = member world xf).
        auto* World = GetWorld();
        auto* OutlineSubsystem = ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{})
            ? World->GetSubsystem<UCkUsf_OutlineSubsystem>()
            : nullptr;

        CK_ENSURE_IF_NOT(OutlineSubsystem != nullptr,
            TEXT("[CkIskm] Set_MemberOutline: no CkUsf outline subsystem available"))
        { return; }

        const auto Stencil = OutlineSubsystem->Get_OrAllocate_StencilFor(InPreset);
        if (Stencil == 0)
        { return; } // stencil range exhausted — already warned by the subsystem

        USkeletalMesh* Mesh = (_Collection != nullptr) ? _Collection->Get_DefaultMesh() : nullptr;

        CK_ENSURE_IF_NOT(Mesh != nullptr,
            TEXT("[CkIskm] Set_MemberOutline: crowd has no collection/mesh"))
        {
            OutlineSubsystem->Release_StencilFor(InPreset);
            return;
        }

        auto* Comp = NewObject<UCk_Iskm_BatchedClusterComponent>(this,
            MakeUniqueObjectName(this, UCk_Iskm_BatchedClusterComponent::StaticClass(), TEXT("IskmOutlineCluster")));
        Comp->SetupAttachment(_Root);
        // Instances AND fixed bounds are authored in WORLD space, so the component must stay at
        // identity regardless of the crowd actor's own transform. Absolute flags + identity (not
        // just SetWorldLocation) — an inherited actor rotation would rotate the far-from-origin
        // bounds box to the wrong side of the world and the cluster frustum-culls away at some
        // view angles even though the members are on screen. (Tiles are immune: their local box
        // is centered on the component, so rotation maps it onto itself.)
        Comp->SetUsingAbsoluteLocation(true);
        Comp->SetUsingAbsoluteRotation(true);
        Comp->SetUsingAbsoluteScale(true);
        Comp->RegisterComponent();
        Comp->SetWorldTransform(FTransform::Identity);
        Comp->Setup(_Collection, Mesh);
        Comp->Set_ManagedExternally(true);
        Comp->CastShadow = false;
        // Custom-depth-only: the silhouette source for the SolidOutline post-process.
        Comp->bRenderInMainPass = false;
        Comp->SetRenderCustomDepth(true);
        Comp->SetCustomDepthStencilValue(static_cast<int32>(Stencil));

        auto NewGroup = FOutlineGroup{};
        NewGroup.Comp = Comp;
        NewGroup.Stencil = Stencil;
        Group = &_OutlineGroups.Add(InPreset, MoveTemp(NewGroup));
    }

    Group->Members.AddUnique(InIndex);
    _MemberOutlines.Add(InIndex, InPreset);
    RebuildOutlineGroup(InPreset);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Clear_MemberOutline(int32 InIndex)
{
    const auto* PresetPtr = _MemberOutlines.Find(InIndex);
    if (PresetPtr == nullptr)
    { return; }

    auto* Preset = PresetPtr->Get();
    _MemberOutlines.Remove(InIndex);

    auto* Group = _OutlineGroups.Find(Preset);
    if (Group == nullptr)
    { return; }

    Group->Members.RemoveSingleSwap(InIndex);

    if (Group->Members.Num() > 0)
    {
        RebuildOutlineGroup(Preset);
        return;
    }

    // Last member left: release the group's stencil refcount and drop the cluster.
    if (auto* Comp = Group->Comp.Get())
    { Comp->DestroyComponent(); }

    if (auto* World = GetWorld();
        ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    {
        if (auto* OutlineSubsystem = World->GetSubsystem<UCkUsf_OutlineSubsystem>())
        { OutlineSubsystem->Release_StencilFor(Preset); }
    }

    _OutlineGroups.Remove(Preset);
}

UCkUsf_OutlinePreset*
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberOutlinePreset(int32 InIndex) const
{
    const auto* PresetPtr = _MemberOutlines.Find(InIndex);
    return PresetPtr != nullptr ? PresetPtr->Get() : nullptr;
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_OutlinedMemberCount() const
{
    return _MemberOutlines.Num();
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_OutlineRenderedInstanceCount() const
{
    int32 Total = 0;
    for (const auto& Pair : _OutlineGroups)
    {
        if (const auto* Comp = Pair.Value.Comp.Get())
        { Total += Comp->Get_Instances().Num(); }
    }
    return Total;
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    RebuildOutlineGroup(UCkUsf_OutlinePreset* InPreset)
{
    auto* Group = _OutlineGroups.Find(InPreset);
    if (Group == nullptr)
    { return; }
    auto* Comp = Group->Comp.Get();
    if (Comp == nullptr)
    { return; }

    // Component sits at the world origin with identity rotation/scale → instance transforms are the
    // members' world transforms directly. Custom depth has no velocity output, so Prev == Current.
    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Visible;
    auto PositionBounds = FBox{ForceInit};
    Visible.Reserve(Group->Members.Num());
    for (const int32 MemberIndex : Group->Members)
    {
        const FMember& M = _Members[MemberIndex];
        if (M.Visible == false)
        { continue; }

        auto Inst = M.Inst;
        Inst.Transform = M.WorldXf;
        Inst.PrevPushedTransform = M.WorldXf;
        Visible.Add(Inst);
        PositionBounds += M.WorldXf.GetLocation();
    }

    // Fixed conservative bounds: member-position union padded by the animated pose box (a member sticks
    // out at most that much) + half a tile of walk-slack so ordinary movement doesn't churn rebuilds.
    if (PositionBounds.IsValid)
    {
        const FBox MeshBox = (_Collection != nullptr) ? _Collection->Get_AnimatedMeshBounds() : FBox(FVector(-50.0), FVector(50.0));
        const FVector MeshSize = MeshBox.GetSize();
        const float Pad = FMath::Max(MeshSize.X, MeshSize.Y) + _TileSize * 0.5f;

        Group->PaddedBounds = FBox(
            PositionBounds.Min + FVector(-Pad, -Pad, MeshBox.Min.Z - MeshSize.Z),
            PositionBounds.Max + FVector(+Pad, +Pad, MeshBox.Max.Z + MeshSize.Z));
        Comp->Set_FixedLocalBounds(Group->PaddedBounds); // component at origin ⇒ local == world
    }

    Comp->Set_Instances(Visible);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    PushOutlineGroup(FOutlineGroup& InGroup)
{
    auto* Comp = InGroup.Comp.Get();
    if (Comp == nullptr)
    { return; }

    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Visible;
    Visible.Reserve(InGroup.Members.Num());
    for (const int32 MemberIndex : InGroup.Members)
    {
        const FMember& M = _Members[MemberIndex];
        if (M.Visible == false)
        { continue; }

        auto Inst = M.Inst;
        Inst.Transform = M.WorldXf;
        Inst.PrevPushedTransform = M.WorldXf;
        Visible.Add(Inst);
    }

    // Count changes always flow through RebuildOutlineGroup at write time — a mismatch here is a
    // bookkeeping bug; recover via the heavy path (mirrors PushTile).
    if (Visible.Num() == Comp->Get_Instances().Num())
    { Comp->Push_LiveInstances(MoveTemp(Visible)); }
    else
    {
        CK_TRIGGER_ENSURE(TEXT("[CkIskm] PushOutlineGroup count mismatch ({} members visible vs {} instances) — outline bookkeeping bug, recovering via Set_Instances"),
            Visible.Num(), Comp->Get_Instances().Num());
        Comp->Set_Instances(Visible);
    }
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    EndPlay(const EEndPlayReason::Type InEndPlayReason)
{
    // Release the outline groups' stencil refcounts (components die with the actor).
    if (auto* World = GetWorld();
        ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    {
        if (auto* OutlineSubsystem = World->GetSubsystem<UCkUsf_OutlineSubsystem>())
        {
            for (const auto& Pair : _OutlineGroups)
            {
                if (auto* Preset = Pair.Key.Get())
                { OutlineSubsystem->Release_StencilFor(Preset); }
            }
        }
    }
    _OutlineGroups.Reset();
    _MemberOutlines.Reset();

    DoDestroy_ControllerEntity();

    Super::EndPlay(InEndPlayReason);
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Destroyed()
    -> void
{
    // Editor-world actors never BeginPlay, so EndPlay never fires for them — an editor preview
    // crowd destroyed by the per-owner reclaim would otherwise leak its controller entity (a dead
    // fragment FProcessor_IskmCrowd_Advance iterates forever). Runtime destroys hit EndPlay first;
    // the cleanup is idempotent.
    DoDestroy_ControllerEntity();

    Super::Destroyed();
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    DoDestroy_ControllerEntity()
    -> void
{
    if (ck::Is_NOT_Valid(_ControllerEntity))
    { return; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_ControllerEntity);
    _ControllerEntity = FCk_Handle{};
}
