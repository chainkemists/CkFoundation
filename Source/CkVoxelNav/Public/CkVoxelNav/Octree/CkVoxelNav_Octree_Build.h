#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Rasterize.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

#include "CkVoxelNav_Octree_Build.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// The resumable voxelization machine: the stage sequence, the cursor that survives between slices, and
// the one function that advances a build by a bounded amount of work.
//
// Engine-light in the same sense as the rest of Octree/ - reflection only, so a stage and its counters
// reach Blueprint and the debugger; no UObject, no entity, no world. Everything the build learns about
// geometry arrives through ICk_VoxelNav_GeometryBackend, which is what lets a whole bake run against a
// hand-authored box list in a hermetic test.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

/** The build's position in its stage sequence. Stage boundaries are the coarse resume points; the three
 *  probe-spending stages additionally carry a cursor, so a slice may end inside one of them. */
UENUM(BlueprintType)
enum class ECk_VoxelNav_BuildStage : uint8
{
    NotStarted,

    // Layer sizing from the authored bounds and cell size. Unbudgeted, one slice.
    InitializeOctree,

    // One broadphase sweep of the whole cube. An empty world completes the build outright.
    ProbeVolumeEmpty,

    // One occupancy probe per layer-1 cell. BUDGETED.
    RasterizeL1,

    // Lifts blocked codes to their parents, layer by layer. Unbudgeted, O(blocked).
    PropagateBlocked,

    // Expands blocked layer-1 codes into leaf codes and sizes the leaf store. Unbudgeted.
    AllocateLeaves,

    // One occupancy probe per leaf. BUDGETED.
    ProbeLeafOccupancy,

    // Morton-sorts the leaf layer and creates its nodes. Unbudgeted, one sort.
    SortLeafLayer,

    // 64 occupancy probes per occluded leaf. BUDGETED, and the stage that dominates a bake.
    RasterizeSubNodes,

    // Creates the interior layers, coarsest last. One layer per slice, no probes.
    RasterizeLayers,

    // Points every leaf at its layer-1 parent. Unbudgeted.
    BuildParentLinks,

    // Links each layer's nodes to their face neighbours, finest last. One layer per slice, no probes.
    BuildNeighbourLinks,

    Complete,
    Failed
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoxelNav_BuildStage);

// --------------------------------------------------------------------------------------------------------------------

/** What a finished bake cost and produced. The probe counts are the numbers to watch: they are
 *  deterministic for a given scene, so a regression in the hierarchy's pruning shows up here as a
 *  changed count long before it shows up as a frame-time spike. */
USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_VoxelNav_BuildStats
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VoxelNav_BuildStats);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _OccupancyProbes = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Slices = 0;

    // Bodies the whole-volume broadphase sweep returned. Zero means the bake saw an empty world.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _BroadphaseBodyCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _BlockedLayer1Cells = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _LeafNodeCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _OccludedLeafCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _TotalNodeCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _BuildSeconds = 0.0f;

    // Free cells the merge pass was handed. Zero when merging is off, and the A/B baseline when it is on.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _PlainCellCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _MergedCellCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _MergeSeconds = 0.0f;

