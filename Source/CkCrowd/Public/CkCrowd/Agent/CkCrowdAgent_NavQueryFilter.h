#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NavFilters/NavigationQueryFilter.h>

#include "CkCrowdAgent_NavQueryFilter.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The STRICT planning filter: stationary-crowd markup is impassable, not a toll. Used for the
// strict phase of two-phase planning when the agent's params carry no _NavQueryFilterStrict tag of
// their own — a host whose agents already plan with a custom filter registers a strict variant of
// THAT filter (same areas plus this exclusion) and maps it behind the strict tag, because a filter
// class cannot compose with another at query time.
UCLASS()
class CKCROWD_API UCk_NavQueryFilter_AvoidStandingCrowds : public UNavigationQueryFilter
{
    GENERATED_BODY()

public:
    UCk_NavQueryFilter_AvoidStandingCrowds();
};

// --------------------------------------------------------------------------------------------------------------------
