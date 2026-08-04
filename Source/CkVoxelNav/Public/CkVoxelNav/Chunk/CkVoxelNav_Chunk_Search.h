#pragma once

#include "CkVoxelNav/Chunk/CkVoxelNav_Chunk_Types.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"
#include "CkVoxelNav/Path/CkVoxelNav_Path_Graph.h"
#include "CkVoxelNav/Path/CkVoxelNav_Path_Refine.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Searching a PARTITIONED volume: the decision tree over chunks, and the per-chunk segments it stitches.
//
// The in-chunk search is `Search_PathGraph`, unchanged - a chunk is an ordinary baked octree and nothing
// here reaches inside one. What this file adds is the layer above: which chunk holds an endpoint, which
// chunks a route must cross, where it crosses them, and how the resulting segments join up.
//
// REFINEMENT IS PER SEGMENT, AND THAT IS NOT AN OPTIMISATION CHOICE. The octree ray-marcher can only answer
// for the structure it belongs to, so a span reaching from one chunk into another is unprovable by either
// chunk's octree: pruning it would mean asserting free space nothing baked, and asserting it against the
// WRONG structure is exactly the failure the refinement pass was written to avoid. A cross-chunk route is
// therefore refined chunk by chunk, and the two waypoints either side of a crossing always survive.
//
// CROSS-VOLUME IS OUT OF SCOPE. Upstream's fourth branch (FindPathCrossVolume, with its volume exit/entry
// point machinery) has no equivalent here: this file plans within ONE volume's chunks.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** One chunk's leg of a route. Kept separate rather than pre-concatenated because refinement runs per
     *  segment against that segment's own octree. */
    struct CKVOXELNAV_API FChunkedPathSegment
    {
        FChunkId _ChunkId;
        int32 _ChunkIndex = FChunkId::InvalidIndex;

        TArray<FCellId> _Cells;
        TArray<FVector> _Waypoints;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** A finished cross-chunk plan. `_Waypoints` is the stitched route; `_Portals` names the crossings it
     *  took, in order, so a caller can tell where the route left one bake and entered the next. */
    struct CKVOXELNAV_API FChunkedPathPlan
    {
        ECk_VoxelNav_PathSearchOutcome _Outcome = ECk_VoxelNav_PathSearchOutcome::InvalidRequest;

        TArray<FChunkedPathSegment> _Segments;
        TArray<FVector> _Waypoints;
        TArray<FChunkPortal> _Portals;

        float _TotalCost = 0.0f;
        int32 _Iterations = 0;
        int32 _PlannedAgainstEpoch = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Which chunk holds a point. 3D containment, which is upstream's bug fixed: its ContainsPoint tested
     *  X and Y only (Nav3DDataChunkActor.cpp:261, IsInsideXY), so in a plugin whose entire purpose is
     *  vertical navigation, vertically-stacked chunks resolved to whichever one came first.
     *
     *  A point inside no chunk falls back to the NEAREST chunk by distance to its box rather than failing:
     *  every chunk's octree addresses a power-of-two cube larger than its authored box, so a point just
     *  outside the partition is very often still resolvable - and if it is not, the in-chunk search says so
     *  with the same EndpointUnresolvable an unpartitioned volume would give. */
    CKVOXELNAV_API auto
    TryGet_ChunkIndexContainingPoint(
        const TArray<FChunkSearchInput>& InChunks,
        const FVector& InPoint) -> int32;

    /** Breadth-first over the adjacency graph. BFS rather than A* on purpose: chunk counts are tiny, every
     *  edge is one face crossing, and fewest-crossings is the property the segment stitching wants. */
    CKVOXELNAV_API auto
    Get_ChunkPath(
        const FChunkAdjacencyTable& InAdjacency,
        int32 InFromChunkIndex,
        int32 InToChunkIndex,
        TArray<int32>& OutChunkPath) -> bool;

    /** Two addresses name the same CELL when they are equal, or when both are layer-0 addresses into the
     *  same FULLY-FREE leaf: such a leaf is one cell, and the cell seam reports the leaf's own centre and
     *  extent for every one of its 64 sub-node indices, so the sub-node field carries no meaning there. */
    CKVOXELNAV_API auto
    Get_CellAddressesNameSameCell(
        const FOctree& InOctree,
        const FNodeAddress& InLeft,
        const FNodeAddress& InRight) -> bool;

    // ----------------------------------------------------------------------------------------------------------------

    /** The decision tree: same chunk runs the existing single-octree search and returns one segment;
     *  different chunks run BFS over the adjacency and then one in-chunk search per chunk on the way,
     *  portal cell centre to portal cell centre.
     *
     *  The iteration cap in InParams applies PER SEGMENT. A cross-chunk route is several searches, and
     *  splitting one budget across them would make a route's success depend on how many chunks it happened
     *  to cross. */
    CKVOXELNAV_API auto
    Search_ChunkedPathGraph(
        const TArray<FChunkSearchInput>& InChunks,
        const FChunkAdjacencyTable& InAdjacency,
        const FPathSearchParams& InParams) -> FChunkedPathPlan;

    /** Refines each segment against its own chunk's octree and re-stitches. The crossing waypoints are
     *  never candidates for removal - see the per-segment note at the top of this file. */
    CKVOXELNAV_API auto
    Refine_ChunkedWaypoints(
        const TArray<FChunkSearchInput>& InChunks,
        const FChunkedPathPlan& InPlan,
        const FPathRefineParams& InParams) -> FPathRefineResult;
}

// --------------------------------------------------------------------------------------------------------------------
