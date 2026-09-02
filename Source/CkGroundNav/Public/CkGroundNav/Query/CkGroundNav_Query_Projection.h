#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THREAD CONTRACT: pure over the field handed in; writes only to the returned value or the caller's
// output span. Callable from any thread by anybody holding the field.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The nearest walkable surface to a point, inside the query's box, admitting only cells with
     * room for the query's body.
     *
     * CANDIDATES ARE RANKED VERTICAL BAND FIRST, THEN HORIZONTALLY. Every admitted cell in the box
     * is a candidate with a horizontal distance (from the query XY to the cell's square, zero when
     * inside it) and a vertical distance (from the query height to the cell's surface). The vertical
     * distance is bucketed by the field's step height; a lower bucket always wins, and within a
     * bucket the horizontally nearer cell wins, then the vertically nearer. So a surface a little way
     * sideways at the agent's own height beats a floor a storey below it directly underneath, and a
     * floor one step down under the agent's feet beats a surface across the room — which is what
     * keeps an agent on the floor it is standing on rather than snapping it to the one beneath.
     *
     * Down admits only surfaces at or below the query height, Up only those at or above, and Closest
     * both; the vertical reach is the matching extent either way.
     *
     * Statuses, in the order they are decided:
     *   Blocked   — the body's radius exceeds the field's clearance ceiling; nothing can admit it.
     *   Success   — a candidate qualified; the result carries it.
     *   Unbuilt   — nothing qualified and the box reached at least one tile that is not built. The
     *               answer may be in that tile, so this is never reported as NoSurface.
     *   NoSurface — nothing qualified and every tile the box reached is built, or the box reached no
     *               tile at all.
     *
     * The horizontal search expands outward in rings of cells from the query's own column and stops
     * once the best candidate found sits in the lowest vertical bucket and every remaining ring is
     * horizontally further than it — a ring cannot then improve on it. Every cell read is billed to
     * the result's cost.
     */
    CKGROUNDNAV_API auto
    Get_ProjectPoint(
        const FCk_GroundNav_Field&           InField,
        const FCk_GroundNav_ProjectionQuery& InQuery) -> FCk_GroundNav_ProjectionResult;

    /**
     * N projections at once. Amortizes nothing but the call: every element is answered exactly as
     * Get_ProjectPoint would answer it alone, and a batch that disagreed with the single form on any
     * element would be a defect. OutResults must be at least as long as InQueries.
     */
    CKGROUNDNAV_API auto
    Get_ProjectPoints_Batch(
        const FCk_GroundNav_Field&                     InField,
        TConstArrayView<FCk_GroundNav_ProjectionQuery> InQueries,
        TArrayView<FCk_GroundNav_ProjectionResult>     OutResults) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Whether an exact position is on walkable ground: the query's own column only, every layer,
     * admitting the layer whose surface lies within the vertical tolerance of the query height and
     * has room for the body. Where several qualify, the vertically nearest wins.
     *
     * The cheaper sibling of projection — no ring, so at most one cell read per layer — and it agrees
     * with a projection of zero horizontal extent and the same vertical reach both ways.
     *
     * Statuses: Blocked (radius over the ceiling), Success, Unbuilt (the column's tile is not built),
     * NoSurface (nothing qualified in a built tile, or no tile covers the point).
     */
    CKGROUNDNAV_API auto
    Get_IsNavigable(
        const FCk_GroundNav_Field&            InField,
        const FCk_GroundNav_IsNavigableQuery& InQuery) -> FCk_GroundNav_IsNavigableResult;

    /** N answers at once; element-for-element identical to N single calls. */
    CKGROUNDNAV_API auto
    Get_IsNavigable_Batch(
        const FCk_GroundNav_Field&                      InField,
        TConstArrayView<FCk_GroundNav_IsNavigableQuery> InQueries,
        TArrayView<FCk_GroundNav_IsNavigableResult>     OutResults) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
