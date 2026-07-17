#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h" // FCk_Handle_Transform (cosmetic registration)

#include "CkIskm_BatchedUtils.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_Iskm_BatchedClusterComponent;
class ACk_Iskm_BatchedCrowd_Actor;
class UMaterialInterface;
class UCkUsf_OutlinePreset;

// --------------------------------------------------------------------------------------------------------------------
// CkIskmRenderer Plan-2 — script-facing surface for the batched skeletal renderer.
// Debug_* helpers stand up ready-made clusters/crowds for gyms/tests. Game flip-drivers build crowds through the
// production entries (Create_Crowd -> Add_CrowdMember xN -> Finalize_Crowd) and drive members via Set_CrowdMember*.
// Auto-routing UCk_Utils_IskmProxy_UE::Add to batched via PoseSource remains a possible later addition.
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
    // culling operate per-tile. Returns the crowd manager actor (or null on failure).
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

    // ---- Production entry (flip drivers): build a crowd from game code ----

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

    // Editor-world entry for preview visuals (the batched-crowd sibling of the per-owner ISKM
    // renderer): resolves the preview entity's world and editor-selection owner, and returns the
    // per-(collection, owner) preview crowd — clicking any of its instances selects the owner.
    // Returns null outside editor worlds, when the entity has no selection owner, or in
    // non-editor builds — callers treat null as "no preview". The first caller per owner gets a
    // freshly Initialize()d crowd (Get_CrowdMemberCount() == 0): AddInstance + Finalize it once;
    // later calls (preview rebuilds re-running composition) get the same finalized crowd back —
    // reuse its existing member(s) via Set_CrowdMember* instead of re-adding.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] EditorOnly Get Or Create Preview Crowd")
    static ACk_Iskm_BatchedCrowd_Actor*
    EditorOnly_GetOrCreate_PreviewCrowd(
        const FCk_Handle& InPreviewEntity,
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

    // ---- LOD-facing: member queries + per-member visibility ----

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

    // Absolute-indexed per-member custom data. InFirstFloat is the material PerInstanceCustomData
    // DataIndex (game floats: 2..15; [0]/[1] carry the animation frame bits). Game layout of [2..15]:
    // BB DESIGN_IskmCosmeticParity.md — [2..9] slot indices, [10..12] skin RGB, [13..15] spare.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Member Custom Data Floats")
    static void
    Set_CrowdMemberCustomDataFloats(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, int32 InFirstFloat, const TArray<float>& InValues);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Member Custom Data Float")
    static float
    Get_CrowdMemberCustomDataFloat(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, int32 InFloatIndex);

    // World-space transform of a baked socket for one crowd member at its CURRENT animation frame
    // (far cosmetics). False when the collection bakes no such socket — leave the cosmetic
    // hidden/parked. Socket names come from the AnimCollection's _BakedSockets (BB: head_attach).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] TryGet Crowd Member Socket Transform")
    static bool
    TryGet_CrowdMemberSocketTransform(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, FName InSocket, FTransform& OutWorld);

    // register a cosmetic entity to ride a member's baked socket while the member is far. The
    // crowd places it every frame from its own FGroup_Transform_Finalize advance (lockstep with the
    // member). Replace-if-same-entity. Call on the far transition; pair with Clear on promote/hide/release.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Register Crowd Member Cosmetic")
    static void
    Register_CrowdMemberCosmetic(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, FCk_Handle_Transform InCosmetic, FName InSocket, const FTransform& InRelOffset);

    // stop the crowd driving InIndex's cosmetics (promote hands them to the SKMC follower;
    // hide parks them; slot release recycles the member). Idempotent.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Clear Crowd Member Cosmetics")
    static void
    Clear_CrowdMemberCosmetics(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex);

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

    // Per-SLOT override materials: index = mesh material slot. The whole-crowd
    // Set_CrowdOverrideMaterial (debug) WINS over these; null/absent entries fall back to mesh
    // defaults. Applied to existing and future tiles.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Slot Override Materials")
    static void
    Set_CrowdSlotOverrideMaterials(ACk_Iskm_BatchedCrowd_Actor* InCrowd, const TArray<UMaterialInterface*>& InMaterials);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Slot Override Material")
    static UMaterialInterface*
    Get_CrowdSlotOverrideMaterial(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InSlotIndex);

    // ---- Entity outline (member-indexed — batched members are not entities; see CkUsf/DESIGN_EntityOutlines.md) ----

    // Outline a member with a CkUsf outline preset (custom-depth highlight cluster mirrors its skinned
    // pose). Null preset clears. Hidden members (Plan-1 flip stand-ins) are excluded while hidden —
    // outline the stand-in proxy via UCk_Utils_Usf_Outline_UE instead.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Set Crowd Member Outline")
    static void
    Set_CrowdMemberOutline(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex, UCkUsf_OutlinePreset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Clear Crowd Member Outline")
    static void
    Clear_CrowdMemberOutline(ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Member Outline Preset")
    static UCkUsf_OutlinePreset*
    Get_CrowdMemberOutlinePreset(const ACk_Iskm_BatchedCrowd_Actor* InCrowd, int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Outlined Member Count")
    static int32
    Get_CrowdOutlinedMemberCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);

    // Instances currently in the highlight clusters (visible outlined members only).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Crowd Outline Rendered Instance Count")
    static int32
    Get_CrowdOutlineRenderedInstanceCount(const ACk_Iskm_BatchedCrowd_Actor* InCrowd);
};
