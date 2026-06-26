#pragma once

#include "Stats/Stats.h"

// --------------------------------------------------------------------------------------------------------------------

// Stat group for script-driven scope timings (AngelScript `FCk_ScopedStat`).
// Kept separate from STATGROUP_CkProcessors so script costs can be filtered on
// their own: `stat CkScript`, or its own track in Unreal Insights.
DECLARE_STATS_GROUP(TEXT("CkScript"), STATGROUP_CkScript, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------
