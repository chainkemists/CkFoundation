#pragma once

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Build stages, as pure functions over (octree, scratch, backend). No member state and no process-wide
// cancel flag: a build is cancelled by dropping its scratch, which is also what makes a build resumable
// at any stage boundary.
//
// Stages that spend occupancy probes take a probe budget and report how much of it they spent; the
// geometry-free stages derive structure from occupancy already recorded in the scratch and spend none.
// A budgeted stage that returns _StageComplete == false left its cursor on the scratch and resumes from
// there on the next call with identical results.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** Everything a build in flight needs, and nothing a finished octree carries. */
    struct CKVOXELNAV_API FRasterizeScratch
    {
        /** Subdivided (blocked) Morton codes, indexed with a DELIBERATE off-by-one that runs through the
         *  whole rasterizer: `_BlockedNodes[L]` holds LAYER-(L+1) codes. Entry 0 therefore holds the
         *  layer-1 codes whose eight layer-0 children the leaf rasterizer expands. Sized LayerCount+1
         *  because the upward propagation writes one entry past the top layer.
         *
         *  A set, not a list: a parent with eight blocked children is reached eight times, and the
         *  duplicates would multiply every downstream membership test by up to 8x. */
        TArray<TSet<MortonCode>> _BlockedNodes;

        /** The leaf layer's Morton codes, expanded once from `_BlockedNodes[0]` and then walked by index.
         *  The leaf stages resume through a cursor into THIS array rather than re-deriving their position
         *  from a set, whose iteration order is an implementation detail. */
        TArray<MortonCode> _LeafMortonCodes;

        /** The subset of `_LeafMortonCodes` whose leaf-extent probe found geometry; those leaves are the
         *  only ones whose 64 sub-nodes are worth probing. */
        TSet<MortonCode> _OccludedLeafMortons;

        TMap<LeafIndex, MortonCode> _LeafIndexToParentMorton;

        int32 _Cursor = 0;
        int32 _SubCursor = 0;
        int32 _LayerCursor = 0;
    };

    struct CKVOXELNAV_API FRasterizeStageResult
    {
        bool _StageComplete = false;
        int32 _ProbesSpent = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    CKVOXELNAV_API auto
    Request_ResetScratch(
        FRasterizeScratch& InOutScratch) -> void;

    CKVOXELNAV_API auto
    Request_InitializeScratch(
        const FOctree& InOctree,
        FRasterizeScratch& InOutScratch) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKVOXELNAV_API FVolumeProbeResult
    {
        bool _VolumeIsEmpty = true;
        int32 _BodyCount = 0;
    };

    /** One broadphase sweep of the whole addressed cube. An empty answer means the volume is entirely
     *  free space and the build is finished before it starts - the single largest saving available, and
     *  the only place `Get_BodiesInBox` is spent. Sweeps the NAVIGATION bounds, not the authored ones:
     *  the rasterizer probes cells across the snapped power-of-two cube, so geometry that sits outside
     *  the authored box but inside the cube still has to be found. */
    CKVOXELNAV_API auto
    Stage_ProbeVolumeEmpty(
        const FOctree& InOctree,
        const ICk_VoxelNav_GeometryBackend& InBackend) -> FVolumeProbeResult;

    /** One probe per layer-1 cell, recording the blocked ones. This is the gate the whole cascade hangs
     *  off: an unoccupied layer-1 cell costs one probe and skips its 8 leaves and their 512 sub-nodes. */
    CKVOXELNAV_API auto
    Stage_RasterizeL1(
        const FOctree& InOctree,
        FRasterizeScratch& InOutScratch,
        const ICk_VoxelNav_GeometryBackend& InBackend,
        float InClearanceUu,
        int32 InProbeBudget) -> FRasterizeStageResult;

    // ----------------------------------------------------------------------------------------------------------------

    /** Lifts every blocked code to its parent, layer by layer. Bounded by the blocked-node count rather
     *  than by the volume's cube, so it cannot spike on a sparse world. */
    CKVOXELNAV_API auto
    Stage_PropagateBlockedUpward(
        const FOctree& InOctree,
        FRasterizeScratch& InOutScratch) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    /** Expands every blocked layer-1 code into its eight leaf-layer children and sizes the leaf store to
     *  match. The leaf store is parallel to layer 0 - leaf i belongs to layer-0 node i - so it is sized
     *  here, once, rather than grown as leaves are discovered. */
    CKVOXELNAV_API auto
    Stage_AllocateLeaves(
        FOctree& InOutOctree,
        FRasterizeScratch& InOutScratch) -> void;

    /** One probe per leaf, recording which leaves hold geometry. Creates no nodes: the layer-0 array has
     *  to be Morton-ordered and a budgeted pass cannot produce sorted output, so node creation waits for
     *  the sort stage. */
    CKVOXELNAV_API auto
    Stage_ProbeLeafOccupancy(
        const FOctree& InOctree,
        FRasterizeScratch& InOutScratch,
        const ICk_VoxelNav_GeometryBackend& InBackend,
        float InClearanceUu,
        int32 InProbeBudget) -> FRasterizeStageResult;

    /** Sorts the leaf-layer codes, then creates layer-0 in that order: an occluded leaf's node points at
     *  its own leaf entry, a free leaf's node stays childless, and every node records the parent code the
     *  link stage resolves later. One shot, not budgeted - a single sort over the leaf layer. */
    CKVOXELNAV_API auto
    Stage_SortLeafLayer(
        FOctree& InOutOctree,
        FRasterizeScratch& InOutScratch) -> void;

    /** 64 probes per occluded leaf, at the finest cell size. Dominates a bake's probe count by an order
     *  of magnitude, so the cursor descends INTO a leaf: a budget smaller than 64 still makes progress. */
    CKVOXELNAV_API auto
    Stage_RasterizeLeafSubNodes(
        FOctree& InOutOctree,
        FRasterizeScratch& InOutScratch,
        const ICk_VoxelNav_GeometryBackend& InBackend,
        float InClearanceUu,
        int32 InProbeBudget) -> FRasterizeStageResult;

    // ----------------------------------------------------------------------------------------------------------------

    /** Creates the nodes of one interior layer and links them to their children. Enumerates the eight
     *  children of each blocked parent rather than scanning the layer's whole cube. */
    CKVOXELNAV_API auto
    Stage_RasterizeLayer(
        FOctree& InOutOctree,
        FRasterizeScratch& InOutScratch,
        LayerIndex InLayerIndex) -> FRasterizeStageResult;

    CKVOXELNAV_API auto
    Stage_BuildParentLinks(
        FOctree& InOutOctree,
        const FRasterizeScratch& InScratch) -> void;

    CKVOXELNAV_API auto
    Stage_BuildNeighbourLinks(
        FOctree& InOutOctree,
        FRasterizeScratch& InOutScratch,
        LayerIndex InLayerIndex) -> FRasterizeStageResult;

    // ----------------------------------------------------------------------------------------------------------------

    /** `_Resolved` says the walk may stop here - either a neighbour was found, or the step provably left
     *  the layer's lattice and there is no neighbour to find. An unresolved result means "not on this
     *  layer", which is the caller's cue to climb to the parent and try again. */
    struct CKVOXELNAV_API FNeighbourSearchResult
    {
        bool _Resolved = false;
        FNodeAddress _NeighbourAddress;
    };

    CKVOXELNAV_API auto
    TryFind_NeighbourInDirection(
        const FOctree& InOctree,
        LayerIndex InLayerIndex,
        NodeIndex InNodeIndex,
        NeighbourDirection InDirection) -> FNeighbourSearchResult;
}

// --------------------------------------------------------------------------------------------------------------------
