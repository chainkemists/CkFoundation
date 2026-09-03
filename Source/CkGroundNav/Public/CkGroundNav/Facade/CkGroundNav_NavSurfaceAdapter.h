#pragma once

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
}

// --------------------------------------------------------------------------------------------------------------------