public:
    CK_PROPERTY(_OccupancyProbes);
    CK_PROPERTY(_Slices);
    CK_PROPERTY(_BroadphaseBodyCount);
    CK_PROPERTY(_BlockedLayer1Cells);
    CK_PROPERTY(_LeafNodeCount);
    CK_PROPERTY(_OccludedLeafCount);
    CK_PROPERTY(_TotalNodeCount);
    CK_PROPERTY(_BuildSeconds);
    CK_PROPERTY(_PlainCellCount);
    CK_PROPERTY(_MergedCellCount);
    CK_PROPERTY(_MergeSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** What the build needs from whoever owns it, which is everything the octree cannot derive itself. */
    struct CKVOXELNAV_API FBuildParams
    {
        FBox _VolumeBounds = FBox{ForceInit};
        float _FinestCellSizeUu = 0.0f;

        // Added to every probe half-extent; grows obstacles and shrinks free space. Clearance is the
        // rasterizer's business, never the backend's.
        float _ClearanceUu = 0.0f;

        /** Coalesce the finished free-cell set into merged boxes (Octree/CkVoxelNav_Octree_Merge.h). The
         *  bake is IDENTICAL either way - merging only adds a table beside the octree - but a search over a
         *  merged octree expands merged cells, so a route's cell list is a different (coarser) answer to the
         *  same question. Anything asserting on octree ADDRESSES along a route wants it off, and should say
         *  so rather than inherit whatever the caller defaulted to. */
        ECk_EnableDisable _CellMerging = ECk_EnableDisable::Enable;
    };

    /** How much work one slice may do.
     *
     *  Probe count is the PRIMARY budget and wall-clock is only a guard, deliberately: a probe count is
     *  deterministic, so a test can assert a bake's exact cost and catch a pruning regression, whereas a
     *  time-only budget makes that count machine-dependent and the assertion flaky.
     *
     *  The guard is checked between stage steps, so it bounds when a slice STOPS, not how long a single
     *  unbudgeted stage may run. */
    struct CKVOXELNAV_API FBuildBudget
    {
        int32 _MaxOccupancyProbes = 2048;

        // Zero or negative disables the guard.
        float _MaxSeconds = 0.004f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct FBuildState;

    CKVOXELNAV_API auto
    Request_ResetBuild(
        FBuildState& InOutState) -> void;

    /** Abandons a build outright. The half-built octree goes with it: a partial bake is not navigation
     *  data, and keeping one around invites a consumer to read it. */
    CKVOXELNAV_API auto
    Request_FailBuild(
        FBuildState& InOutState) -> void;

    /** Advances the build by at most one budget's worth of work and returns. Calling it on a finished
     *  build is a no-op, so a driver may call it unconditionally and read the stage afterwards. */
    CKVOXELNAV_API auto
    Request_AdvanceBuild(
        FBuildState& InOutState,
        const FBuildParams& InParams,
        const ICk_VoxelNav_GeometryBackend& InBackend,
        const FBuildBudget& InBudget) -> void;

    /** Hands the finished octree over as an immutable shared value and empties the build state. This is
     *  the ONLY way the octree leaves the machine: publishing it as TSharedPtr<const FOctree> is what
     *  makes post-bake reads lock-free, and moving it out is what stops a rebuild from mutating an
     *  octree a search is still walking. Returns null unless the build completed. */
    CKVOXELNAV_API auto
    Request_ReleaseBuiltOctree(
        FBuildState& InOutState) -> TSharedPtr<const FOctree>;

    // ----------------------------------------------------------------------------------------------------------------

    /** A build in flight: the stage, the octree being assembled, and the rasterizer scratch. Owning all
     *  three together is what makes the build resumable - a slice ends by simply returning, and the next
     *  one picks up from the same state with no re-derivation.
     *
     *  The octree in here is NOT readable as navigation data. It is a half-built structure until the
     *  Complete transition marks it, and Get_IsValid on it answers false until then. */
    struct CKVOXELNAV_API FBuildState
    {
    public:
        friend CKVOXELNAV_API auto Request_ResetBuild(FBuildState&) -> void;
        friend CKVOXELNAV_API auto Request_FailBuild(FBuildState&) -> void;
        friend CKVOXELNAV_API auto Request_AdvanceBuild(
            FBuildState&, const FBuildParams&, const ICk_VoxelNav_GeometryBackend&, const FBuildBudget&) -> void;
        friend CKVOXELNAV_API auto Request_ReleaseBuiltOctree(FBuildState&) -> TSharedPtr<const FOctree>;

    public:
        auto Get_Stage() const -> ECk_VoxelNav_BuildStage        { return _Stage; }
        auto Get_Stats() const -> const FCk_VoxelNav_BuildStats& { return _Stats; }

        auto Get_IsFinished() const -> bool
        {
            return _Stage == ECk_VoxelNav_BuildStage::Complete || _Stage == ECk_VoxelNav_BuildStage::Failed;
        }

        /** Stage granularity, not probe granularity: a caller wanting a smooth bar should interpolate it
         *  itself rather than have the machine pay for a finer estimate every slice. */
        auto
        Get_Progress01() const -> float;

    private:
        ECk_VoxelNav_BuildStage _Stage = ECk_VoxelNav_BuildStage::NotStarted;
        FOctree _Octree;
        FRasterizeScratch _Scratch;
        FCk_VoxelNav_BuildStats _Stats;
        double _StartSeconds = 0.0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
