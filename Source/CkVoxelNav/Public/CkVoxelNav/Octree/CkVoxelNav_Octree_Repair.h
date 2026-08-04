#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Build.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Rasterize.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

#include "CkVoxelNav_Octree_Repair.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// LOCAL REPAIR: re-rasterizing the cells a moved obstacle dirtied, without re-probing anything else and
// without ever mutating a published octree.
//
// THE CONTRACT (test-pinned):
//   1. Occupancy is re-derived by PROBING only for cells whose inflated probe box intersects the dirty
//      bounds. Every other cell keeps the occupancy the last bake or repair recorded for it, bit for bit.
//   2. A repair NEVER mutates the octree it repairs. It assembles a whole new one and hands it over, so a
//      search walking the published structure keeps walking a consistent one for as long as it holds the
//      pointer. This is the same publish discipline the bake uses, and it is what makes the repair safe
//      without a lock.
//   3. Structure is DERIVED, never patched. The node set of an octree is a pure function of its blocked
//      cell set, so the repair updates that set over the dirty range and then re-runs the bake's own
//      structural stages against it. No node array is ever inserted into, swap-removed from, or re-sorted
//      in place.
//
// Rule 3 is the whole reason this file exists rather than an in-place edit. Node identity here is
// (layer, INDEX): parent links, child links and all six neighbour links per node store array positions,
// and the layers are Morton-sorted because every lookup binary-searches them. Upstream's dynamic path
// swap-removed and appended layer-0 nodes in place (Nav3DVolumeNavigationData.cpp:1245/1297) and never
// re-sorted layer 0 at all, so the first obstacle movement invalidated every link and every binary search
// in the octree. Deriving the structure instead makes that class of corruption unrepresentable.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

/** The repair's position in its stage sequence. The first three stages are the only ones that touch
 *  geometry; everything after them re-derives structure from occupancy already in hand. */
UENUM(BlueprintType)
enum class ECk_VoxelNav_RepairStage : uint8
{
    NotStarted,

    // Sizes the replacement octree and reads the published one's blocked cells and leaf occupancy back out
    // of it. Unbudgeted, O(published layer-0 nodes).
    SeedFromPublished,

    // One occupancy probe per layer-1 cell the dirty bounds reach. BUDGETED.
    ProbeDirtyCells,

    // One leaf probe, plus 64 sub-node probes for an occupied leaf, per leaf the dirty bounds reach.
    // BUDGETED, and the stage that dominates a repair.
    ProbeDirtyLeaves,

    // Everything below spends ZERO probes: the same structural stages the bake runs, over the updated
    // blocked-cell set and the recorded occupancy.
    PropagateBlocked,
    AllocateLeaves,
    SortLeafLayer,
    ApplyOccupancy,
    RasterizeLayers,
    BuildParentLinks,
    BuildNeighbourLinks,

    Complete,
    Failed
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoxelNav_RepairStage);

// --------------------------------------------------------------------------------------------------------------------

/** What a repair cost and how much of the octree it actually touched. `_LeavesChanged` is the number to
 *  watch: a repair that probes hundreds of leaves and changes none is a movement threshold set too fine. */
USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_VoxelNav_RepairStats
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VoxelNav_RepairStats);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _OccupancyProbes = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Slices = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _DirtyCellsProbed = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _DirtyLeavesProbed = 0;

    // Leaves whose 4x4x4 occupancy word came out DIFFERENT. Every other leaf in the volume is carried over
    // untouched, which is the local-repair contract expressed as a number.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _LeavesChanged = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _LeafNodeCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _TotalNodeCount = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _RepairSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _MergedCellCount = 0;

    // Merged boxes taken from the published table untouched because the dirty bounds never reached them.
    // The merge half of the local-repair contract, expressed as a number: what is NOT re-derived.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _MergedCellsCarriedOver = 0;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _MergeSeconds = 0.0f;

