#pragma once

#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace detail
    {
        // Declared, never defined, not constexpr: calling it poisons constant evaluation, so an invalid
        // tick rate fails to COMPILE with this name in the diagnostic. The factories below are consteval,
        // so it is never reachable at runtime.
        auto TickRate_MustBePositive() -> void;
    }

    // Compile-time tick-rate literals. Each produces an FCk_Time interval, so a processor spells its fixed
    // cadence directly in the house time type:
    //
    //     static constexpr FCk_Time TickRate = ck::Hz(4);          // 4 evaluations per second
    //     static constexpr FCk_Time TickRate = ck::Seconds(0.25);  // the same rate, interval spelling
    //
    // A non-positive rate is a compile error (poisons the consteval via TickRate_MustBePositive) — declare
    // no TickRate at all for an every-tick processor (every-tick is the default).

    consteval auto
    Seconds(
        double InSeconds)
        -> FCk_Time
    {
        if (NOT (InSeconds > 0.0))
        { detail::TickRate_MustBePositive(); }

        return FCk_Time{InSeconds};
    }

    consteval auto
    Hz(
        double InCyclesPerSecond)
        -> FCk_Time
    {
        if (NOT (InCyclesPerSecond > 0.0))
        { detail::TickRate_MustBePositive(); }

        return FCk_Time{1.0 / InCyclesPerSecond};
    }
}

// --------------------------------------------------------------------------------------------------------------------
