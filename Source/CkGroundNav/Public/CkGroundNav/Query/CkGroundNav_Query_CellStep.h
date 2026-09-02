#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THE definition of adjacency at query time. The bake keeps no connection field once a tile is
// published; what it keeps is plates and the crossings between them, and every query that moves from
// one cell to a neighbour goes through this one function so that the walk, the raycast and the search
// cannot drift apart on what "next to" means.
//
// THREAD CONTRACT: pure over the field handed in. Callable from any thread by anybody holding it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /** What happened when a body tried to step from one cell to the next. */
    enum class ECk_GroundNav_StepVerdict : uint8
    {
        // The neighbour exists on a layer the body may step onto, and its clearance admits the body.
        Admitted,

        // The neighbour exists but has less room than the body needs.
        Blocked,

        // No adjacency: the neighbour column has no surface the body can reach from here — a wall, a
        // hole, a ledge, or a plate boundary with no crossing covering this cell pair.
        NoCrossing,

        // The step leaves the tile into one that is not built.
        Unbuilt,

        // The step leaves the field.
        OutsideField
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Step from a surface to the neighbouring cell in one lattice direction.
     *
     * Inside a plate the neighbour is simply the next cell of the same plate — the merge criteria
     * already guarantee the step is walkable. Across a plate boundary the step is admitted only where
     * a portal covers this exact cell pair; across a tile boundary, only where a seam portal does. A
     * portal's own clearance is not re-checked: it is the best of the per-cell-pair minima, so
     * admitting both cells on their own clearance is at least as strict.
     *
     * The out-parameters are written only on Admitted and Blocked (where the neighbour exists). One
     * cell read is billed per neighbour examined.
     */
    CKGROUNDNAV_API auto
    Get_StepAcross(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_SurfaceRef& InFrom,
        int32                           InDirection,
        const FCk_GroundNav_QueryAgent& InAgent,
        FCk_GroundNav_SurfaceRef&       OutTo,
        float&                          OutSurfaceZUu,
        float&                          OutClearanceUu,
        FCk_GroundNav_QueryCost&        InOutCost) -> ECk_GroundNav_StepVerdict;
}

// --------------------------------------------------------------------------------------------------------------------
