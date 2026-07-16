#pragma once

#include "GameFramework/Actor.h"

#include "CkIskm_BatchedClusterComponent.h" // for UCk_Iskm_BatchedClusterComponent::FInstance

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h" // FCk_Handle_Transform / FCk_Handle (member cosmetics + controller entity)

#include "CkIskm_BatchedCrowd_Actor.generated.h"

class UCk_IskmAnimCollection_Data;
class USceneComponent;
class UCkUsf_OutlinePreset;

// --------------------------------------------------------------------------------------------------------------------
// CkIskmRenderer Plan-2 — spatial tile manager for a batched GPU-skinned crowd.
//
// Owns a crowd, spatially partitioned into tiles. Each occupied tile gets its own
// UCk_Iskm_BatchedClusterComponent (one FPrimitiveSceneProxy) placed at the tile centre with FIXED conservative
// bounds (tile extent + mesh pad), so frustum + per-instance occlusion culling operate per-tile and member
// movement never recomputes bounds.
//
// SINGLE SOURCE OF TRUTH: the manager owns all member state (transform, animation time/frames, visibility,
// custom data) and ticks it itself — tile components are render-facing views with self-tick disabled. Tile
// rebuilds therefore always carry live animation time (no snap-back), and members can MOVE:
// Set_MemberTransform updates in-tile transforms via the light per-frame push path and migrates members
// across tile borders (two Set_Instances rebuilds).
//
// Distance-LOD flip: Set_MemberVisible hides a member from its batched tile so a per-SKMC proxy
// (Plan-1) can stand in for it (ragdoll/montage), then shows it again to return to batched. Hidden members
// keep advancing time so they rejoin in phase.
//
// Usage: Initialize() -> AddInstance() x N -> Finalize(). Then drive members via Set_Member* as the game
// (e.g. NPC/crowd systems) moves them.
UCLASS(NotBlueprintable)
class CKISKMRENDERER_API ACk_Iskm_BatchedCrowd_Actor : public AActor
{
    GENERATED_BODY()

public:
    ACk_Iskm_BatchedCrowd_Actor();

    void Initialize(UCk_IskmAnimCollection_Data* InCollection, float InTileSize);

    // Buffer one instance at a world transform; assigned to its tile (created on demand). Flushed by Finalize().
    void AddInstance(const FTransform& InWorldTransform, int32 InSequenceIndex, float InRate, float InTimeOffset);

    // Flush all members into their tile clusters. Call once after all AddInstance().
    void Finalize();

    int32 Get_TileCount() const { return _Tiles.Num(); }
    int32 Get_InstanceCount() const { return _Members.Num(); }

    // ---- Member API (game-facing: NPC/crowd systems drive these) ----
    int32      Get_MemberCount() const { return _Members.Num(); }
    FTransform Get_MemberWorldTransform(int32 InIndex) const;
    int32      Get_MemberSequenceIndex(int32 InIndex) const;
    bool       Get_MemberVisible(int32 InIndex) const;

    // Move a member. In-tile moves ride the light per-frame push (no proxy recreate); crossing a tile border
    // migrates the member (rebuilds both tiles). Velocity history is handled (no TAA smear on migration).
    void       Set_MemberTransform(int32 InIndex, const FTransform& InWorldTransform);

    // Switch a member's sequence/rate (e.g. idle -> walk when its NPC starts moving).
    void       Set_MemberAnimation(int32 InIndex, int32 InSequenceIndex, float InRate, bool InResetTime);

    // Per-member material custom data, legacy 2-float form — writes UserData[0]/[1] (shader floats [2]/[3]).
    void       Set_MemberCustomData(int32 InIndex, float InA, float InB);

    // Absolute-indexed writes into the per-instance custom-data floats. InFirstFloat is in the SAME index
    // space as the material PerInstanceCustomData node's DataIndex (game data: 2..15; [0]/[1] are the
    // animation frame bits and are rejected — writing them would corrupt pose lookup).
    void       Set_MemberCustomData(int32 InIndex, int32 InFirstFloat, const TArray<float>& InValues);
    float      Get_MemberCustomData(int32 InIndex, int32 InFloatIndex) const;

