#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THREAD CONTRACT: safe to call from any thread. Both queries operate entirely on the immutable field
// snapshot the caller already holds and write only into the caller's own storage. Holding the snapshot
// is the caller's guarantee of self-consistency — a concurrent republish cannot affect it. There is no
// lock, no shared scratch and no lazily built index anywhere beneath these calls.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The boundary runs within a radius of a point, nearest first, on the surface the point stands on.
     *
     * The point is resolved to its surface first; the answer is then every precomputed run of the
     * same reachability component within the radius and the vertical window, ordered by distance to
     * the point and truncated to the cap. An empty answer on open floor is a Success — there are no
     * walls in range — and only a point that resolves to no surface fails.
     *
     * Statuses: Blocked (radius over the ceiling), Unbuilt, NoSurface, Success.
     */
    CKGROUNDNAV_API auto
    Get_BoundarySegments(
        const FCk_GroundNav_Field&             InField,
        const FCk_GroundNav_BoundaryQuery&     InQuery,
        TArray<FCk_GroundNav_BoundarySegment>& OutSegments) -> ECk_NavSurface_QueryStatus;

    /**
     * The single nearest boundary run to a point, the closest point on it, and the distance — for
     * the consumer that wants one wall, not a set. Expanding rings of index buckets from the point's
     * own bucket outward, across tile boundaries, stopping once no further ring can be nearer than
     * what was found. NoSurface when nothing lies within the radius.
     */
    CKGROUNDNAV_API auto
    Get_ClosestBoundary(
        const FCk_GroundNav_Field&                InField,
        const FCk_GroundNav_ClosestBoundaryQuery& InQuery) -> FCk_GroundNav_ClosestBoundaryResult;
}

// --------------------------------------------------------------------------------------------------------------------
