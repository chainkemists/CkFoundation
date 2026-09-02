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
     * The attributes of a surface the caller already holds a reference to: no lookup, no search.
     *
     * A reference the field does not have — a tile index, layer or cell it does not carry, or a cell
     * with no plate — answers NoSurface, never a default attribute set; a reference from an unbuilt
     * tile answers Unbuilt.
     */
    CKGROUNDNAV_API auto
    Get_SurfaceAttributes(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_SurfaceRef& InSurface) -> FCk_GroundNav_SurfaceAttributes;

    /**
     * The attributes of the ground under a position: the is-navigable lookup, then the attributes of
     * what it found. Same statuses as that lookup.
     */
    CKGROUNDNAV_API auto
    Get_SurfaceAttributesAt(
        const FCk_GroundNav_Field&            InField,
        const FCk_GroundNav_IsNavigableQuery& InQuery) -> FCk_GroundNav_SurfaceAttributes;

    /** N answers at once; element-for-element identical to N calls of Get_SurfaceAttributesAt. */
    CKGROUNDNAV_API auto
    Get_SurfaceAttributesAt_Batch(
        const FCk_GroundNav_Field&                      InField,
        TConstArrayView<FCk_GroundNav_IsNavigableQuery> InQueries,
        TArrayView<FCk_GroundNav_SurfaceAttributes>     OutResults) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
