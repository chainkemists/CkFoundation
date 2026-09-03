#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// CkGroundNav seen through CkNavigation's provider-neutral surface.
//
// Every neutral capability is answered from a field the world-field registry resolves, so a consumer
// that speaks only the facade can be moved onto grounded navigation by naming the provider and
// nothing else. The mapping lives HERE rather than in CkNavigation: the neutral layer knows about no
// provider, and a GroundNav-shaped query struct has no business in it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::nav_surface_adapter
{
    /** Builds the capability table and registers it as the GroundNav provider. Called at module startup. */
    CKGROUNDNAV_API auto
    Register() -> void;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Whether the ground a record covers has been published under a field that already knows about it.
     *
     * DERIVED AT THE READ, and nothing anywhere stores it. Every tile the record's world bounds meet
     * must be Built and must carry an epoch STRICTLY PAST the one the record was submitted against —
     * every one, because a record that reaches two tiles is only as live as its laggard, and a caller
     * told otherwise would act on ground the paint has not reached yet. A record whose bounds meet no
     * tile at all is NOT live: there is no ground for it to be live on, and answering true would make
     * "live" mean "nothing contradicted it".
     *
     * Strictly past, not at or past, because _RequestedAtEpoch is stamped at admission with the epoch
     * the field was ALREADY published at. An equal epoch is therefore the very publish the record was
     * submitted against, which by construction knew nothing about it — reading that as live puts every
     * tile already built at admission live the instant the paint drains, before anything republished.
     *
     * Exposed beside the provider entry because it is the whole rule, and because a field and a record
     * are all it needs — which is what lets it be verified without a world.
     */
    CKGROUNDNAV_API auto
    Get_IsMarkupLive(
        const FCk_GroundNav_Field&        InField,
        const FCk_GroundNav_MarkupRecord& InRecord) -> bool;

    /**
     * The same rule reached through the markup ENTITY, which is the identity the neutral seam names.
     *
     * False for an entity carrying no back-pointer — the paint has not drained onto a volume yet — for
     * a record the named volume no longer holds, and for a volume with nothing published.
     */
    CKGROUNDNAV_API auto
    Get_IsMarkupLive(
        const FCk_Handle& InMarkupEntity) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