    // World-space transform of a baked socket for one member at the member's CURRENT animation
    // frame (far cosmetics — the crowd owns the clock). False when the collection bakes no
    // such socket or the index is invalid; callers leave their cosmetics hidden/parked.
    bool       TryGet_MemberSocketTransform(int32 InIndex, FName InSocket, FTransform& OutWorld) const;

    // Hide/show a member in its batched tile (rebuilds that one tile). Hidden members leave a gap for a per-SKMC stand-in.
    void       Set_MemberVisible(int32 InIndex, bool InVisible);

    // Default CustomPrimitiveData floats applied to EVERY tile component (existing and future). Materials whose
    // parameters ride CPD (e.g. CharacterMaster skin color at CPD 0/1/2) read zeros on batched tiles otherwise —
    // the whole crowd renders with unset (grey/black) parameters. One shared value set per crowd; per-member
    // variation needs the per-instance floats ([2]/[3] via Set_MemberCustomData) and a material that reads them.
    void       Set_DefaultTileCustomPrimitiveData(const TArray<float>& InFloats);

    // One material applied to EVERY slot of EVERY tile (existing and future) — the far-LOD whole-body look.
    // Mesh default slots are usually authored for per-SKMC customization and render unset/grey when batched;
    // one cheap override (e.g. a skin-toned MID) gives coherent distant silhouettes. Null restores mesh materials.
    void       Set_OverrideMaterial(UMaterialInterface* InMaterial);

    // Per-SLOT override materials: element index = mesh material slot. The whole-body
    // Set_OverrideMaterial above WINS over these (debug); null/absent entries fall back to mesh defaults.
    // Applied to existing and future tiles.
    void       Set_SlotOverrideMaterials(const TArray<UMaterialInterface*>& InMaterials);
    UMaterialInterface* Get_SlotOverrideMaterial(int32 InSlotIndex) const;

    // Sum of each tile component's CURRENT instance count (i.e. only visible members) — drops when a member is hidden.
    int32      Get_RenderedInstanceCount() const;

    // ---- Entity outline (member-indexed — batched members are not entities; see CkUsf/DESIGN_EntityOutlines.md) ----
    //
    // Mechanism: one custom-depth-only "highlight cluster" per (crowd, preset) at the world origin, holding
    // copies of the outlined members' instances; AdvanceAnimation pushes their live transforms/anim frames
    // so the outline tracks the GPU-skinned pose. Hidden members (Plan-1 flip stand-ins) are excluded —
    // outline the stand-in proxy via the entity API instead.

    // Outline a member with InPreset (replaces any previous preset). Null preset = Clear_MemberOutline.
    void       Set_MemberOutline(int32 InIndex, UCkUsf_OutlinePreset* InPreset);
    void       Clear_MemberOutline(int32 InIndex);
    UCkUsf_OutlinePreset* Get_MemberOutlinePreset(int32 InIndex) const;
    int32      Get_OutlinedMemberCount() const;
    // Instances currently in the highlight clusters (visible outlined members only) — test/gym surface.
    int32      Get_OutlineRenderedInstanceCount() const;

    //~ AActor
    virtual void EndPlay(const EEndPlayReason::Type InEndPlayReason) override;

    // ---- ECS-clock advance (the actor no longer self-ticks) ----

    // Advance every member's animation time/frame and push dirty tiles, then push the outline groups
    // (AdvanceAnimation replaces AActor::Tick — see the constructor's bCanEverTick = false). Driven once
    // per frame by FProcessor_IskmCrowd_Advance (FGroup_Transform_Finalize) so member render, far-cosmetic
    // placement, and the entity-outline highlight clusters all resolve on the SAME ECS clock.
    void       AdvanceAnimation(float InDeltaTime);

    // Place every registered member cosmetic at its baked socket for the member's CURRENT frame, via an
    // immediate transform write — lockstep with the member PushTile that AdvanceAnimation just did.
    void       DriveCosmetics();

