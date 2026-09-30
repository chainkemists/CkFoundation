#include "CkRotateTowards/CkRotateTowards_Kernel.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_rotate_towards_kernel
{
    auto
        DoGet_IsAxisValid(
            const FCk_RotateTowards_Axis& InAxis)
        -> bool
    {
        return FMath::IsFinite(InAxis.Get_TurnRateDegPerSec()) && InAxis.Get_TurnRateDegPerSec() >= 0.0f;
    }

    auto
        DoGet_IsRangeEnabled(
            const FCk_RotateTowards_AxisRange& InRange)
        -> bool
    {
        return InRange.Get_Enabled() == ECk_EnableDisable::Enable;
    }

    auto
        DoGet_IsRangeValid(
            const FCk_RotateTowards_AxisRange& InRange)
        -> bool
    {
        if (NOT DoGet_IsRangeEnabled(InRange))
        { return true; }

        const auto Min = InRange.Get_RangeDeg().Get_Min();
        const auto Max = InRange.Get_RangeDeg().Get_Max();
        return FMath::IsFinite(Min) && FMath::IsFinite(Max) && Min <= Max && Min >= -180.0 && Max <= 180.0;
    }

    auto
        DoClamp_Axis(
            double InDesiredDeg,
            double InRestDeg,
            const FCk_RotateTowards_AxisRange& InRange)
        -> double
    {
        if (NOT DoGet_IsRangeEnabled(InRange))
        { return InDesiredDeg; }

        return ck::rotate_towards::Clamp_AngleToRange(
            static_cast<float>(InDesiredDeg), static_cast<float>(InRestDeg), InRange.Get_RangeDeg());
    }

    auto
        DoStep_Axis(
            double InCurrentDeg,
            double InDesiredDeg,
            const FCk_RotateTowards_Axis& InAxis,
            ECk_RotateTowards_Mode InMode,
            float InDeltaSeconds)
        -> double
    {
        if (InAxis.Get_Mode() == ECk_RotateTowards_AxisMode::Locked)
        { return InCurrentDeg; }

        if (InMode == ECk_RotateTowards_Mode::Instant)
        { return InDesiredDeg; }

        return ck::rotate_towards::Step_Angle(
            static_cast<float>(InCurrentDeg), static_cast<float>(InDesiredDeg), InAxis.Get_TurnRateDegPerSec(), InDeltaSeconds);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Get_IsTunablesValid(
        const FCk_RotateTowards_Tunables& InTunables)
    -> bool
{
    return ck_rotate_towards_kernel::DoGet_IsAxisValid(InTunables.Get_Pitch())
        && ck_rotate_towards_kernel::DoGet_IsAxisValid(InTunables.Get_Yaw())
        && ck_rotate_towards_kernel::DoGet_IsAxisValid(InTunables.Get_Roll())
        && FMath::IsFinite(InTunables.Get_ReachedToleranceDeg()) && InTunables.Get_ReachedToleranceDeg() >= 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Get_HasAnyRange(
        const FCk_RotateTowards_RangeClamp& InClamp)
    -> bool
{
    return ck_rotate_towards_kernel::DoGet_IsRangeEnabled(InClamp.Get_Pitch())
        || ck_rotate_towards_kernel::DoGet_IsRangeEnabled(InClamp.Get_Yaw())
        || ck_rotate_towards_kernel::DoGet_IsRangeEnabled(InClamp.Get_Roll());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Get_IsRangeClampValid(
        const FCk_RotateTowards_RangeClamp& InClamp)
    -> bool
{
    return ck_rotate_towards_kernel::DoGet_IsRangeValid(InClamp.Get_Pitch())
        && ck_rotate_towards_kernel::DoGet_IsRangeValid(InClamp.Get_Yaw())
        && ck_rotate_towards_kernel::DoGet_IsRangeValid(InClamp.Get_Roll());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Compute_LookAtRotation(
        const FVector& InFrom,
        const FVector& InTo)
    -> TOptional<FRotator>
{
    if (InFrom.ContainsNaN() || InTo.ContainsNaN())
    { return {}; }

    const auto Delta = InTo - InFrom;
    if (Delta.Size() < kCoincidentDistanceCm)
    { return {}; }

    return Delta.Rotation();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Apply_AxisLocks(
        const FRotator& InDesired,
        const FRotator& InCurrent,
        const FCk_RotateTowards_Tunables& InTunables)
    -> FRotator
{
    const auto IsLocked = [](const FCk_RotateTowards_Axis& InAxis)
    {
        return InAxis.Get_Mode() == ECk_RotateTowards_AxisMode::Locked;
    };

    return FRotator{
        IsLocked(InTunables.Get_Pitch()) ? InCurrent.Pitch : InDesired.Pitch,
        IsLocked(InTunables.Get_Yaw()) ? InCurrent.Yaw : InDesired.Yaw,
        IsLocked(InTunables.Get_Roll()) ? InCurrent.Roll : InDesired.Roll};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Clamp_AngleToRange(
        float InDesiredDeg,
        float InRestDeg,
        const FCk_FloatRange& InRangeDeg)
    -> float
{
    const auto Delta = FMath::Clamp<double>(FMath::FindDeltaAngleDegrees(InRestDeg, InDesiredDeg), InRangeDeg.Get_Min(), InRangeDeg.Get_Max());
    return static_cast<float>(FRotator::NormalizeAxis(InRestDeg + Delta));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Apply_RangeClamp(
        const FRotator& InDesired,
        const FRotator& InRest,
        const FCk_RotateTowards_RangeClamp& InClamp)
    -> FRotator
{
    return FRotator{
        ck_rotate_towards_kernel::DoClamp_Axis(InDesired.Pitch, InRest.Pitch, InClamp.Get_Pitch()),
        ck_rotate_towards_kernel::DoClamp_Axis(InDesired.Yaw, InRest.Yaw, InClamp.Get_Yaw()),
        ck_rotate_towards_kernel::DoClamp_Axis(InDesired.Roll, InRest.Roll, InClamp.Get_Roll())};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Step_Angle(
        float InCurrentDeg,
        float InDesiredDeg,
        float InRateDegPerSec,
        float InDeltaSeconds)
    -> float
{
    if (InRateDegPerSec <= 0.0f || InDeltaSeconds <= 0.0f)
    { return InCurrentDeg; }

    return static_cast<float>(FRotator::NormalizeAxis(FMath::FixedTurn(InCurrentDeg, InDesiredDeg, InRateDegPerSec * InDeltaSeconds)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Step_Rotation(
        const FRotator& InCurrent,
        const FRotator& InDesired,
        const FCk_RotateTowards_Tunables& InTunables,
        float InDeltaSeconds)
    -> FRotator
{
    const auto Mode = InTunables.Get_Mode();
    switch (Mode)
    {
        case ECk_RotateTowards_Mode::RateLimited:
        case ECk_RotateTowards_Mode::Instant:
        {
            return FRotator{
                ck_rotate_towards_kernel::DoStep_Axis(InCurrent.Pitch, InDesired.Pitch, InTunables.Get_Pitch(), Mode, InDeltaSeconds),
                ck_rotate_towards_kernel::DoStep_Axis(InCurrent.Yaw, InDesired.Yaw, InTunables.Get_Yaw(), Mode, InDeltaSeconds),
                ck_rotate_towards_kernel::DoStep_Axis(InCurrent.Roll, InDesired.Roll, InTunables.Get_Roll(), Mode, InDeltaSeconds)};
        }
        default:
        {
            CK_INVALID_ENUM(Mode);
            return InCurrent;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Compute_RemainingDelta(
        const FRotator& InCurrent,
        const FRotator& InDesired)
    -> FRotator
{
    return FRotator{
        FMath::FindDeltaAngleDegrees(InCurrent.Pitch, InDesired.Pitch),
        FMath::FindDeltaAngleDegrees(InCurrent.Yaw, InDesired.Yaw),
        FMath::FindDeltaAngleDegrees(InCurrent.Roll, InDesired.Roll)};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Get_IsAtTarget(
        const FRotator& InCurrent,
        const FRotator& InDesired,
        float InToleranceDeg)
    -> bool
{
    const auto Remaining = Compute_RemainingDelta(InCurrent, InDesired);
    return FMath::Abs(Remaining.Pitch) <= InToleranceDeg
        && FMath::Abs(Remaining.Yaw) <= InToleranceDeg
        && FMath::Abs(Remaining.Roll) <= InToleranceDeg;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::rotate_towards::Compose_OffsetRotation(
        const FQuat& InCurrentOffset,
        const FQuat& InCurrentWorld,
        const FQuat& InNewWorld)
    -> FQuat
{
    return (InCurrentOffset * (InCurrentWorld.Inverse() * InNewWorld)).GetNormalized();
}

// --------------------------------------------------------------------------------------------------------------------
