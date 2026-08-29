#include "CkIskm_BatchedCrowd_Actor.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_BakedPose.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Processor.h" // FFragment_IskmCrowd_Controller

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"
#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"
#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"        // Get_TransientEntity
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h" // Request_CreateEntity

#include "CkEcsExt/Transform/CkTransform_Utils.h"         // Request_SetTransform (far cosmetic placement)

#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

#if WITH_EDITOR
namespace ck_iskm_batched_crowd
{
    static auto
        IsNonPieEditorWorld(const UWorld* InWorld)
        -> bool
    {
        return ck::IsValid(InWorld) &&
            (InWorld->WorldType == EWorldType::Editor || InWorld->WorldType == EWorldType::EditorPreview);
    }
}
#endif

ACk_Iskm_BatchedCrowd_Actor::ACk_Iskm_BatchedCrowd_Actor()
{
    // FProcessor_IskmCrowd_Advance drives AdvanceAnimation on the ECS clock instead of AActor::Tick.
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
    CK_ENSURE_IF_NOT(ck::IsValid(InCollection),
        TEXT("[CkIskm] Crowd [{}] Initialize with invalid AnimCollection — crowd will render nothing"), this)
    { return; }

    _Collection = InCollection;
    _TileSize = InTileSize;

    CK_ENSURE_IF_NOT(InTileSize >= 1.0f,
        TEXT("[CkIskm] Crowd [{}] Initialize with TileSize [{}] — sub-1cm (and non-positive) tile sizes are rejected; falling back to the default tile size"), this, InTileSize)
    { _TileSize = 2000.0f; }

    // Must bake BEFORE any tile exists: tile fixed bounds derive from the baked ANIMATED pose extent, and a
    // tile created pre-bake freezes the smaller static mesh box. CPU-only, headless-safe, idempotent.
    if (ck::IsValid(_Collection) && (NOT _Collection->Get_IsBaked() || _Collection->Get_IsBakeStale()))
    { _Collection->Build_BakedPoseData(); }

    // The crowd's ECS presence, owned by the transient entity (session lifetime). Without an ECS world there
    // is no controller and the crowd simply never self-advances.
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
    // Component-relative (the component sits at the tile centre): the tile's XY extent padded by the ANIMATED
    // pose box — an instance stands anywhere in the tile and sticks out at most its baked animated bounds.
    const float Half = _TileSize * 0.5f;
    FBox MeshBox = FBox(FVector(-50.0), FVector(50.0));
    if (ck::IsValid(_Collection))
    { MeshBox = _Collection->Get_AnimatedMeshBounds(); }
    else
    { CK_TRIGGER_ENSURE(TEXT("[CkIskm] TileLocalBounds on crowd [{}] with no AnimCollection — using a placeholder mesh box"), this); }
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

    CK_ENSURE_IF_NOT(ck::IsValid(_Collection),
        TEXT("[CkIskm] Crowd [{}] has no AnimCollection — was Initialize called?"), this)
    { return nullptr; }

    USkeletalMesh* Mesh = _Collection->Get_DefaultMesh();
    CK_ENSURE_IF_NOT(ck::IsValid(Mesh),
        TEXT("[CkIskm] AnimCollection [{}] has no DefaultMesh"), _Collection)
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

    if (ck::IsValid(_OverrideMaterial))
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
        if (ck::Is_NOT_Valid(Pair.Value))
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
        if (ck::Is_NOT_Valid(Pair.Value))
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
        if (ck::Is_NOT_Valid(Pair.Value))
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
    auto* TileComp = GetOrCreate_Tile(Tile);
    CK_ENSURE_IF_NOT(ck::IsValid(TileComp),
        TEXT("[CkIskm] AddInstance on crowd [{}]: tile creation failed (no collection/default mesh) — member dropped, index space desyncs"), this)
    { return; }

    const FCk_Iskm_BakedPose* Baked = ck::IsValid(_Collection) ? _Collection->Get_BakedPose() : nullptr;
    const int32 BakedSequenceCount = ck::IsValid(Baked, ck::IsValid_Policy_NullptrOnly{}) ? Baked->Sequences.Num() : 0;

    // Recovery clamps rather than drops: the contract is call-order == member index, so dropping a member
    // would shift every subsequent index.
    int32 SequenceIndex = InSequenceIndex;
    CK_ENSURE_IF_NOT(ck::IsValid(Baked, ck::IsValid_Policy_NullptrOnly{}) && Baked->Sequences.IsValidIndex(InSequenceIndex),
        TEXT("[CkIskm] AddInstance on crowd [{}]: sequence index [{}] is outside the baked range [0..{}) — clamping to sequence 0 to preserve the member index space"),
        this, InSequenceIndex, BakedSequenceCount)
    { SequenceIndex = 0; }

    // Component-relative transform: the tile component sits at the tile centre with identity rotation/scale.
    const FTransform TileXf(TileCentre(Tile));

    FMember Member;
    Member.WorldXf = InWorldTransform;
    Member.Tile = Tile;
    Member.Visible = true;
    Member.Inst.Transform = InWorldTransform.GetRelativeTransform(TileXf);
    Member.Inst.PrevPushedTransform = Member.Inst.Transform;
    Member.Inst.SequenceIndex = SequenceIndex;
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
    if (ck::Is_NOT_Valid(Comp))
    { return; }

    // Members carry LIVE animation state (the manager is the single source of truth), so a rebuild never
    // snaps animation back to the spawn pose.
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
    if (ck::Is_NOT_Valid(Comp))
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

    // Visibility/migration already recreate the proxy at write time, so a mismatch here is a bookkeeping bug.
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

#if WITH_EDITOR
    // FProcessor_IskmCrowd_Advance is deliberately absent from non-PIE editor graphs. Members start at
    // baked frame zero (the reference pose), so resolve their authored sequence/time offset once after every
    // member has been added and the tiles exist. This is a one-shot render-data push, not an editor tick.
    if (ck_iskm_batched_crowd::IsNonPieEditorWorld(GetWorld()))
    { AdvanceAnimation(0.0f); }
#endif
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    AdvanceAnimation(float InDeltaTime)
    -> void
{
    // Rendering is client-local; a dedicated server has nothing to feed.
    if (GetNetMode() == NM_DedicatedServer)
    { return; }
    if (ck::Is_NOT_Valid(_Collection) || _Members.Num() == 0)
    { return; }
    const FCk_Iskm_BakedPose* Baked = _Collection->Get_BakedPose();
    CK_ENSURE_IF_NOT(ck::IsValid(Baked, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("[CkIskm] Crowd [{}]: AnimCollection [{}] has no baked pose — crowd animation frozen"), this, _Collection)
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

    // Groups are tiny, so an unconditional per-tick push beats tracking per-group dirtiness.
    for (auto& Pair : _OutlineGroups)
    {
        Push_HighlightGroup(Pair.Value);
    }

    for (auto& Pair : _CelGroups)
    {
        Push_HighlightGroup(Pair.Value);
    }

    for (auto& Pair : _StylizeMaskGroups)
    {
        Push_HighlightGroup(Pair.Value);
    }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    DriveCosmetics()
    -> void
{
    if (_MemberCosmetics.Num() == 0)
    { return; }

    // Must run in the same processor tick as AdvanceAnimation, on the SAME _Members snapshot PushTile just
    // rendered. Request_SetTransform is the deferred cross-entity write: HandleRequests applies it later
    // THIS tick, so the PostTransform flush picks it up the frame its member is PushTiled — no trail.
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
                C.Cosmetic, FCk_Request_Transform_SetTransform{C.RelOffset * SocketWorld}, {});
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
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex) && ck::IsValid(InCosmetic),
        TEXT("[CkIskm] Register_MemberCosmetic: invalid member index [{}] or cosmetic handle [{}]"), InIndex, InCosmetic)
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
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return FTransform::Identity; }

    return _Members[InIndex].WorldXf;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberSequenceIndex(int32 InIndex) const
    -> int32
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return 0; }

    return _Members[InIndex].Inst.SequenceIndex;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberAnimationTime(int32 InIndex) const
    -> float
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return 0.0f; }

    return _Members[InIndex].Inst.Time;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberVisible(int32 InIndex) const
    -> bool
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return false; }

    return _Members[InIndex].Visible;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberTransform(int32 InIndex, const FTransform& InWorldTransform)
    -> void
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return; }

    FMember& M = _Members[InIndex];
    M.WorldXf = InWorldTransform;

    const FIntPoint NewTile = TileCoordOf(InWorldTransform.GetLocation());
    if (NewTile == M.Tile)
    {
        // In-tile: rides the light per-frame push, and PrevPushedTransform still holds last frame's value,
        // so the move produces a real motion vector.
        M.Inst.Transform = InWorldTransform.GetRelativeTransform(FTransform(TileCentre(M.Tile)));
        if (M.Visible)
        { _DirtyTiles.Add(M.Tile); }

        // Highlighted member walking out of its cluster's fixed bounds → rebuild (recomputes bounds).
        if (auto* Group = DoFind_MemberHighlightGroup(InIndex);
            Group != nullptr && NOT Group->PaddedBounds.IsInsideOrOn(InWorldTransform.GetLocation()))
        { Rebuild_HighlightGroup(*Group); }
        return;
    }

    // Tile migration: rebuild both tiles (instance counts change on each).
    const FIntPoint OldTile = M.Tile;
    if (TArray<int32>* OldList = _TileMembers.Find(OldTile))
    { OldList->RemoveSingleSwap(InIndex); }

    M.Tile = NewTile;
    M.Inst.Transform = InWorldTransform.GetRelativeTransform(FTransform(TileCentre(NewTile)));
    // Different proxy = different motion-vector space: zero velocity for one frame beats a cross-primitive smear.
    M.Inst.PrevPushedTransform = M.Inst.Transform;

    if (GetOrCreate_Tile(NewTile) != nullptr)
    { _TileMembers.FindOrAdd(NewTile).Add(InIndex); }

    RebuildTile(OldTile);
    RebuildTile(NewTile);
    _DirtyTiles.Remove(OldTile);
    _DirtyTiles.Remove(NewTile);

    if (auto* Group = DoFind_MemberHighlightGroup(InIndex);
        Group != nullptr && NOT Group->PaddedBounds.IsInsideOrOn(InWorldTransform.GetLocation()))
    { Rebuild_HighlightGroup(*Group); }
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberAnimation(int32 InIndex, int32 InSequenceIndex, float InRate, bool InResetTime)
    -> void
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return; }

    const FCk_Iskm_BakedPose* Baked = ck::IsValid(_Collection) ? _Collection->Get_BakedPose() : nullptr;
    const int32 BakedSequenceCount = ck::IsValid(Baked, ck::IsValid_Policy_NullptrOnly{}) ? Baked->Sequences.Num() : 0;
    CK_ENSURE_IF_NOT(ck::IsValid(Baked, ck::IsValid_Policy_NullptrOnly{}) && Baked->Sequences.IsValidIndex(InSequenceIndex),
        TEXT("[CkIskm] Set_MemberAnimation: sequence index [{}] is outside the baked range [0..{})"),
        InSequenceIndex, BakedSequenceCount)
    { return; }

    FMember& M = _Members[InIndex];
    M.Inst.SequenceIndex = InSequenceIndex;
    M.Inst.Rate = InRate;
    if (InResetTime)
    { M.Inst.Time = 0.0f; }
    if (M.Visible)
    { _DirtyTiles.Add(M.Tile); }

