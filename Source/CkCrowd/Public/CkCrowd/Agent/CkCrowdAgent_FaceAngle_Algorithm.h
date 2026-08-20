#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_face_angle_algorithm
{
    // A bare threshold is a cliff an oscillating speed toggles across frame to frame, which would
    // reintroduce the churn the floor exists to remove; the two edges are therefore different speeds.
    constexpr auto FacingEngageHysteresisMultiplier = 2.0f;

    // 15 degrees, below the avoidance sampler's 22.5 degree minimum winner-flip quantum, so a genuine
    // gradual turn tracks immediately and only a sampler-sized jump has to prove itself.
    constexpr auto TargetTrackToleranceRad = 0.261799f;

    // A sampler winner flip lasts 1-2 frames; 3 costs a genuine corner ~50ms of commit latency.
    constexpr auto TargetPersistFrames = 3;

    inline auto IsValidSpeedFloor(const float InSpeedFloorCm) -> bool
    {
        return FMath::IsFinite(InSpeedFloorCm) && InSpeedFloorCm >= 0.0f;
    }

    // Facing tracks the desired-velocity heading only while the agent is genuinely moving. That
    // heading legally slews at the full turn rate at any speed and flips with the avoidance sampler's
    // per-frame winner, so an agent pressing at a few cm/s must not transcribe it.
    inline auto Update_Engaged(
        bool& InOutEngaged,
        int32& InOutPendingFrames,
        const float InSpeed,
        const float InSpeedFloorCm) -> bool
    {
        const auto InputsAreValid = FMath::IsFinite(InSpeed) && IsValidSpeedFloor(InSpeedFloorCm);
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd facing engage inputs (speed [{}], speed_floor [{}])"),
            InSpeed,
            InSpeedFloorCm)
        {
            InOutEngaged = false;
            InOutPendingFrames = 0;
            return false;
        }

        // A zero floor disables the gate, but a zero-length heading vector still has no direction to
        // read, so both edges stay above the normalization epsilon.
        const auto HoldSpeed = FMath::Max(InSpeedFloorCm, KINDA_SMALL_NUMBER);
        const auto EngageSpeed = FMath::Max(
            InSpeedFloorCm * FacingEngageHysteresisMultiplier,
            KINDA_SMALL_NUMBER);

        const auto WasEngaged = InOutEngaged;
        InOutEngaged = WasEngaged
            ? InSpeed >= HoldSpeed
            : InSpeed >= EngageSpeed;

        // EITHER edge starts the candidate chain over. A held facing has to hold whole, and a resumed
        // one must not inherit a stale candidate that a single fresh frame could then commit.
        if (InOutEngaged != WasEngaged)
        { InOutPendingFrames = 0; }

        return InOutEngaged;
    }

    // Seeds the committed target from the body's CURRENT orientation on the disengaged->engaged
    // transition. Without it the committed target is stale — 0 at first engage, or the heading of a
    // journey that ended pointing somewhere else — and the body would slew the wrong way for the whole
    // persistence window before the fresh heading commits: a visible twitch at every move start.
    // This is not a bypass of the flip filter: the seed is the body's own stable rotation, never the
    // noisy desired-velocity heading, so facing simply resumes from where the agent already points and
    // the filter still decides when it may start turning away from there. The caller has just come
    // through Update_Engaged, which zeroed the pending bucket on the transition.
    inline auto Seed_CommittedFromBody(
        float& InOutCommittedRad,
        const float InBodyRad) -> void
    {
        const auto InputIsValid = FMath::IsFinite(InBodyRad);
        CK_ENSURE_IF_NOT(
            InputIsValid,
            TEXT("Invalid crowd facing body angle [{}]"),
            InBodyRad)
        { return; }

        InOutCommittedRad = InBodyRad;
    }

    // Returns the committed facing target. A large heading change is held as a CANDIDATE until it
    // repeats: committing to each per-frame sampler flip is what makes a pressing agent whip. A
    // small change is a genuine gradual turn and is accepted the frame it appears — UNLESS it is
    // smaller than the dead-band, in which case the target does not move at all: sub-band chatter
    // (separation/blend wobble under neighbour pressure, well below the sampler's 22.5-degree flip
    // quantum) would otherwise be transcribed to the body at the full turn rate every frame, which
    // is the residual micro-jitter a moving crowd shows once the big flips are already filtered. A
    // real slow arc accumulates past the band within a few frames and then tracks normally, so the
    // body trails the true heading by at most the band.
    inline auto Commit_TrackedYaw(
        const float InCommittedYawRad,
        const float InCandidateYawRad,
        const float InDeadBandRad,
        float& InOutPendingYawRad,
        int32& InOutPendingFrames) -> float
    {
        const auto InputsAreValid =
            FMath::IsFinite(InCommittedYawRad) &&
            FMath::IsFinite(InCandidateYawRad) &&
            FMath::IsFinite(InDeadBandRad) && InDeadBandRad >= 0.0f;
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd facing target yaw (committed [{}], candidate [{}], dead_band [{}])"),
            InCommittedYawRad,
            InCandidateYawRad,
            InDeadBandRad)
        {
            InOutPendingFrames = 0;
            return InCommittedYawRad;
        }

        const auto DeltaFromCommitted =
            FMath::Abs(FMath::FindDeltaAngleRadians(InCommittedYawRad, InCandidateYawRad));

        if (DeltaFromCommitted <= InDeadBandRad)
        {
            InOutPendingFrames = 0;
            return InCommittedYawRad;
        }

        if (DeltaFromCommitted <= TargetTrackToleranceRad)
        {
            InOutPendingFrames = 0;
            return InCandidateYawRad;
        }

        const auto TracksPending = InOutPendingFrames > 0 &&
            FMath::Abs(FMath::FindDeltaAngleRadians(InOutPendingYawRad, InCandidateYawRad)) <=
            TargetTrackToleranceRad;

        // A heading matching neither the committed target nor the standing candidate restarts the
        // bucket; the anchor stays put while it fills so the candidate cannot drift a turn's worth of
        // arc across the persistence window.
        if (TracksPending)
        {
            InOutPendingFrames += 1;
        }
        else
        {
            InOutPendingYawRad = InCandidateYawRad;
            InOutPendingFrames = 1;
        }

        if (InOutPendingFrames < TargetPersistFrames)
        { return InCommittedYawRad; }

        InOutPendingFrames = 0;
        return InCandidateYawRad;
    }
}

// --------------------------------------------------------------------------------------------------------------------
