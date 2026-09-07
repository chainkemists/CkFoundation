#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------
// A neutral query filter, compiled into the two things a grounded query understands: what one plate
// costs, and which plates it may not enter.
//
// Recast answers an area tag with a UNavArea and lets the engine filter carry the exclusions; a field
// has no such class, so the tag has to be resolved against the plates THEMSELVES - each of which
// carries the interned area-tag container its markup stamped. The translation is therefore a pass over
// the field, and the cache below exists to not repeat it.
//
// Shared rather than private to the neutral facade because BOTH ways into a grounded search need the
// same answer: the one-shot facade query and the sliced path processor. Two copies of this compile
// would be two definitions of what a filter tag means on the same ground.
// --------------------------------------------------------------------------------------------------------------------

struct FCk_Nav_QueryFilterOverlay;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    struct FCk_GroundNav_CompiledFilterTables
    {
        // Flat plate id to what crossing it costs under this filter. A plate absent from the table is
        // priced at whatever the field itself says, which is the unfiltered ground.
        TMap<int32, float> _Multipliers;

        // Flat plate ids the filter refuses outright: its excluded areas, and anything a required set
        // does not admit.
        TSet<int32> _Denied;
    };

    /**
     * The compiled tables for this (field snapshot, filter tag, overlay), built at most once each.
     *
     * The field is taken as the shared handle the caller already holds rather than as a reference: the
     * cache OUTLIVES individual fields, and a raw address plus an epoch cannot tell a live field from a
     * freed one whose allocation was reused - a coincidence that would hand back another field's tables
     * silently. A weak handle can: an entry whose weak pointer no longer pins is dead by construction,
     * because a new field at the same address carries a new reference controller.
     *
     * An invalid handle, a filter naming nothing and an overlay excluding nothing all answer the same
     * empty tables, which is the unfiltered field.
     *
     * GAME-THREAD STATE. The only capability contracted to run off the game thread is
     * _BoundarySegments (CkNavSurface_ProviderTable.h:32-36), and a boundary query carries no filter.
     */
    CKGROUNDNAV_API auto
    Get_CompiledFilterTables(
        const FCk_GroundNav_FieldPtr&     InField,
        const FGameplayTag&               InFilterTag,
        const FCk_Nav_QueryFilterOverlay& InOverlay) -> const FCk_GroundNav_CompiledFilterTables&;
}

// --------------------------------------------------------------------------------------------------------------------