#if WITH_EDITOR
    // The editor graph intentionally has no crowd-advance processor. Flush this explicit authored change
    // immediately so changing a preview member's sequence never leaves its renderer at a stale pose.
    if (ck_iskm_batched_crowd::IsNonPieEditorWorld(GetWorld()))
    { AdvanceAnimation(0.0f); }
#endif
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberCustomData(int32 InIndex, float InA, float InB)
    -> void
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
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
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
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
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
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
    if (_Members.IsValidIndex(InIndex) == false || ck::Is_NOT_Valid(_Collection))
    { return false; }
    const FCk_Iskm_BakedPose* Baked = _Collection->Get_BakedPose();
    if (ck::Is_NOT_Valid(Baked, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }
    const FCk_Iskm_BakedSocket* Socket = Baked->Find_Socket(InSocket);
    if (ck::Is_NOT_Valid(Socket, ck::IsValid_Policy_NullptrOnly{}) || Socket->FrameTransforms.Num() == 0)
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
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return; }
    if (_Members[InIndex].Visible == InVisible)
    { return; }
    _Members[InIndex].Visible = InVisible;
    RebuildTile(_Members[InIndex].Tile);
    _DirtyTiles.Remove(_Members[InIndex].Tile);

    // Hidden members leave their highlight cluster (their Plan-1 stand-in is styled via the entity API).
    if (auto* Group = DoFind_MemberHighlightGroup(InIndex))
    { Rebuild_HighlightGroup(*Group); }
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
// Custom-depth highlights (member-indexed) — see CkUsf/Claude.md § Entity outlines / § Cel shade.

