#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment_Data.h"

#include "CkVoxelNavVolume_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoxelNavVolume"))
class CKVOXELNAV_API UCk_Utils_VoxelNavVolume_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoxelNavVolume_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoxelNavVolume);

public:
    /** Add the volumetric-navigation feature to InOwner. Creates a child entity carrying the bake params;
     *  FProcessor_VoxelNavVolume_Setup consumes its NeedsSetup tag on the next tick and, unless the params
     *  opted out, arms the first build from there.
     *
     *  A volume whose bounds exceed the max chunk size PARTITIONS here, synchronously: the returned handle
     *  owns one chunk child per sub-box, each an ordinary volume entity baking its own octree, and it bakes
     *  none itself. Partitioning is decided at composition rather than in a processor because creating
     *  entities is a structural change, and a processor doing that mid-view is how a view invalidates
     *  underneath itself. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Add Volume Feature")
    static FCk_Handle_VoxelNavVolume
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_VoxelNavVolume_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Has Volume Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /** Bakes the volume, resuming across as many ticks as the probe budget needs. A request arriving while
     *  a build is already underway is an idempotent no-op unless it forces a restart.
     *
     *  The completion delegate fires when the BUILD ends, not when the request is accepted — that is
     *  frames later, and it is the only outcome a caller can act on. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Request Build",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoxelNavVolume
    Request_Build(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_Build& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Abandons an in-flight build. An already-published octree stays published. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Request Cancel Build",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoxelNavVolume
    Request_CancelBuild(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_CancelBuild& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Declares a world-space region whose occupancy is no longer trustworthy, and schedules a LOCAL repair
     *  of exactly that region — never a rebake. Regions arriving before the repair starts are UNIONED into
     *  one repair, so many occluders moving in one frame cost one pass.
     *
     *  Only cells the dirty region reaches are re-probed; every other cell keeps the occupancy the last bake
     *  or repair recorded for it. When the repair completes, the volume publishes a new octree and bumps its
     *  epoch, which is what makes paths planned against the old one read Stale.
     *
     *  The completion delegate fires when the REPAIR ends, not when the region is accepted. A volume that
     *  has never published a bake reports Failed: there is no occupancy to carry over, and a build — not a
     *  repair — is the answer. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Request Mark Dirty",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoxelNavVolume
    Request_MarkDirty(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_MarkDirty& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // True once a bake has been published. A volume rebuilding over an existing bake still reads true —
    // the old octree stays live until the new one atomically replaces it.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Is Built")
    static bool
    Get_IsBuilt(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // Bumps on every completed build. A path planned against an older epoch is stale and must replan.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Epoch")
    static int32
    Get_BuildEpoch(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Stage")
    static ECk_VoxelNav_BuildStage
    Get_BuildStage(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Progress")
    static float
    Get_BuildProgress(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Stats")
    static FCk_VoxelNav_BuildStats
    Get_BuildStats(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Num Layers")
    static int32
    Get_NumLayers(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Repair Stage")
    static ECk_VoxelNav_RepairStage
    Get_RepairStage(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // What the LAST local repair cost and changed. `LeavesChanged` is the one to watch: repairs that change
    // nothing mean the movement threshold is finer than the volume's own resolution.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Repair Stats")
    static FCk_VoxelNav_RepairStats
    Get_RepairStats(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // The union of every dirty region waiting for a repair to start. Degenerate when none is pending.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Pending Dirty Bounds")
    static FBox
    Get_PendingDirtyBounds(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // True when this volume split into chunk children. A partitioned volume publishes no octree of its own:
    // every query on it routes to the chunk that owns the space being asked about.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Is Partitioned")
    static bool
    Get_IsPartitioned(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // Zero for an unpartitioned volume, which is the answer that keeps "how many chunks" and "is it
    // partitioned" from disagreeing.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Chunk Count")
    static int32
    Get_ChunkCount(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // The chunk at a partition-lattice index. A chunk is an ordinary volume handle, so every query and
    // every request on this page works on it.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Chunk")
    static FCk_Handle_VoxelNavVolume
    Get_Chunk(
        const FCk_Handle_VoxelNavVolume& InVolume,
        int32 InChunkIndex);

    // The chunk's index within its parent volume's partition, or -1 for a volume that is not a chunk.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Chunk Index")
    static int32
    Get_ChunkIndex(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // Crossings the adjacency bake found between this volume's chunks, counted in both directions.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Chunk Portal Count")
    static int32
    Get_ChunkPortalCount(
        const FCk_Handle_VoxelNavVolume& InVolume);

public:
    /** Everything a cross-chunk query needs about this volume's chunks, in partition-lattice order so a
     *  chunk's position IS its adjacency-table index. Empty for an unpartitioned volume. Not reflected:
     *  it hands out octree shares, which no Blueprint can hold. */
    static auto
    Get_ChunkSearchInputs(
        const FCk_Handle_VoxelNavVolume& InVolume) -> TArray<ck::voxelnav::FChunkSearchInput>;

    static auto
    Get_ChunkAdjacency(
        const FCk_Handle_VoxelNavVolume& InVolume) -> const ck::voxelnav::FChunkAdjacencyTable&;

public:
    /** Is the finest cell containing InLocation navigable?
     *
     *  A volume with no published bake answers false for every point, as does a point outside the baked
     *  cube: the volume makes no claim about space it has not rasterized, and answering "free" there would
     *  invite a path straight out of the volume. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Is Point Free")
    static bool
    Get_IsPointFree(
        const FCk_Handle_VoxelNavVolume& InVolume,
        FVector InLocation);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Bind To OnBuildComplete")
    static FCk_Handle_VoxelNavVolume
    BindTo_OnBuildComplete(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnBuildComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Unbind From OnBuildComplete")
    static FCk_Handle_VoxelNavVolume
    UnbindFrom_OnBuildComplete(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnBuildComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Bind To OnRepairComplete")
    static FCk_Handle_VoxelNavVolume
    BindTo_OnRepairComplete(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnRepairComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Unbind From OnRepairComplete")
    static FCk_Handle_VoxelNavVolume
    UnbindFrom_OnRepairComplete(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnRepairComplete& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
