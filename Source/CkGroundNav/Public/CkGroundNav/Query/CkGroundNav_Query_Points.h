#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THREAD CONTRACT: safe on any thread against the immutable field snapshot the caller holds. The
// random generators are caller-seeded and deterministic for a seed and a field epoch; nothing here
// keeps state between calls.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Random points on walkable ground within a horizontal radius of an origin, on every storey the
     * disc touches, uniformly by AREA: a plate is chosen with probability proportional to the number
     * of its cells inside the disc that admit the body, a cell uniformly among those, and a point
     * uniformly inside the cell. Connectivity is not consulted — that is the path-distance generator.
     * NoSurface when no cell in the disc admits the body.
     */
    CKGROUNDNAV_API auto
    Get_RandomPointsInRadius(
        const FCk_GroundNav_Field&             InField,
        const FCk_GroundNav_RandomPointsQuery& InQuery) -> FCk_GroundNav_PointsResult;

    /**
     * Random points whose WALKED distance from the origin lies in [min, max] — the query a radius
     * cannot express, because a point across a wall is near and far at once. Runs the flood fill
     * with the maximum as its limit, draws area-weighted points over the reached plates and keeps
     * those whose flood distance is in range; the draw count is bounded, so the answer can be short
     * of _Count and says how many draws it spent. Statuses follow the flood fill's.
     */
    CKGROUNDNAV_API auto
    Get_RandomPointsByPathDistance(
        const FCk_GroundNav_Field&                   InField,
        const FCk_GroundNav_PathDistancePointsQuery& InQuery) -> FCk_GroundNav_PointsResult;

    /**
     * A regular lattice of points over walkable ground inside a box, one point per lattice position
     * per storey that has an admitted cell there. With alignment enabled the lattice is phased to
     * the field's origin, so two overlapping queries agree on every shared position; otherwise it is
     * phased to the box's own minimum corner. Deterministic.
     */
    CKGROUNDNAV_API auto
    Get_GridPoints(
        const FCk_GroundNav_Field&           InField,
        const FCk_GroundNav_GridPointsQuery& InQuery) -> FCk_GroundNav_PointsResult;
}

// --------------------------------------------------------------------------------------------------------------------
