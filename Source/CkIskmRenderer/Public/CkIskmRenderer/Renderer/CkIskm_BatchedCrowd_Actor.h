#pragma once

#include "GameFramework/Actor.h"

#include "CkIskm_BatchedClusterComponent.h" // for UCk_Iskm_BatchedClusterComponent::FInstance

#include "CkIskm_BatchedCrowd_Actor.generated.h"

class UCk_IskmAnimCollection_Data;
class USceneComponent;

// ====================================================================================================================
//  CkIskmRenderer Plan-2 Phase 4b — spatial tile manager for a batched GPU-skinned crowd.
//
//  Owns a crowd, spatially partitioned into tiles. Each occupied tile gets its own
//  UCk_Iskm_BatchedClusterComponent (one FPrimitiveSceneProxy) placed at the tile centre with tight bounds, so
//  frustum + per-instance occlusion culling operate per-tile instead of over one giant aggregate bound that spans
//  the whole crowd (which never occludes cleanly and wastes GPU on off-screen instances).
//
//  Usage: Initialize() -> AddInstance() x N -> Finalize(). The tile components self-tick their instances'
//  animation, so a static-transform crowd needs no per-frame manager flush.
// ====================================================================================================================
UCLASS(NotBlueprintable)
class CKISKMRENDERER_API ACk_Iskm_BatchedCrowd_Actor : public AActor
{
    GENERATED_BODY()

public:
    ACk_Iskm_BatchedCrowd_Actor();

    void Initialize(UCk_IskmAnimCollection_Data* InCollection, float InTileSize);

    // Buffer one instance at a world transform; it is assigned to its tile (created on demand). Flushed by Finalize().
    void AddInstance(const FTransform& InWorldTransform, int32 InSequenceIndex, float InRate, float InTimeOffset);

    // Flush buffered instances into each tile's cluster component via Set_Instances. Call once after all AddInstance().
    void Finalize();

    int32 Get_TileCount() const { return _Tiles.Num(); }
    int32 Get_InstanceCount() const { return _TotalInstances; }

private:
    FIntPoint TileCoordOf(const FVector& InWorldLocation) const;
    FVector   TileCentre(const FIntPoint& InTile) const;
    UCk_Iskm_BatchedClusterComponent* GetOrCreate_Tile(const FIntPoint& InTile);

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> _Root;

    UPROPERTY(Transient)
    TObjectPtr<UCk_IskmAnimCollection_Data> _Collection;

    float _TileSize = 2000.0f;

    UPROPERTY(Transient)
    TMap<FIntPoint, TObjectPtr<UCk_Iskm_BatchedClusterComponent>> _Tiles;

    // Buffered per-tile instances (component-relative) until Finalize.
    TMap<FIntPoint, TArray<UCk_Iskm_BatchedClusterComponent::FInstance>> _PendingInstances;

    int32 _TotalInstances = 0;
};