UCk_Iskm_BatchedClusterComponent*
    ACk_Iskm_BatchedCrowd_Actor::
    DoCreate_HighlightCluster(uint8 InStencilValue, const TCHAR* InNameBase)
{
    USkeletalMesh* Mesh = ck::IsValid(_Collection) ? _Collection->Get_DefaultMesh() : nullptr;

    CK_ENSURE_IF_NOT(ck::IsValid(Mesh),
        TEXT("[CkIskm] DoCreate_HighlightCluster: crowd [{}] has no collection/mesh"), this)
    { return nullptr; }

    auto* Comp = NewObject<UCk_Iskm_BatchedClusterComponent>(this,
        MakeUniqueObjectName(this, UCk_Iskm_BatchedClusterComponent::StaticClass(), InNameBase));
    Comp->SetupAttachment(_Root);
    // Instances AND fixed bounds are authored in WORLD space, so this component must stay at identity
    // regardless of the crowd actor's transform — absolute flags, not just SetWorldLocation. Inheriting
    // an actor rotation would rotate the bounds box away from the instances. See CkIskmRenderer/CLAUDE.md.
    Comp->SetUsingAbsoluteLocation(true);
    Comp->SetUsingAbsoluteRotation(true);
    Comp->SetUsingAbsoluteScale(true);
    Comp->RegisterComponent();
    Comp->SetWorldTransform(FTransform::Identity);
    Comp->Setup(_Collection, Mesh);
    Comp->Set_ManagedExternally(true);
    Comp->CastShadow = false;
    // Custom-depth-only: the silhouette source the post-process reads.
    Comp->bRenderInMainPass = false;
    Comp->SetRenderCustomDepth(true);
    Comp->SetCustomDepthStencilValue(static_cast<int32>(InStencilValue));

    return Comp;
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    DoFind_MemberHighlightGroup(int32 InIndex)
    -> FHighlightGroup*
{
    if (const auto* OutlinePreset = _MemberOutlines.Find(InIndex))
    { return _OutlineGroups.Find(*OutlinePreset); }

    if (const auto* CelPattern = _MemberCelPatterns.Find(InIndex))
    { return _CelGroups.Find(CelPattern->Stencil); }

    // Found by membership rather than by key: a masked member records no stencil of its own.
    if (_StylizeMaskedMembers.Contains(InIndex))
    {
        for (auto& Pair : _StylizeMaskGroups)
        {
            if (Pair.Value.Members.Contains(InIndex))
            { return &Pair.Value; }
        }
    }

    return nullptr;
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberOutline(int32 InIndex, UCkUsf_OutlinePreset* InPreset)
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
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
        // First member with this preset: one stencil refcount per group + a highlight cluster at the origin.
        auto* World = GetWorld();
        auto* OutlineSubsystem = ck::IsValid(World)
            ? World->GetSubsystem<UCkUsf_OutlineSubsystem>()
            : nullptr;

        CK_ENSURE_IF_NOT(ck::IsValid(OutlineSubsystem),
            TEXT("[CkIskm] Set_MemberOutline: no CkUsf outline subsystem available"))
        { return; }

        const auto Stencil = OutlineSubsystem->Get_OrAllocate_StencilFor(InPreset);
        CK_ENSURE_IF_NOT(Stencil != 0,
            TEXT("[CkIskm] Set_MemberOutline: stencil range exhausted for preset [{}] — outline dropped"), InPreset)
        { return; }

        auto* Comp = DoCreate_HighlightCluster(Stencil, TEXT("IskmOutlineCluster"));
        if (ck::Is_NOT_Valid(Comp))
        {
            OutlineSubsystem->Release_StencilFor(InPreset);
            return;
        }

        auto NewGroup = FHighlightGroup{};
        NewGroup.Comp = Comp;
        NewGroup.Stencil = Stencil;
        Group = &_OutlineGroups.Add(InPreset, MoveTemp(NewGroup));
    }

    // The outline WINS: both write the member's Custom-Stencil byte, and leaving the cel cluster alive
    // would put two custom-depth writers on the same pixels with the winner undefined. Silent because the
    // caller asked for the outline and gets it — the reverse order is the one that is refused loudly. Last,
    // after every way this can fail: an outline that never lands must not take the pattern with it.
    Clear_MemberCelPattern(InIndex);
    // Same rule one precedence level further down — the mask is the lowest of the three claims.
    Clear_MemberStylizeMask(InIndex);

    Group->Members.AddUnique(InIndex);
    _MemberOutlines.Add(InIndex, InPreset);
    Rebuild_HighlightGroup(*Group);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Clear_MemberOutline(int32 InIndex)
{
    const auto* PresetPtr = _MemberOutlines.Find(InIndex);
    if (PresetPtr == nullptr)
    { return; }

    // Look the group up by the WEAK key itself, never a raw round-trip: a dead preset's Get() is null and a
    // null-constructed weak never matches the stale key, leaking the group, its component and its stencil.
    const TWeakObjectPtr<UCkUsf_OutlinePreset> PresetKey = *PresetPtr;
    auto* Preset = PresetKey.Get();
    _MemberOutlines.Remove(InIndex);

    auto* Group = _OutlineGroups.Find(PresetKey);
    if (Group == nullptr)
    { return; }

    Group->Members.RemoveSingleSwap(InIndex);

    if (Group->Members.Num() > 0)
    {
        Rebuild_HighlightGroup(*Group);
        return;
    }

    if (auto* Comp = Group->Comp.Get())
    { Comp->DestroyComponent(); }

    if (auto* World = GetWorld();
        ck::IsValid(World))
    {
        if (auto* OutlineSubsystem = World->GetSubsystem<UCkUsf_OutlineSubsystem>())
        {
            // A dead preset cannot be released against the subsystem (its allocation is keyed by the
            // live preset); the refcount gap is accepted for the editor-only force-delete case.
            if (ck::IsValid(Preset))
            { OutlineSubsystem->Release_StencilFor(Preset); }
        }
    }

    _OutlineGroups.Remove(PresetKey);
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

// --------------------------------------------------------------------------------------------------------------------
// Cel pattern (member-indexed) — see CkUsf/Claude.md § Cel shade (Stylize).

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberCelPattern(int32 InIndex, ECk_Usf_CelPattern InPattern)
{
    CK_ENSURE_IF_NOT(_Members.IsValidIndex(InIndex),
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return; }

    // Both features write the member's Custom-Stencil byte, so accepting this would silently replace the
    // outline the caller asked for earlier. The entity-level Request_SetCelPattern refuses the same way.
    const auto StencilIsFree = NOT _MemberOutlines.Contains(InIndex);
    CK_ENSURE_IF_NOT(StencilIsFree,
        TEXT("[CkIskm] Set_MemberCelPattern on crowd [{}] member [{}]: the member already carries an "
             "OUTLINE, which owns its Custom Stencil value; cel pattern NOT applied"), this, InIndex)
    { return; }

    auto* World = GetWorld();
    auto* CelShadeSubsystem = ck::IsValid(World)
        ? World->GetSubsystem<UCkUsf_CelShadeSubsystem>()
        : nullptr;

    CK_ENSURE_IF_NOT(ck::IsValid(CelShadeSubsystem),
        TEXT("[CkIskm] Set_MemberCelPattern: no CkUsf cel-shade subsystem available"))
    { return; }

    // 0 is the engine's "nothing written here", which the contract returns when stencil patterns are
    // disabled in this world's settings — writing it would mark the cluster and read as a silent success.
    const auto StencilValue = CelShadeSubsystem->Get_StencilValueFor(InPattern);
    const auto StencilIsAddressable = StencilValue > 0 && StencilValue <= MAX_uint8;
    CK_ENSURE_IF_NOT(StencilIsAddressable,
        TEXT("[CkIskm] Set_MemberCelPattern: the cel stencil contract resolves pattern [{}] to [{}] — "
             "cel pattern dropped"), InPattern, StencilValue)
    { return; }

    const auto Stencil = static_cast<uint8>(StencilValue);

    if (const auto* Existing = _MemberCelPatterns.Find(InIndex);
        Existing != nullptr)
    {
        if (Existing->Pattern == InPattern && Existing->Stencil == Stencil)
        { return; }
        Clear_MemberCelPattern(InIndex); // pattern change: leave the old group first
    }

    auto* Group = _CelGroups.Find(Stencil);
    if (Group == nullptr)
    {
        auto* Comp = DoCreate_HighlightCluster(Stencil, TEXT("IskmCelPatternCluster"));
        if (ck::Is_NOT_Valid(Comp))
        { return; }

        auto NewGroup = FHighlightGroup{};
        NewGroup.Comp = Comp;
        NewGroup.Stencil = Stencil;
        Group = &_CelGroups.Add(Stencil, MoveTemp(NewGroup));
    }

    // The pattern outranks the mask and they share the member's Custom-Stencil byte, so the mask cluster
    // must go — silently, exactly as the outline takes the pattern over. Last, after every way this can
    // fail: a pattern that never lands must not take the mask with it.
    Clear_MemberStylizeMask(InIndex);

    Group->Members.AddUnique(InIndex);
    _MemberCelPatterns.Add(InIndex, FMemberCelPattern{InPattern, Stencil});
    Rebuild_HighlightGroup(*Group);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Clear_MemberCelPattern(int32 InIndex)
{
    const auto* Existing = _MemberCelPatterns.Find(InIndex);
    if (Existing == nullptr)
    { return; }

    const auto Stencil = Existing->Stencil;
    _MemberCelPatterns.Remove(InIndex);

    auto* Group = _CelGroups.Find(Stencil);
    if (Group == nullptr)
    { return; }

    Group->Members.RemoveSingleSwap(InIndex);

    if (Group->Members.Num() > 0)
    {
        Rebuild_HighlightGroup(*Group);
        return;
    }

    // No release half: the cel contract is a direct stencil value, not a refcounted allocation.
    if (auto* Comp = Group->Comp.Get())
    { Comp->DestroyComponent(); }

    _CelGroups.Remove(Stencil);
}

ECk_Usf_CelPattern
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberCelPatternOr(int32 InIndex, ECk_Usf_CelPattern InFallback) const
{
    const auto* Existing = _MemberCelPatterns.Find(InIndex);
    return Existing != nullptr ? Existing->Pattern : InFallback;
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_MemberCelPatternStencilValue(int32 InIndex) const
{
    const auto* Existing = _MemberCelPatterns.Find(InIndex);
    return Existing != nullptr ? static_cast<int32>(Existing->Stencil) : 0;
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_CelPatternedMemberCount() const
{
    return _MemberCelPatterns.Num();
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_CelPatternRenderedInstanceCount() const
{
    int32 Total = 0;
    for (const auto& Pair : _CelGroups)
    {
        if (const auto* Comp = Pair.Value.Comp.Get())
        { Total += Comp->Get_Instances().Num(); }
    }
    return Total;
}

// --------------------------------------------------------------------------------------------------------------------
// Stylize effect mask (member-indexed) — see CkUsf/Claude.md § Stylize.

void
    ACk_Iskm_BatchedCrowd_Actor::
    Set_MemberStylizeMask(int32 InIndex)
{
    // Hoisted out of the ensure body: CK_DISABLE_ENSURE_CHECKS compiles the macro and its body away
    // entirely, and an out-of-range index reaching the map writes below is memory corruption, not a
    // diagnostic. The bounds check has to survive the diagnostic being switched off.
    const auto IndexIsValid = _Members.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("[CkIskm] Member index [{}] out of range [0..{})"), InIndex, _Members.Num())
    { return; }

    // The mask is the LAST of the three claims on the member's Custom-Stencil byte, so accepting this would
    // silently replace the outline or cel pattern the caller asked for earlier. The entity-level
    // Request_AddToStylizeMask refuses the same way.
    const auto StencilIsFree = NOT _MemberOutlines.Contains(InIndex) && NOT _MemberCelPatterns.Contains(InIndex);
    CK_ENSURE_IF_NOT(StencilIsFree,
        TEXT("[CkIskm] Set_MemberStylizeMask on crowd [{}] member [{}]: the member already carries an "
             "OUTLINE or a CEL PATTERN, which owns its Custom Stencil value; stylize mask NOT applied"), this, InIndex)
    { return; }

    // Project config rather than a per-world subsystem: there is exactly ONE mask value per project. 0 is
    // the engine's "nothing written here", so a cluster carrying it would mark its instances with the value
    // that means untagged and read as a silent success; the setting clamps at 1, so only a hand-edited .ini
    // reaches here. This is a one-shot imperative call rather than a per-frame processor, so an unusable
    // value is refused LOUDLY at the call site instead of being skipped the way the sync processors skip it.
    const auto StencilValue = UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue();
    const auto StencilIsAddressable = StencilValue > 0 && StencilValue <= MAX_uint8;
    CK_ENSURE_IF_NOT(StencilIsAddressable,
        TEXT("[CkIskm] Set_MemberStylizeMask: the project's stylize mask stencil value is [{}], which is "
             "outside the addressable range [1..{}]; stylize mask dropped"), StencilValue, MAX_uint8)
    { return; }

    const auto Stencil = static_cast<uint8>(StencilValue);

    if (_StylizeMaskedMembers.Contains(InIndex))
    {
        if (const auto* Existing = DoFind_MemberHighlightGroup(InIndex);
            Existing != nullptr && Existing->Stencil == Stencil)
        { return; }

        Clear_MemberStylizeMask(InIndex); // the project value moved: leave the old group first
    }

    auto* Group = _StylizeMaskGroups.Find(Stencil);
    if (Group == nullptr)
    {
        auto* Comp = DoCreate_HighlightCluster(Stencil, TEXT("IskmStylizeMaskCluster"));
        if (ck::Is_NOT_Valid(Comp))
        { return; }

        auto NewGroup = FHighlightGroup{};
        NewGroup.Comp = Comp;
        NewGroup.Stencil = Stencil;
        Group = &_StylizeMaskGroups.Add(Stencil, MoveTemp(NewGroup));
    }

    Group->Members.AddUnique(InIndex);
    _StylizeMaskedMembers.Add(InIndex);
    Rebuild_HighlightGroup(*Group);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Clear_MemberStylizeMask(int32 InIndex)
{
    if (NOT _StylizeMaskedMembers.Contains(InIndex))
    { return; }

    _StylizeMaskedMembers.Remove(InIndex);

    // Located by membership, not by re-resolving the project value: a setting change between Set and Clear
    // would otherwise strand the cluster the member is actually in, its component and its instances.
    for (auto It = _StylizeMaskGroups.CreateIterator(); It; ++It)
    {
        auto& Group = It.Value();

        if (Group.Members.RemoveSingleSwap(InIndex) == 0)
        { continue; }

        if (Group.Members.Num() > 0)
        {
            Rebuild_HighlightGroup(Group);
            return;
        }

        // No release half: the mask is a direct stencil value, not a refcounted allocation.
        if (auto* Comp = Group.Comp.Get())
        { Comp->DestroyComponent(); }

        It.RemoveCurrent();
        return;
    }
}

bool
    ACk_Iskm_BatchedCrowd_Actor::
    Get_IsMemberStylizeMasked(int32 InIndex) const
{
    return _StylizeMaskedMembers.Contains(InIndex);
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_StylizeMaskedMemberCount() const
{
    return _StylizeMaskedMembers.Num();
}

int32
    ACk_Iskm_BatchedCrowd_Actor::
    Get_StylizeMaskRenderedInstanceCount() const
{
    int32 Total = 0;
    for (const auto& Pair : _StylizeMaskGroups)
    {
        if (const auto* Comp = Pair.Value.Comp.Get())
        { Total += Comp->Get_Instances().Num(); }
    }
    return Total;
}

// --------------------------------------------------------------------------------------------------------------------

void
    ACk_Iskm_BatchedCrowd_Actor::
    Rebuild_HighlightGroup(FHighlightGroup& InGroup)
{
    auto* Comp = InGroup.Comp.Get();
    if (ck::Is_NOT_Valid(Comp))
    { return; }

    // Component sits at the world origin with identity rotation/scale → instance transforms are the
    // members' world transforms directly. Custom depth has no velocity output, so Prev == Current.
    TArray<UCk_Iskm_BatchedClusterComponent::FInstance> Visible;
    auto PositionBounds = FBox{ForceInit};
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
        PositionBounds += M.WorldXf.GetLocation();
    }

    // Fixed conservative bounds: member-position union padded by the animated pose box (a member sticks
    // out at most that much) + half a tile of walk-slack so ordinary movement doesn't churn rebuilds.
    if (PositionBounds.IsValid)
    {
        FBox MeshBox = FBox(FVector(-50.0), FVector(50.0));
        if (ck::IsValid(_Collection))
        { MeshBox = _Collection->Get_AnimatedMeshBounds(); }
        else
        { CK_TRIGGER_ENSURE(TEXT("[CkIskm] Rebuild_HighlightGroup on crowd [{}] with no AnimCollection — using a placeholder mesh box"), this); }
        const FVector MeshSize = MeshBox.GetSize();
        const float Pad = FMath::Max(MeshSize.X, MeshSize.Y) + _TileSize * 0.5f;

        InGroup.PaddedBounds = FBox(
            PositionBounds.Min + FVector(-Pad, -Pad, MeshBox.Min.Z - MeshSize.Z),
            PositionBounds.Max + FVector(+Pad, +Pad, MeshBox.Max.Z + MeshSize.Z));
        Comp->Set_FixedLocalBounds(InGroup.PaddedBounds); // component at origin ⇒ local == world
    }

    Comp->Set_Instances(Visible);
}

void
    ACk_Iskm_BatchedCrowd_Actor::
    Push_HighlightGroup(FHighlightGroup& InGroup)
{
    auto* Comp = InGroup.Comp.Get();
    if (ck::Is_NOT_Valid(Comp))
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

    // Count changes always flow through Rebuild_HighlightGroup at write time — a mismatch is a bookkeeping bug.
    if (Visible.Num() == Comp->Get_Instances().Num())
    { Comp->Push_LiveInstances(MoveTemp(Visible)); }
    else
    {
        CK_TRIGGER_ENSURE(TEXT("[CkIskm] Push_HighlightGroup count mismatch ({} members visible vs {} instances) — highlight bookkeeping bug, recovering via Set_Instances"),
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
        ck::IsValid(World))
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

    // Cel and mask groups have no release half — a direct stencil value is not a refcounted allocation.
    _CelGroups.Reset();
    _MemberCelPatterns.Reset();

    _StylizeMaskGroups.Reset();
    _StylizeMaskedMembers.Reset();

    DoDestroy_ControllerEntity();

    Super::EndPlay(InEndPlayReason);
}

auto
    ACk_Iskm_BatchedCrowd_Actor::
    Destroyed()
    -> void
{
    // Editor-world actors never BeginPlay, so EndPlay never fires and a destroyed preview crowd would leak a
    // controller entity FProcessor_IskmCrowd_Advance iterates forever. Idempotent with the EndPlay call.
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
