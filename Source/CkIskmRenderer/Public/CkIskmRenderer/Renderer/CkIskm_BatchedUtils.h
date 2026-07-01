#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkIskm_BatchedUtils.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_Iskm_BatchedClusterComponent;
class ACk_Iskm_BatchedCrowd_Actor;

// ====================================================================================================================
//  CkIskmRenderer Plan-2 — debug/gym driver for the batched skeletal renderer.
//  The batched path's production entry point is the same UCk_Utils_IskmProxy_UE::Add + PoseSource routing as Plan-1
//  (wired in a later phase). This utility is a direct spawn helper for gyms/tests to stand up a cluster now.
// ====================================================================================================================
UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmBatched_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmBatched_UE);

public:
    // Spawns an actor hosting a batched cluster component with an InGridSize x InGridSize grid of instances, each
    // playing InSequenceIndex with a per-instance phase offset (independent looping animation). InRate 0 = hold the
    // sequence's first frame. Returns the component (or null on failure). CPU-safe under -nullrhi (component spawns;
    // the GPU proxy + animation only materialize when the app can render).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][IskmBatched] Debug Spawn Cluster")
    static UCk_Iskm_BatchedClusterComponent*
    Debug_SpawnCluster(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        const FTransform& InBaseTransform,
        int32 InGridSize = 1,
        float InSpacing = 150.0f,
        int32 InSequenceIndex = 0,
        float InRate = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Cluster Instance Count")
    static int32
    Get_InstanceCount(const UCk_Iskm_BatchedClusterComponent* InCluster);

    // Spawns a crowd of InNumInstances scattered (deterministic grid) over a 2*InAreaExtent square around
    // InBaseTransform, spatially partitioned into tile clusters of InTileSize so frustum + per-instance occlusion
    // culling operate per-tile (Phase 4b). Returns the crowd manager actor (or null on failure).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][IskmBatched] Debug Spawn Scattered Crowd")
    static ACk_Iskm_BatchedCrowd_Actor*
    Debug_SpawnScatteredCrowd(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        const FTransform& InBaseTransform,
        int32 InNumInstances = 100,
        float InAreaExtent = 6000.0f,
        float InTileSize = 2000.0f,
        int32 InSequenceIndex = 0,
        float InRate = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Tile Count")
    static int32
    Get_CrowdTileCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Instance Count")
    static int32
    Get_CrowdInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);

    // ---- LOD-facing (Phase 5): member queries + per-member visibility ----

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Member Count")
    static int32
    Get_CrowdMemberCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Member Transform")
    static FTransform
    Get_CrowdMemberTransform(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Member Sequence Index")
    static int32
    Get_CrowdMemberSequenceIndex(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex);

    // Hide/show a member in its batched tile. Hide a member so a per-SKMC proxy can stand in for it (ragdoll/montage);
    // show it to return to batched rendering.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Member Visible")
    static void
    Set_CrowdMemberVisible(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, bool bInVisible);

    // Total instances actually in the tile proxies right now (only visible members) — drops when a member is hidden.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Rendered Instance Count")
    static int32
    Get_CrowdRenderedInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);
};