public:
    CK_PROPERTY(_OccupancyProbes);
    CK_PROPERTY(_Slices);
    CK_PROPERTY(_DirtyCellsProbed);
    CK_PROPERTY(_DirtyLeavesProbed);
    CK_PROPERTY(_LeavesChanged);
    CK_PROPERTY(_LeafNodeCount);
    CK_PROPERTY(_TotalNodeCount);
    CK_PROPERTY(_RepairSeconds);
    CK_PROPERTY(_MergedCellCount);
    CK_PROPERTY(_MergedCellsCarriedOver);
    CK_PROPERTY(_MergeSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** The bake params the replacement octree must be sized with - they MUST match the ones the published
     *  octree was baked with, or the two structures address different lattices - plus the world-space box
     *  the obstacle dirtied. */
    struct CKVOXELNAV_API FRepairParams
    {
        FBox _VolumeBounds = FBox{ForceInit};
        float _FinestCellSizeUu = 0.0f;
        float _ClearanceUu = 0.0f;

        /** Union of the obstacle's OLD and NEW world bounds. Both halves are required: the new bounds say
         *  where space became blocked, the old ones say where it became free again, and a repair given only
         *  the new bounds leaves the obstacle's departure point permanently occupied. */
        FBox _DirtyBounds = FBox{ForceInit};

        /** MUST match the value the published octree was baked with. A repair that merges where the bake did
         *  not (or the reverse) republishes the volume in a different cell representation, and a path planned
         *  against the old one would be re-validated against cells that do not exist. */
        ECk_EnableDisable _CellMerging = ECk_EnableDisable::Enable;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The Morton codes of every cell on InLayerIndex whose cell box intersects InBounds, in ascending
     *  order. This is the replacement for upstream's two whole-octree scans per dirty box
     *  (Nav3DData.cpp:2240-2282, whose result was a dead local): a dirty box maps DIRECTLY onto a lattice
     *  coordinate range, so the cost is the size of the dirty region rather than the size of the octree.
     *
     *  Coordinates are clamped to the layer's lattice, and a box that misses it entirely yields nothing -
     *  upstream fed unclamped, possibly NEGATIVE coordinates straight into the Morton encoder. */
    CKVOXELNAV_API auto
    Get_CellMortonsIntersectingBounds(
        const FOctree& InOctree,
        LayerIndex InLayerIndex,
        const FBox& InBounds,
        TArray<MortonCode>& OutMortonCodes) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    struct FRepairState;

    CKVOXELNAV_API auto
    Request_ResetRepair(
        FRepairState& InOutState) -> void;

    CKVOXELNAV_API auto
    Request_FailRepair(
        FRepairState& InOutState) -> void;

    /** Opens a repair against a PUBLISHED octree. The state keeps its own reference to that octree for the
     *  whole repair, so the volume may publish a replacement mid-repair without pulling the ground out from
     *  under it. Rejects a null or unbuilt octree: there is nothing to repair locally until a bake exists. */
    CKVOXELNAV_API auto
    Request_BeginRepair(
        FRepairState& InOutState,
        TSharedPtr<const FOctree> InPublishedOctree) -> bool;

    /** Advances the repair by at most one budget's worth of work and returns. A no-op on a finished repair,
     *  so a driver may call it unconditionally and read the stage afterwards. */
    CKVOXELNAV_API auto
    Request_AdvanceRepair(
        FRepairState& InOutState,
        const FRepairParams& InParams,
        const ICk_VoxelNav_GeometryBackend& InBackend,
        const FBuildBudget& InBudget) -> void;

    /** Hands the repaired octree over as an immutable shared value and empties the repair state. Returns
     *  null unless the repair completed. */
    CKVOXELNAV_API auto
    Request_ReleaseRepairedOctree(
        FRepairState& InOutState) -> TSharedPtr<const FOctree>;

    // ----------------------------------------------------------------------------------------------------------------

    /** A repair in flight.
     *
     *  `_LeafOccupancy` is the repair's source of truth for everything OUTSIDE the dirty region: it is read
     *  once out of the published octree and then updated only where a probe was actually spent. Keying it by
     *  Morton code rather than by leaf index is deliberate - leaf indices are positions in an array the
     *  structural stages rebuild, and carrying occupancy on an index across that rebuild is precisely the
     *  mistake that corrupts an octree. */
    struct CKVOXELNAV_API FRepairState
    {
    public:
        friend CKVOXELNAV_API auto Request_ResetRepair(FRepairState&) -> void;
        friend CKVOXELNAV_API auto Request_FailRepair(FRepairState&) -> void;
        friend CKVOXELNAV_API auto Request_BeginRepair(FRepairState&, TSharedPtr<const FOctree>) -> bool;
        friend CKVOXELNAV_API auto Request_AdvanceRepair(
            FRepairState&, const FRepairParams&, const ICk_VoxelNav_GeometryBackend&, const FBuildBudget&) -> void;
        friend CKVOXELNAV_API auto Request_ReleaseRepairedOctree(FRepairState&) -> TSharedPtr<const FOctree>;

    public:
        auto Get_Stage() const -> ECk_VoxelNav_RepairStage        { return _Stage; }
        auto Get_Stats() const -> const FCk_VoxelNav_RepairStats& { return _Stats; }

        auto Get_IsFinished() const -> bool
        {
            return _Stage == ECk_VoxelNav_RepairStage::Complete || _Stage == ECk_VoxelNav_RepairStage::Failed;
        }

        auto
        Get_Progress01() const -> float;

    private:
        ECk_VoxelNav_RepairStage _Stage = ECk_VoxelNav_RepairStage::NotStarted;

        TSharedPtr<const FOctree> _PublishedOctree;
        FOctree _Octree;
        FRasterizeScratch _Scratch;

        TMap<MortonCode, uint64> _LeafOccupancy;

        TArray<MortonCode> _DirtyCellMortons;
        TArray<MortonCode> _DirtyLeafMortons;

        int32 _CellCursor = 0;
        int32 _LeafCursor = 0;
        int32 _SubCursor = 0;

        // The word being assembled for the leaf at `_LeafCursor`, so a slice may end mid-leaf and resume
        // without re-probing the sub-nodes it already paid for.
        uint64 _PendingLeafWord = 0;

        FCk_VoxelNav_RepairStats _Stats;
        double _StartSeconds = 0.0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
