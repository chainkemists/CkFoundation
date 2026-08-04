#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Revelles parametric octree traversal: marches a segment through a BUILT octree and answers whether the
// bake blocks it, without touching the world or any physics backend.
//
// WHICH SEGMENT TEST TO USE. There are two, and they answer different questions:
//
//   * THIS one - Raycast / Get_IsSegmentBlocked over an FOctree - asks "does the BAKE block this segment".
//     It reads only octree occupancy, so it costs no physics query, it is safe to issue thousands of times
//     per frame, and it already accounts for the agent clearance the rasterizer inflated every cell by. It
//     is the right test for PATHFINDING: visibility pruning of a planned path, corridor validation,
//     line-of-sight between two cells. Its resolution is the bake's - it cannot see anything finer than a
//     leaf sub-node, and it is conservative there, because a sub-node holding any geometry at all is
//     wholly occluded.
//
//   * ICk_VoxelNav_GeometryBackend::Get_IsSegmentBlocked asks "does PHYSICS block this segment" against the
//     live world. It sees exact collision shapes and geometry the bake never resolved, and it costs a
//     narrowphase query. It is the right test at BAKE time (validating what was rasterized) and for
//     tactical queries that must beat cell resolution - never for validating a path the search already
//     planned through the octree, which would be asking a different structure than the one that was
//     planned against.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

/** What a segment met inside the octree. `_Distance` and everything derived from it are meaningful only
 *  when `_IsBlocked`; the impact point is the entry point of the first occluded cell along the segment,
 *  which is a CELL face, not the surface of the geometry that occluded it. */
struct CKVOXELNAV_API FCk_VoxelNav_RaycastResult
{
    bool _IsBlocked = false;

    // Along the segment, in uu from its start.
    double _Distance = 0.0;

    FVector _ImpactPoint = FVector::ZeroVector;

    // The axis-aligned outward normal of the cell face the segment entered through.
    FVector _ImpactNormal = FVector::ZeroVector;

    // The occluded cell: a leaf sub-node where the bake resolved that finely, the leaf itself where the
    // whole leaf is occluded.
    ck::voxelnav::FNodeAddress _BlockingCell;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** Marches the segment from InFrom to InTo through the octree and reports the first occluded cell it
     *  enters.
     *
     *  A segment that never enters the octree's navigation bounds reads as CLEAR, and so does the part of
     *  a segment that leaves them. That is the opposite of Get_IsPositionFree, which reports NOT free
     *  outside the bounds, and the asymmetry is deliberate: a point query asks "may an agent stand here",
     *  which unrasterized space cannot answer yes to, while a segment query asks "did the bake put
     *  something in the way", and outside the bake there is nothing in the way. A caller that must keep a
     *  path inside the volume tests its endpoints, not its segments.
     *
     *  A zero-length segment is never blocked: it traverses no space. */
    CKVOXELNAV_API auto
    Raycast(
        const FOctree& InOctree,
        const FVector& InFrom,
        const FVector& InTo) -> FCk_VoxelNav_RaycastResult;

    /** Raycast's boolean form, for the visibility-pruning caller that discards the hit data anyway. */
    CKVOXELNAV_API auto
    Get_IsSegmentBlocked(
        const FOctree& InOctree,
        const FVector& InFrom,
        const FVector& InTo) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
