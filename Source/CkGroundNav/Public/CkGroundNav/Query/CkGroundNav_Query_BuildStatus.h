#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldBuild.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// What a consumer asks before it asks anything else: is there a field here, and can I trust it yet.
//
// A published field is immutable, so over one of these the answer is never Building — the tiles it
// carries are built or they are not. Building is the volume's word, said while a build is in flight,
// and the volume layer above this composes it from the two.
//
// THREAD CONTRACT: the field-only functions are pure over the field handed in and callable from any
// thread by anybody holding it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /** Built, Unbuilt or OutsideField for the tile under a point. Never PartiallyBuilt or Building. */
    CKGROUNDNAV_API auto
    Get_RegionStatusAt(
        const FCk_GroundNav_Field& InField,
        const FVector&             InLocation) -> ECk_GroundNav_RegionStatus;

    /**
     * The status of every tile a box touches, folded: Built when all are, Unbuilt when none are,
     * PartiallyBuilt otherwise, OutsideField when the box reaches no tile. Only XY decides which tiles
     * are touched — the field's vertical slab is one for every tile.
     */
    CKGROUNDNAV_API auto
    Get_RegionStatusWithin(
        const FCk_GroundNav_Field& InField,
        const FBox&                InBounds) -> ECk_GroundNav_RegionStatus;

    /**
     * The world box the BUILT tiles cover, in XY, over the field's vertical slab. An empty (ForceInit)
     * box when nothing is built: a consumer fitting a view to it must be told there is nothing to fit
     * rather than handed the volume's authored bounds as if they held ground.
     */
    CKGROUNDNAV_API auto
    Get_SurfaceBounds(
        const FCk_GroundNav_Field& InField) -> FBox;

    /**
     * Provider health from what a volume holds: its published field (may be null), the outcome of
     * its last build, and whether one is running now.
     *
     *   Building — a build is in flight, whatever is published meanwhile.
     *   Ready    — a field is published and nothing is in flight.
     *   Error    — nothing is published and the last build failed.
     *   NoData   — nothing is published and nothing has been tried.
     */
    CKGROUNDNAV_API auto
    Get_ProviderHealth(
        const FCk_GroundNav_FieldPtr& InPublished,
        ECk_GroundNav_BuildStatus     InLastBuildStatus,
        bool                          InIsBuildInFlight) -> ECk_NavSurface_ProviderHealth;

    /**
     * A region status seen through a volume: the published field's answer, with Unbuilt promoted to
     * Building while a build is in flight, and OutsideField / Unbuilt when nothing is published yet
     * depending on whether the volume's own bounds cover the point.
     */
    CKGROUNDNAV_API auto
    Get_RegionStatusAt_ForVolume(
        const FCk_GroundNav_FieldPtr& InPublished,
        const FBox&                   InVolumeBounds,
        bool                          InIsBuildInFlight,
        const FVector&                InLocation) -> ECk_GroundNav_RegionStatus;

    CKGROUNDNAV_API auto
    Get_RegionStatusWithin_ForVolume(
        const FCk_GroundNav_FieldPtr& InPublished,
        const FBox&                   InVolumeBounds,
        bool                          InIsBuildInFlight,
        const FBox&                   InBounds) -> ECk_GroundNav_RegionStatus;
}

// --------------------------------------------------------------------------------------------------------------------
