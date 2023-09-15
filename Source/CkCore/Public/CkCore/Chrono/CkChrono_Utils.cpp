#include "CkChrono_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Chrono_UE::
    Break_Chrono(
        const FCk_Chrono& InChrono,
        FCk_Time&         OutGoal,
        FCk_Time&         OutTimeElapsed,
        FCk_Time&         OutTimeRemaining)
    -> void
{
    OutGoal          = InChrono.Get_GoalValue();
    OutTimeElapsed   = InChrono.Get_TimeElapsed();
    OutTimeRemaining = InChrono.Get_TimeRemaining();
}

auto
    UCk_Utils_Chrono_UE::
    Get_IsDone(
        const FCk_Chrono& InChrono)
    -> bool
{
    return InChrono.Get_IsDone();
}

auto
    UCk_Utils_Chrono_UE::
    Get_TimeRemaining(
        const FCk_Chrono&       InChrono,
        ECk_NormalizationPolicy InNormalization)
    -> FCk_Time
{
    return InChrono.Get_TimeRemaining(InNormalization);
}

auto
    UCk_Utils_Chrono_UE::
    Get_TimeElapsed(
        const FCk_Chrono&       InChrono,
        ECk_NormalizationPolicy InNormalization)
    -> FCk_Time
{
    return InChrono.Get_TimeElapsed(InNormalization);
}

auto
    UCk_Utils_Chrono_UE::
    Tick_Chrono(
        FCk_Chrono&     InChrono,
        const FCk_Time& InDeltaT)
    -> ECk_Chrono_TickState
{
    return InChrono.Tick(InDeltaT);
}

auto
    UCk_Utils_Chrono_UE::
    Consume_Chrono(
        FCk_Chrono&     InChrono,
        const FCk_Time& InDeltaT)
    -> ECk_Chrono_ConsumeState
{
    return InChrono.Consume(InDeltaT);
}

auto
    UCk_Utils_Chrono_UE::
    Reset_Chrono(
        FCk_Chrono& InChrono)
    -> void
{
    InChrono.Reset();
}

void
    UCk_Utils_Chrono_UE::
    Complete_Chrono(
        FCk_Chrono& InChrono)
{
    InChrono.Complete();
}

// --------------------------------------------------------------------------------------------------------------------
