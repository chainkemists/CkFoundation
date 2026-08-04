#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Post-search refinement of a planned waypoint list: visibility pruning first, then optional Catmull-Rom
// smoothing of what survives.
//
// A* over face-adjacent cells can only step axis-by-axis, so any route with a diagonal component comes back
// as a staircase of cell centres. Refinement is what turns that staircase into something an agent can fly:
// pruning removes every waypoint the octree can prove redundant, and smoothing rounds the corners the
// pruning left.
//
// BOTH steps ask the OCTREE ray-marcher whether a segment is clear, never the physics backend. The reason is
// in `Octree/CkVoxelNav_Octree_Raycast.h`: validating a path against a different structure than the one the
// search planned through is how a "shortcut" that physics calls clear ends up crossing a cell the search
// considered blocked - and the agent then flies a route no replan can reproduce.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    struct CKVOXELNAV_API FPathRefineParams
    {
        ECk_EnableDisable _VisibilityPruning = ECk_EnableDisable::Enable;

        ECk_EnableDisable _Smoothing = ECk_EnableDisable::Disable;

        // Segments each surviving span is divided into. One or less emits no interior point at all, which is
        // the same polyline that went in.
        int32 _SmoothingSubdivisions = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** `_Waypoints` is the refined path - the only output a follower needs. The rest is accounting a caller
     *  (or a test) uses to see what refinement bought: the counts and lengths on either side of it. */
    struct CKVOXELNAV_API FPathRefineResult
    {
        TArray<FVector> _Waypoints;

        int32 _RawWaypointCount = 0;

        float _RawLengthUu = 0.0f;
        float _RefinedLengthUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    CKVOXELNAV_API auto
    Get_PathLength(
        const TArray<FVector>& InWaypoints) -> float;

    /** Greedy forward walk: from each anchor, keep the FARTHEST later waypoint the octree reports a clear
     *  segment to, and drop everything between. Endpoints are never dropped, and a span the octree cannot
     *  prove clear keeps its original waypoint - so the result is always traversable by the same structure
     *  the search planned through, never longer than the input, and shorter whenever a waypoint was a corner
     *  rather than a necessity.
     *
     *  COMPLEXITY: worst case O(n^2) octree raycasts, best case O(n) - the scan for each anchor starts at the
     *  far end, so a path that is already one clear line costs one raycast in total. */
    CKVOXELNAV_API auto
    Prune_VisibleWaypoints(
        const FOctree& InOctree,
        const TArray<FVector>& InWaypoints) -> TArray<FVector>;

    /** Centripetal Catmull-Rom through the waypoints, PROPOSED span by span and kept only where the octree
     *  agrees.
     *
     *  CONTRACT: a span between two consecutive waypoints is replaced by its subdivided curve only when every
     *  sub-segment of that span is clear per the octree raycast AND every point the subdivision introduced
     *  stands in free space. A span that fails either test reverts to the straight segment it came from, and
     *  the two spans on either side of it are unaffected. Smoothing therefore can never make a path less
     *  traversable than the polyline it was handed: the worst case is that polyline, returned unchanged.
     *
     *  The point test is not redundant with the segment test. A segment leaving the baked volume reads as
     *  CLEAR by design (the bake makes no claim about space it never rasterized), so a curve that bulges out
     *  of the volume and back would pass a segment-only check while standing in space nothing has vouched
     *  for. */
    CKVOXELNAV_API auto
    Smooth_Waypoints(
        const FOctree& InOctree,
        const TArray<FVector>& InWaypoints,
        int32 InSubdivisions) -> TArray<FVector>;

    /** Runs the enabled steps in order - pruning, then smoothing - and reports what each cost and saved.
     *  An octree that was never built refines nothing and hands the input straight back. */
    CKVOXELNAV_API auto
    Refine_Waypoints(
        const FOctree& InOctree,
        const TArray<FVector>& InWaypoints,
        const FPathRefineParams& InParams) -> FPathRefineResult;
}

// --------------------------------------------------------------------------------------------------------------------
