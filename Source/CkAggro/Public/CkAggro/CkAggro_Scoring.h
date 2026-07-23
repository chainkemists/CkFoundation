#pragma once

#include "CoreMinimal.h"

#include "CkCore/Macros/CkMacros.h"   // NOT

// --------------------------------------------------------------------------------------------------------------------
// Pure scoring + analytic-decay math for CkAggro. Every function is a primitive-in/primitive-out free function so the
// model is trivially unit-testable (including Elapsed == 0) without any ECS/editor. The processors compose these from
// fragment/tag state; the C++ unit tests call them directly. Model of record: CkAggro/Claude.md + plan §A4.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_aggro
{
    // Perception decay accelerator. Full-rate (1.0) while perceived OR still inside the lost-sight grace window;
    // otherwise the unperceived multiplier (>= 1).
    inline auto
    Compute_PerceptionDecayMultiplier(
        bool   InIsPerceived,
        double InSecondsSinceLastPerceived,
        double InLostSightGraceSeconds,
        float  InUnperceivedMultiplier)
        -> float
    {
        const auto WithinGrace = InSecondsSinceLastPerceived <= InLostSightGraceSeconds;
        return (InIsPerceived || WithinGrace) ? 1.0f : InUnperceivedMultiplier;
    }

    // Out-of-range decay accelerator. Full-rate (1.0) while within retention; otherwise the out-of-range multiplier.
    inline auto
    Compute_RangeDecayMultiplier(
        bool  InIsWithinRetention,
        float InOutOfRangeMultiplier)
        -> float
    {
        return InIsWithinRetention ? 1.0f : InOutOfRangeMultiplier;
    }

    // Analytic decay: threat lost over Elapsed at (rate * perception * range), clamped. Elapsed == 0 is a no-op
    // (returns the clamped current threat). Never integrated per tick — the caller passes Now - LastDecayTime.
    inline auto
    Compute_DecayedThreat(
        float  InCurrentThreat,
        double InElapsedSeconds,
        double InDecayRatePerSecond,
        float  InPerceptionMultiplier,
        float  InRangeMultiplier,
        double InClampMin,
        double InClampMax)
        -> float
    {
        const auto Loss    = InDecayRatePerSecond * InPerceptionMultiplier * InRangeMultiplier * FMath::Max(InElapsedSeconds, 0.0);
        const auto Decayed = static_cast<double>(InCurrentThreat) - Loss;
        return static_cast<float>(FMath::Clamp(Decayed, InClampMin, InClampMax));
    }

    // Continuous distance falloff in [0, 1]: 1 at zero distance, 0.5 at the half-distance, → 0 far out. A larger
    // exponent approaches nearest-wins more sharply. HalfDistance <= 0 disables falloff (returns 1).
    inline auto
    Compute_DistanceFactor(
        double InDistance,
        double InHalfDistance,
        double InExponent)
        -> double
    {
        if (InHalfDistance <= 0.0)
        { return 1.0; }

        const auto Ratio = FMath::Max(InDistance, 0.0) / InHalfDistance;
        return 1.0 / (1.0 + FMath::Pow(Ratio, InExponent));
    }

    // Optional bounded "prefer whoever is in my face" multiplier: (>= 1) when enabled and inside the nearby band,
    // else 1.0.
    inline auto
    Compute_NearbyFactor(
        bool   InNearbyEnabled,
        double InDistance,
        double InNearbyDistance,
        double InNearbyMultiplier)
        -> double
    {
        const auto InBand = InNearbyEnabled && InDistance <= InNearbyDistance;
        return InBand ? InNearbyMultiplier : 1.0;
    }

    // Final raw score (no incumbent bias baked in — selection applies that).
    inline auto
    Compute_Score(
        float  InThreat,
        double InDistanceFactor,
        double InNearbyFactor,
        double InScoreMultiplier,
        double InScoreBias)
        -> double
    {
        return static_cast<double>(InThreat) * InDistanceFactor * InNearbyFactor * InScoreMultiplier + InScoreBias;
    }

    // A candidate is eligible to be(come) the active target.
    inline auto
    Is_EligibleTarget(
        bool   InCannotBecomeActive,
        bool   InPendingForget,
        bool   InTrackedIsValid,
        double InScore,
        double InMinimumScore,
        bool   InIsWithinRetention,
        double InDistance,
        double InAcquisitionDistance)
        -> bool
    {
        if (InCannotBecomeActive || InPendingForget || (NOT InTrackedIsValid))
        { return false; }

        if (InScore < InMinimumScore)
        { return false; }

        // Spatial hysteresis: already-retained targets stay eligible; new ones must be inside acquisition.
        return InIsWithinRetention || InDistance <= InAcquisitionDistance;
    }

    // Whether to switch AWAY from a valid incumbent to a challenger. All four hysteresis gates must pass. (An invalid/
    // ineligible/forgotten incumbent is handled by immediate adoption at the call site, bypassing this.)
    inline auto
    Should_SwitchTarget(
        double InChallengerScore,
        double InIncumbentScore,
        double InCurrentTargetBias,
        double InSwitchThreshold,
        double InSecondsSinceLastSwitch,
        double InSwitchCooldownSeconds,
        double InSecondsSinceActiveStart,
        double InMinimumAggroDurationSeconds)
        -> bool
    {
        const auto ScoreGate    = InChallengerScore > InIncumbentScore * InCurrentTargetBias * InSwitchThreshold;
        const auto CooldownGate = InSecondsSinceLastSwitch >= InSwitchCooldownSeconds;
        const auto DurationGate = InSecondsSinceActiveStart >= InMinimumAggroDurationSeconds;
        return ScoreGate && CooldownGate && DurationGate;
    }
}

// --------------------------------------------------------------------------------------------------------------------