    // A cosmetic ISM entity rides InIndex's baked socket while its member is far. Replace-if-same-entity
    // (idempotent re-register). InRelOffset is the cosmetic's offset from the socket transform.
    void       Register_MemberCosmetic(int32 InIndex, const FCk_Handle_Transform& InCosmetic, FName InSocket, const FTransform& InRelOffset);

    // Drop all cosmetics registered to InIndex (promote / hide / slot release). Idempotent.
    void       Clear_MemberCosmetics(int32 InIndex);

private:
    struct FMember
    {
        FTransform WorldXf = FTransform::Identity;
        FIntPoint  Tile = FIntPoint(0, 0);
        UCk_Iskm_BatchedClusterComponent::FInstance Inst;
        bool       Visible = true;
    };

    // a cosmetic ISM entity following a member's baked socket while the member is far.
    struct FMemberCosmetic
    {
        FCk_Handle_Transform Cosmetic;
        FName                Socket    = NAME_None;
        FTransform           RelOffset = FTransform::Identity;
    };

    FIntPoint TileCoordOf(const FVector& InWorldLocation) const;
    FVector   TileCentre(const FIntPoint& InTile) const;
    UCk_Iskm_BatchedClusterComponent* GetOrCreate_Tile(const FIntPoint& InTile);
    // Full rebuild (Set_Instances — proxy recreate). Use for count changes: visibility flips + tile migration.
    void      RebuildTile(const FIntPoint& InTile);
    // Light push (same instance count — transforms/frames/custom data only). Rolls members' velocity history.
    void      PushTile(const FIntPoint& InTile);
    FBox      TileLocalBounds() const;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> _Root;

    UPROPERTY(Transient)
    TObjectPtr<UCk_IskmAnimCollection_Data> _Collection;

    float _TileSize = 2000.0f;

    TArray<float> _DefaultTileCustomPrimitiveData;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> _OverrideMaterial;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> _SlotOverrideMaterials;

    UPROPERTY(Transient)
    TMap<FIntPoint, TObjectPtr<UCk_Iskm_BatchedClusterComponent>> _Tiles;

    // Member indices per tile (includes hidden — filtered when building the render arrays).
    TMap<FIntPoint, TArray<int32>> _TileMembers;

    TSet<FIntPoint> _DirtyTiles;

    TArray<FMember> _Members;

    // ---- entity-outline state ----

    // Not reflected (nested plain struct): the cluster component is kept alive by the actor's
    // OwnedComponents; presets are referenced by weak ptr (dead preset ⇒ group torn down lazily).
    struct FOutlineGroup
    {
        TWeakObjectPtr<UCk_Iskm_BatchedClusterComponent> Comp;
        TArray<int32> Members;                    // member indices (hidden ones filtered at build/push)
        uint8 Stencil = 0;                        // preset's allocated Custom-Stencil (one refcount per group)
        FBox PaddedBounds = FBox(ForceInit);      // world box the fixed bounds cover; exceed → rebuild
    };

    // Full rebuild of a group (Set_Instances + recomputed fixed bounds — proxy recreate). Membership /
    // visibility changes and bounds-exceeding moves go through here.
    void RebuildOutlineGroup(UCkUsf_OutlinePreset* InPreset);
    // Light per-frame push of live member data into the group's cluster (no bounds work).
    void PushOutlineGroup(FOutlineGroup& InGroup);

    TMap<TWeakObjectPtr<UCkUsf_OutlinePreset>, FOutlineGroup> _OutlineGroups;
    TMap<int32, TWeakObjectPtr<UCkUsf_OutlinePreset>> _MemberOutlines;

    // member index -> cosmetics riding its socket while far. Non-UPROPERTY (ECS handles are
    // value handles into the registry, not GC-tracked — matches _Members).
    TMap<int32, TArray<FMemberCosmetic>> _MemberCosmetics;

    // This crowd's ECS presence — FProcessor_IskmCrowd_Advance iterates it to drive AdvanceAnimation +
    // DriveCosmetics on the ECS clock. Created in Initialize(); cleared with the world.
    FCk_Handle _ControllerEntity;
};
