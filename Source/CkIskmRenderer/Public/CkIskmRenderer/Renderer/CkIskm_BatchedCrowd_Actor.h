#pragma once

#include "GameFramework/Actor.h"

#include "CkIskm_BatchedClusterComponent.h" // for UCk_Iskm_BatchedClusterComponent::FInstance

#include "CkIskm_BatchedCrowd_Actor.generated.h"

class UCk_IskmAnimCollection_Data;
class USceneComponent;

// ====================================================================================================================
//  CkIskmRenderer Plan-2 Phase 4b/5 — spatial tile manager for a batched GPU-skinned crowd.
//
//  Owns a crowd, spatially partitioned into tiles. Each occupied tile gets its own
//  UCk_Iskm_BatchedClusterComponent (one FPrimitiveSceneProxy) placed at the tile centre with FIXED conservative
//  bounds (tile extent + mesh pad), so frustum + per-instance occlusion culling operate per-tile and member
//  movement never recomputes bounds.
//
//  SINGLE SOURCE OF TRUTH: the manager owns all member state (transform, animation time/frames, visibility,
//  custom data) and ticks it itself — tile components are render-facing views with self-tick disabled. Tile
//  rebuilds therefore always carry live animation time (no snap-back), and members can MOVE:
//  Set_MemberTransform updates in-tile transforms via the light per-frame push path and migrates members
//  across tile borders (two Set_Instances rebuilds).
//
//  Phase 5 (distance-LOD flip): Set_MemberVisible hides a member from its batched tile so a per-SKMC proxy
//  (Plan-1) can stand in for it (ragdoll/montage), then shows it again to return to batched. Hidden members
//  keep advancing time so they rejoin in phase.
//
//  Usage: Initialize() -> AddInstance() x N -> Finalize(). Then drive members via Set_Member* as the game
//  (e.g. NPC/crowd systems) moves them.
// ====================================================================================================================
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

    // Per-member material custom data — shader instance custom-data floats [2] and [3] (tint/variety).
    void       Set_MemberCustomData(int32 InIndex, float InA, float InB);

    // Hide/show a member in its batched tile (rebuilds that one tile). Hidden members leave a gap for a per-SKMC stand-in.
    void       Set_MemberVisible(int32 InIndex, bool InVisible);

    // Sum of each tile component's CURRENT instance count (i.e. only visible members) — drops when a member is hidden.
    int32      Get_RenderedInstanceCount() const;

    //~ AActor
    virtual void Tick(float InDeltaTime) override;

private:
    struct FMember
    {
        FTransform WorldXf = FTransform::Identity;
        FIntPoint  Tile = FIntPoint(0, 0);
        UCk_Iskm_BatchedClusterComponent::FInstance Inst;
        bool       Visible = true;
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

    UPROPERTY(Transient)
    TMap<FIntPoint, TObjectPtr<UCk_Iskm_BatchedClusterComponent>> _Tiles;

    // Member indices per tile (includes hidden — filtered when building the render arrays).
    TMap<FIntPoint, TArray<int32>> _TileMembers;

    TSet<FIntPoint> _DirtyTiles;

    TArray<FMember> _Members;
};
