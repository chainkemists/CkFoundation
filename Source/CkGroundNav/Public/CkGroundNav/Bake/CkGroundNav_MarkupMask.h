#pragma once

#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The one reduction from an authored markup volume to the cells it covers. Everything that applies
// markup — the walkability demotion and the cost multiply alike — asks these and nothing else, so a
// volume's footprint has a single definition and cannot drift between the stage that blocks ground
// and the stage that prices it.
//
// Pure: no world, no registry, no physics.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The axis-aligned world bounds of a markup's transformed shape.
     *
     * Exact for a box — the transformed corners' extremes — and analytic for the round shapes, taken
     * as the support of the shape along each world axis under the transform's rotation AND scale. A
     * non-uniformly scaled sphere is an ellipsoid and its bounds say so; padding a radius by the
     * largest scale component instead would over-report by the scale ratio on every other axis.
     *
     * Returns an INVALID box for a shape with a zero or non-finite extent on any axis, for an
     * unauthored shape, and for a transform that is not invertible. The bounds of a degenerate volume
     * and the bounds of a volume that covers nothing are different answers, and only the second is a
     * box.
     */
    CKGROUNDNAV_API auto
    Get_MarkupWorldBounds(
        const FCk_GroundNav_MarkupRecord& InMarkup) -> FBox;

    /**
     * The inclusive cell rectangle whose CLOSED squares meet the markup's bounds in XY, clamped to a
     * lattice of InSizeX by InSizeY cells starting at InLatticeOriginXY.
     *
     * Closed, so a bound falling exactly on a cell line claims the cells on both sides of it — the
     * same rule Get_CellAddressesAt answers a lookup with, and by the same exact comparison. Unset
     * when the footprint misses the lattice entirely, which is not the same answer as an empty
     * rectangle and must not be flattened into one.
     *
     * This is a CONSERVATIVE superset: it bounds which cells to ask about, never which are covered.
     * Get_IsMarkupCoveringCell decides that.
     */
    CKGROUNDNAV_API auto
    Get_MarkupCellRect(
        const FCk_GroundNav_MarkupRecord& InMarkup,
        const FVector2D&                  InLatticeOriginXY,
        float                             InCellSizeUu,
        int32                             InSizeX,
        int32                             InSizeY) -> TOptional<FCk_GroundNav_CellRect>;

    /**
     * Whether the markup covers one cell's surface: the cell's closed square, taken at InSurfaceZUu,
     * against the shape itself rather than its bounds.
     *
     * THE TEST IS PER SPAN, NOT PER COLUMN. A volume painted on an upper storey shares its column
     * with the floor beneath it, and a footprint test alone would cover both. The surface height is
     * what separates them, so a caller asks once per span it wants decided.
     *
     * Exact and analytic, evaluated in the shape's own frame so a rotated or non-uniformly scaled
     * volume is answered by its true silhouette and not by an axis-aligned stand-in. A cell square
     * that merely touches the shape IS covered, matching the closed-square rule the rectangle above
     * is built on.
     *
     * Answers geometry only. Whether a disabled markup should be applied is the caller's policy, and
     * folding it in here would make a geometric predicate silently mean two things.
     */
    CKGROUNDNAV_API auto
    Get_IsMarkupCoveringCell(
        const FCk_GroundNav_MarkupRecord& InMarkup,
        const FVector2D&                  InCellMinXY,
        float                             InCellSizeUu,
        float                             InSurfaceZUu) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
