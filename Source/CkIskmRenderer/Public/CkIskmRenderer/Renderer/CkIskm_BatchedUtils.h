#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkIskm_BatchedUtils.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_Iskm_BatchedClusterComponent;
class ACk_Iskm_BatchedCrowd_Actor;
class UMaterialInterface;

// ====================================================================================================================
//  CkIskmRenderer Plan-2 — script-facing surface for the batched skeletal renderer.
//  Debug_* helpers stand up ready-made clusters/crowds for gyms/tests. Game flip-drivers build crowds through the
//  production entries (Create_Crowd -> Add_CrowdMember xN -> Finalize_Crowd) and drive members via Set_CrowdMember*.
//  Auto-routing UCk_Utils_IskmProxy_UE::Add to batched via PoseSource remains a possible later phase.
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

    // ---- Production entry (Phase 5 flip drivers): build a crowd from game code ----

    // Spawns an empty crowd manager for InCollection. Register members with Add_CrowdMember, then call
    // Finalize_Crowd exactly once. Returns null on invalid input or when no world can be resolved.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][IskmBatched] Create Crowd")
    static ACk_Iskm_BatchedCrowd_Actor*
    Create_Crowd(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        float InTileSize = 2000.0f);

    // Buffer one member at a world transform; returns the new member's index (INDEX_NONE on invalid input).
    // Only valid between Create_Crowd and Finalize_Crowd — members cannot be added to a finalized crowd.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Add Crowd Member")
    static int32
    Add_CrowdMember(
        ACk_Iskm_BatchedCrowd_Actor* InCrowd,
        const FTransform& InWorldTransform,
        int32 InSequenceIndex = 0,
        float InRate = 1.0f,
        float InTimeOffset = 0.0f);

    // Flush all buffered members into their tile clusters. Call once after the Add_CrowdMember calls.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Finalize Crowd")
    static void
    Finalize_Crowd(
        ACk_Iskm_BatchedCrowd_Actor* InCrowd);

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
    Set_CrowdMemberVisible(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, bool InVisible);

    // Total instances actually in the tile proxies right now (only visible members) — drops when a member is hidden.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Rendered Instance Count")
    static int32
    Get_CrowdRenderedInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);

    // ---- Movement + animation (game-facing: drive these from NPC/crowd systems) ----

    // Move a member. In-tile moves ride the light per-frame push; crossing a tile border migrates the member.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Member Transform")
    static void
    Set_CrowdMemberTransform(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, const FTransform& InWorldTransform);

    // Switch a member's sequence/rate (e.g. idle -> walk when its NPC starts moving).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Member Animation")
    static void
    Set_CrowdMemberAnimation(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, int32 InSequenceIndex, float InRate, bool InResetTime = false);

    // Per-member material custom data — shader instance custom-data floats [2] and [3] (tint/variety).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Member Custom Data")
    static void
    Set_CrowdMemberCustomData(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, float InA, float InB);

    // Default CustomPrimitiveData floats for every tile component (existing and future). CPD-parameterized
    // materials (e.g. CharacterMaster skin color at CPD 0/1/2) read zeros on batched tiles otherwise — the
    // whole crowd renders with unset (grey) parameters. One shared value per crowd.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Default Custom Primitive Data")
    static void
    Set_CrowdDefaultCustomPrimitiveData(ACk_Iskm_BatchedCrowd_Actor* InCrowd, const TArray<float>& InFloats);

    // One material applied to EVERY slot of every tile (existing and future) — the far-LOD whole-body look.
    // Mesh default slots authored for per-SKMC customization render unset/grey when batched; one cheap
    // override (e.g. a skin-toned MID) gives coherent distant silhouettes. Null restores mesh materials.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Override Material")
    static void
    Set_CrowdOverrideMaterial(ACk_Iskm_BatchedCrowd_Actor* InCrowd, UMaterialInterface* InMaterial);
};
