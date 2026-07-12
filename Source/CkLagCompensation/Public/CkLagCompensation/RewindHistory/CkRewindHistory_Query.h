#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Fixed-capacity ring buffer of rewind frames, ordered by world time. Push overwrites the oldest
 * frame once full — record/prune are O(1), unlike a TArray with RemoveAt(0).
 * Plain (non-reflected) type so the query math is unit-testable without a world.
 */
struct CKLAGCOMPENSATION_API FCk_LagComp_FrameBuffer
{
public:
    CK_GENERATED_BODY(FCk_LagComp_FrameBuffer);

public:
    auto Init(int32 InCapacity) -> void;
    auto Push(FCk_LagComp_RewindFrame InFrame) -> void;
    auto Reset() -> void;

    auto Get_Count() const -> int32;
    auto Get_Capacity() const -> int32;

    // Chronological access: index 0 = oldest recorded frame
    auto Get_FrameAt(int32 InChronologicalIndex) const -> const FCk_LagComp_RewindFrame&;

private:
    TArray<FCk_LagComp_RewindFrame> _Frames;
    int32 _Head = 0;
    int32 _Count = 0;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::lag_comp
{
    struct FSweepResult
    {
        // Fraction [0..1] along the (unadjusted) swept segment
        double _Time01 = 0.0;
        FVector _Location = FVector::ZeroVector;
        FVector _Normal = FVector::ZeroVector;
        FGameplayTag _HitShapeLabel;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Snapshots of all hit shapes interpolated at InWorldTime, from the bracketing recorded frames.
    // Empty if the buffer cannot bracket the time (fewer than 2 frames, or the time predates the
    // oldest recorded frame by more than one frame interval). Times after the newest frame clamp
    // to the newest pose — recording lags the present by up to one record interval
    CKLAGCOMPENSATION_API auto
    Get_InterpolatedSnapshots(
        const FCk_LagComp_FrameBuffer& InFrames,
        FCk_Time InWorldTime) -> TArray<FCk_LagComp_HitShapeSnapshot>;

    // Sweep a moving point (with InSweepRadius) from InSegmentStart to InSegmentEnd against ONE
    // hit shape that itself moves from InShapeAtStart to InShapeAtEnd over the same time slice.
    // Solved in the shape's rest frame by subtracting the shape's motion from the segment —
    // pure math, no physics engine, deterministic
    CKLAGCOMPENSATION_API auto
    Sweep_SegmentVsSnapshot(
        const FVector& InSegmentStart,
        const FVector& InSegmentEnd,
        double InSweepRadius,
        const FCk_LagComp_HitShapeSnapshot& InShapeAtStart,
        const FCk_LagComp_HitShapeSnapshot& InShapeAtEnd) -> TOptional<FSweepResult>;

    // Sweep a segment travelled over [InTimeStart, InTimeEnd] against the recorded history.
    // Interpolates the target's pose at both ends of the slice, AABB pre-rejects, then runs the
    // analytic shape sweeps. Returns the earliest hit, with _TargetEntity left unset
    CKLAGCOMPENSATION_API auto
    Sweep_SegmentVsHistory(
        const FCk_LagComp_FrameBuffer& InFrames,
        const FVector& InSegmentStart,
        const FVector& InSegmentEnd,
        FCk_Time InTimeStart,
        FCk_Time InTimeEnd,
        double InSweepRadius) -> TOptional<FCk_LagComp_RewindHit>;

    // Conservative world-space AABB of a posed shape (used for frame bounds and pre-rejection)
    CKLAGCOMPENSATION_API auto
    Get_SnapshotWorldBounds(
        const FCk_LagComp_HitShapeSnapshot& InSnapshot) -> FBox;
}

// --------------------------------------------------------------------------------------------------------------------
