#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Build stages, as pure functions over (octree, scratch). No member state and no process-wide cancel
// flag: a build is cancelled by dropping its scratch, which is also what makes a build resumable at any
// stage boundary.
//
// The stages here are the geometry-free half - they derive structure from occupancy already recorded in
// the scratch and spend no occupancy probes.
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

        TMap<LeafIndex, MortonCode> _LeafIndexToParentMorton;

        int32 _Cursor = 0;
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

    /** Lifts every blocked code to its parent, layer by layer. Bounded by the blocked-node count rather
     *  than by the volume's cube, so it cannot spike on a sparse world. */
    CKVOXELNAV_API auto
    Stage_PropagateBlockedUpward(
        const FOctree& InOctree,
        FRasterizeScratch& InOutScratch) -> void;

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
