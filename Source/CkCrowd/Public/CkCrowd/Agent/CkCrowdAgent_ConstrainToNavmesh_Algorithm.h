#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_constrain_to_navmesh_algorithm
{
    // Crowd-agent transforms use a feet anchor. Once navigation returns the surface location
    // reached by the requested XY walk, the complete offset must land those feet on that surface.
    // Passing the integrator's free-space Z through here lets turn-limited vertical momentum carry
    // an agent below the projection extent, permanently disabling the navmesh constraint.
    inline auto ResolveSurfaceOffset(
        const FVector& InFeetLocation,
        const FVector& InSurfaceLocation) -> FVector
    {
        const auto InputsAreValid =
            NOT InFeetLocation.ContainsNaN() &&
            NOT InSurfaceLocation.ContainsNaN();
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd navmesh constraint inputs (feet [{}], surface [{}])"),
            InFeetLocation,
            InSurfaceLocation)
        { return FVector::ZeroVector; }

        return InSurfaceLocation - InFeetLocation;
    }

    // The grounding lease: a zero-displacement agent still reconciles against the navmesh once
    // this many seconds have passed since its last constraint pass. An interval <= 0 disables the
    // idle verify (the pre-lease behaviour, for A/B only).
    inline auto Get_ShouldVerifyGrounding(
        float InSecondsSinceVerified,
        float InIntervalSeconds) -> bool
    {
        if (InIntervalSeconds <= 0.0f)
        { return false; }

        return InSecondsSinceVerified >= InIntervalSeconds;
    }

    // Initial lease-clock seed, spreading agents uniformly across [0, Interval) by entity hash —
    // a crowd composed on one frame must not verify on one frame forever.
    inline auto Get_GroundingVerifyPhaseSeconds(
        uint32 InAgentHash,
        float InIntervalSeconds) -> float
    {
        if (InIntervalSeconds <= 0.0f)
        { return 0.0f; }

        constexpr auto PhaseBuckets = 1024u;
        const auto Phase = static_cast<float>(InAgentHash % PhaseBuckets) / static_cast<float>(PhaseBuckets);
        return Phase * InIntervalSeconds;
    }

    // A recovery may pull an agent DOWN any distance (returning a floater to the surface, healing
    // an under-mesh excursion) but may LIFT it at most a step: navmesh higher than the agent could
    // have walked onto is an elevated island (a foliage top, a berm, stacked geometry), and
    // snapping up onto it turns the recovery into a RATCHET — each snap raises the feet, the next
    // search reaches the next-higher island, and the agent climbs into the sky. Measured live:
    // +190cm snaps stair-stepping an agent from Z=3 to Z=319 in 16 seconds.
    inline auto Get_RecoveryExceedsStepUp(
        float InOffsetZ,
        float InMaxStepUpCm) -> bool
    {
        if (InMaxStepUpCm <= 0.0f)
        { return false; }

        return InOffsetZ > InMaxStepUpCm;
    }

    // The idle-verify correction is Z-ONLY, past a dead-band. ProjectPointToNavigation returns the
    // nearest poly point, which near a navmesh edge carries a LATERAL nudge — folding that XY into
    // a resting agent's offset would shove a settled pile a little every lease, re-creating the
    // formation creep the push-apart slop exists to end.
    inline auto ResolveVerticalDriftOffset(
        const FVector& InFeetLocation,
        const FVector& InSurfaceLocation,
        float InMinCorrectionCm) -> FVector
    {
        const auto FullOffset = ResolveSurfaceOffset(InFeetLocation, InSurfaceLocation);

        if (FMath::Abs(FullOffset.Z) <= InMinCorrectionCm)
        { return FVector::ZeroVector; }

        return FVector{0.0f, 0.0f, FullOffset.Z};
    }
}

// --------------------------------------------------------------------------------------------------------------------
