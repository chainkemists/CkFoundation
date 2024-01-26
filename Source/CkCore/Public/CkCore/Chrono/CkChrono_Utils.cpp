#include "CkChrono_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Chrono_UE::
    Conv_ChronoToText(
        const FCk_Chrono& InHandle)
    -> FText
{
    return FText::FromString(Conv_ChronoToString(InHandle));
}

auto
    UCk_Utils_Chrono_UE::
    Conv_ChronoToString(
        const FCk_Chrono& InHandle)
    -> FString
{
    return ck::Format_UE(TEXT("{}"), InHandle);
}

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
    Tick(
        FCk_Chrono&     InChrono,
        const FCk_Time& InDeltaT)
    -> ECk_Chrono_TickState
{
    return InChrono.Tick(InDeltaT);
}

auto
    UCk_Utils_Chrono_UE::
    Consume(
        FCk_Chrono&     InChrono,
        const FCk_Time& InDeltaT)
    -> ECk_Chrono_ConsumeState
{
    return InChrono.Consume(InDeltaT);
}

auto
    UCk_Utils_Chrono_UE::
    Reset(
        FCk_Chrono& InChrono)
    -> void
{
    InChrono.Reset();
}

void
    UCk_Utils_Chrono_UE::
    Complete(
        FCk_Chrono& InChrono)
{
    InChrono.Complete();
}

// --------------------------------------------------------------------------------------------------------------------
