#pragma once

#include "CkRotateTowards/CkRotateTowards_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::rotate_towards
{
    inline constexpr auto kCoincidentDistanceCm = 0.01f;
    inline constexpr auto kApplyEpsilonDeg = 1.0e-3f;

    /** All fields finite; every _TurnRateDegPerSec >= 0; _ReachedToleranceDeg >= 0. */
    CKROTATETOWARDS_API auto Get_IsTunablesValid(const FCk_RotateTowards_Tunables& InTunables) -> bool;

    /** True when at least one axis range is Enable. */
    CKROTATETOWARDS_API auto Get_HasAnyRange(const FCk_RotateTowards_RangeClamp& InClamp) -> bool;

    /** Every ENABLED range: finite, Min <= Max, Min >= -180, Max <= 180. Disabled ranges are not inspected. Handle validity is NOT checked here. */
    CKROTATETOWARDS_API auto Get_IsRangeClampValid(const FCk_RotateTowards_RangeClamp& InClamp) -> bool;

    /** (InTo - InFrom).Rotation(). Unset when either vector is non-finite or the points are closer than kCoincidentDistanceCm. */
    CKROTATETOWARDS_API auto Compute_LookAtRotation(const FVector& InFrom, const FVector& InTo) -> TOptional<FRotator>;

    /** A Locked axis takes InCurrent's value; a Free axis keeps InDesired's. */
    CKROTATETOWARDS_API auto Apply_AxisLocks(const FRotator& InDesired, const FRotator& InCurrent, const FCk_RotateTowards_Tunables& InTunables) -> FRotator;

    /** Delta = FindDeltaAngleDegrees(InRestDeg, InDesiredDeg) clamped to [Min, Max]; returns NormalizeAxis(InRestDeg + Delta). */
    CKROTATETOWARDS_API auto Clamp_AngleToRange(float InDesiredDeg, float InRestDeg, const FCk_FloatRange& InRangeDeg) -> float;

    /** Clamp_AngleToRange per ENABLED axis; disabled axes pass through. */
    CKROTATETOWARDS_API auto Apply_RangeClamp(const FRotator& InDesired, const FRotator& InRest, const FCk_RotateTowards_RangeClamp& InClamp) -> FRotator;

    /** InRateDegPerSec <= 0 or InDeltaSeconds <= 0 → InCurrentDeg. Otherwise NormalizeAxis(FMath::FixedTurn(InCurrentDeg, InDesiredDeg, InRateDegPerSec * InDeltaSeconds)). */
    CKROTATETOWARDS_API auto Step_Angle(float InCurrentDeg, float InDesiredDeg, float InRateDegPerSec, float InDeltaSeconds) -> float;

    /** Instant → InDesired on Free axes; RateLimited → Step_Angle per Free axis with that axis's rate. Locked axes hold InCurrent in both modes. */
    CKROTATETOWARDS_API auto Step_Rotation(const FRotator& InCurrent, const FRotator& InDesired, const FCk_RotateTowards_Tunables& InTunables, float InDeltaSeconds) -> FRotator;

    /** Per-axis FindDeltaAngleDegrees(InCurrent.Axis, InDesired.Axis) as an FRotator (Pitch, Yaw, Roll). */
    CKROTATETOWARDS_API auto Compute_RemainingDelta(const FRotator& InCurrent, const FRotator& InDesired) -> FRotator;

    /** Every |remaining axis| <= InToleranceDeg. */
    CKROTATETOWARDS_API auto Get_IsAtTarget(const FRotator& InCurrent, const FRotator& InDesired, float InToleranceDeg) -> bool;

    /** (InCurrentOffset * (InCurrentWorld.Inverse() * InNewWorld)).GetNormalized(): the world delta applied in the node's own frame. */
    CKROTATETOWARDS_API auto Compose_OffsetRotation(const FQuat& InCurrentOffset, const FQuat& InCurrentWorld, const FQuat& InNewWorld) -> FQuat;
}

// --------------------------------------------------------------------------------------------------------------------
