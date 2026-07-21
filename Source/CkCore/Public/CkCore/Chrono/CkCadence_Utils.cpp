#include "CkCadence_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::cadence::
    ShouldRun(
        FCk_Chrono& InOutChrono,
        const FCk_Time& InDeltaT)
    -> bool
{
    if (InOutChrono.Get_GoalValue() <= FCk_Time::ZeroSecond())
    { return true; }

    if (InOutChrono.Tick(InDeltaT) != ECk_Chrono_TickState::Done)
    { return false; }

    InOutChrono.Reset();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
