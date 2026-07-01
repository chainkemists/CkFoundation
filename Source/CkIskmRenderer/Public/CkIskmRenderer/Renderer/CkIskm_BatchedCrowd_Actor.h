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
//  UCk_Iskm_BatchedClusterComponent (one FPrimitiveSceneProxy) placed at the tile centre with tight bounds, so
//  frustum + per-instance occlusion culling operate per-tile instead of over one giant aggregate bound.
//
//  Phase 5 (distance-LOD flip): each member can be hidden from its batched tile (Set_MemberVisible) so a per-SKMC
//  proxy (Plan-1) can stand in for it (ragdoll/montage), then shown again to return to batched. Members hold both
//  their world transform (for distance queries) and the tile-relative FInstance (for rebuilds).
//
//  Usage: Initialize() -> AddInstance() x N -> Finalize(). Tile components self-tick their instances' animation,
//  so a static-transform crowd needs no per-frame manager flush.
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

    // ---- LOD-facing (Phase 5) ----
    int32      Get_MemberCount() const { return _Members.Num(); }
    FTransform Get_MemberWorldTransform(int32 InIndex) const;
    int32      Get_MemberSequenceIndex(int32 InIndex) const;
    bool       Get_MemberVisible(int32 InIndex) const;
    // Hide/show a member in its batched tile (rebuilds that one tile). Hidden members leave a gap for a per-SKMC stand-in.
    void       Set_MemberVisible(int32 InIndex, bool bInVisible);

    // Sum of each tile component's CURRENT instance count (i.e. only visible members) — drops when a member is hidden.
    int32      Get_RenderedInstanceCount() const;

private:
    struct FMember
    {
        FTransform WorldXf = FTransform::Identity;
        FIntPoint  Tile = FIntPoint(0, 0);
        UCk_Iskm_BatchedClusterComponent::FInstance Inst;
        bool       bVisible = true;
    };

    FIntPoint TileCoordOf(const FVector& InWorldLocation) const;
    FVector   TileCentre(const FIntPoint& InTile) const;
    UCk_Iskm_BatchedClusterComponent* GetOrCreate_Tile(const FIntPoint& InTile);
    void      RebuildTile(const FIntPoint& InTile);

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> _Root;

    UPROPERTY(Transient)
    TObjectPtr<UCk_IskmAnimCollection_Data> _Collection;

    float _TileSize = 2000.0f;

    UPROPERTY(Transient)
    TMap<FIntPoint, TObjectPtr<UCk_Iskm_BatchedClusterComponent>> _Tiles;

    TArray<FMember> _Members;
};
