#pragma once

#include "CkCore/Chrono/CkChrono.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::cadence
{
    // Gates per-entity work to a configured interval instead of every tick. `InOutChrono`'s GoalValue IS the
    // interval — 0 means "every tick" (matches the house 0-is-unlimited/off convention). FCk_Chrono::Tick does NOT
    // auto-reset once Done (it clamps and stays Done), so this wraps that with the Reset a repeating cadence needs;
    // do not call FCk_Chrono::Tick directly for this purpose.
    CKCORE_API auto
    ShouldRun(
        FCk_Chrono& InOutChrono,
        const FCk_Time& InDeltaT)
        -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
