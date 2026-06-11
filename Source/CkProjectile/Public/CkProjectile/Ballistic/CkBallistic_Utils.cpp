#include "CkBallistic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ballistics_detail
{
    // Index of the component best conditioned for the 1D time-of-flight solve.
    // Prefer the displacement axis, then initial velocity, then the terminal velocity
    static auto
    Get_DominantAxisIndex(
        const FVector& InDeltaLocation,
        const FVector& InStartVelocity,
        const FVector& InTerminalWithWind) -> int32
    {
        const auto AxisSource = [&]() -> FVector
        {
            if (InDeltaLocation.SizeSquared() > UE_DOUBLE_SMALL_NUMBER)
            { return InDeltaLocation; }

            if (InStartVelocity.SizeSquared() > UE_DOUBLE_SMALL_NUMBER)
            { return InStartVelocity; }

            return InTerminalWithWind;
        }();

        const auto AbsX = FMath::Abs(AxisSource.X);
        const auto AbsY = FMath::Abs(AxisSource.Y);
        const auto AbsZ = FMath::Abs(AxisSource.Z);

        if (AbsX >= AbsY && AbsX >= AbsZ)
        { return 0; }

        if (AbsY >= AbsZ)
        { return 1; }

        return 2;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ballistics
{
    auto
        Get_PositionAtTime(
            const FCk_Ballistic_InitialConditions& InInitialConditions,
            const FCk_Ballistic_TrajectoryParams& InParams,
            FCk_Time InTimeSinceStart)
        -> FVector
    {
        const auto K = InParams.Get_DragK();
        const auto Vinf = InParams.Get_TerminalVelocityWithWind();
        const auto T = InTimeSinceStart.Get_Seconds();

        const auto Numerator = (InInitialConditions.Get_StartVelocity() + K * T * Vinf) * T;
        const auto Denominator = 1.0 + K * T;

        return InInitialConditions.Get_StartLocation() + Numerator / Denominator;
    }

    auto
        Get_VelocityAtTime(
            const FCk_Ballistic_InitialConditions& InInitialConditions,
            const FCk_Ballistic_TrajectoryParams& InParams,
            FCk_Time InTimeSinceStart)
        -> FVector
    {
        const auto K = InParams.Get_DragK();
        const auto Vinf = InParams.Get_TerminalVelocityWithWind();
        const auto T = InTimeSinceStart.Get_Seconds();

        const auto Numerator = InInitialConditions.Get_StartVelocity() + K * T * (2.0 + K * T) * Vinf;
        const auto Denominator = FMath::Square(1.0 + K * T);

        return Numerator / Denominator;
    }

    auto
        Get_TimeOfFlightTo(
            const FCk_Ballistic_InitialConditions& InInitialConditions,
            const FCk_Ballistic_TrajectoryParams& InParams,
            const FVector& InEndLocation,
            const FVector& InCurrentVelocity)
        -> FCk_Time
    {
        const auto K = InParams.Get_DragK();
        const auto Vinf = InParams.Get_TerminalVelocityWithWind();
        const auto DeltaLocation = InEndLocation - InInitialConditions.Get_StartLocation();

        const auto AxisIndex = ballistics_detail::Get_DominantAxisIndex(
            DeltaLocation, InInitialConditions.Get_StartVelocity(), Vinf);

        const auto Dp = DeltaLocation[AxisIndex];
        const auto V0 = InInitialConditions.Get_StartVelocity()[AxisIndex];
        const auto VinfPart = Vinf[AxisIndex];

        // Solving dp = (v0·t + k·Vinf·t²) / (1 + k·t)  ⇔  0 = k·Vinf·t² + (v0 − dp·k)·t − dp
        if (FMath::Abs(VinfPart) < UE_DOUBLE_SMALL_NUMBER || FMath::Abs(K) < UE_DOUBLE_SMALL_NUMBER)
        {
            const auto LinearDenominator = V0 - Dp * K;

            if (FMath::Abs(LinearDenominator) < UE_DOUBLE_SMALL_NUMBER)
            { return FCk_Time::ZeroSecond(); }

            return FCk_Time{Dp / LinearDenominator};
        }

        const auto TwoA = 2.0 * K * VinfPart;
        const auto B = V0 - Dp * K;
        const auto C = -Dp;

        const auto Discriminant = FMath::Max(B * B - 2.0 * TwoA * C, 0.0);
        const auto SqrtDiscriminant = FMath::Sqrt(Discriminant);

        const auto Root1 = (-B + SqrtDiscriminant) / TwoA;
        const auto Root2 = (-B - SqrtDiscriminant) / TwoA;

        if (Root1 < 0.0)
        { return FCk_Time{Root2}; }

        if (Root2 < 0.0)
        { return FCk_Time{Root1}; }

        // Both roots are in positive time — the trajectory crosses this axis value twice (apex
        // in-between). The current velocity tells us which side of the apex we are on
        const auto CurrentDirection = InCurrentVelocity[AxisIndex];

        if (CurrentDirection * VinfPart < 0.0)
        { return FCk_Time{FMath::Min(Root1, Root2)}; }

        return FCk_Time{FMath::Max(Root1, Root2)};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ballistic_UE::
    Get_PositionAtTime(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InTimeSinceStart)
    -> FVector
{
    return ck::ballistics::Get_PositionAtTime(InInitialConditions, InParams, InTimeSinceStart);
}

auto
    UCk_Utils_Ballistic_UE::
    Get_VelocityAtTime(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InTimeSinceStart)
    -> FVector
{
    return ck::ballistics::Get_VelocityAtTime(InInitialConditions, InParams, InTimeSinceStart);
}

auto
    UCk_Utils_Ballistic_UE::
    Get_TimeOfFlightTo(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        const FVector& InEndLocation,
        const FVector& InCurrentVelocity)
    -> FCk_Time
{
    return ck::ballistics::Get_TimeOfFlightTo(InInitialConditions, InParams, InEndLocation, InCurrentVelocity);
}

auto
    UCk_Utils_Ballistic_UE::
    Get_SampledTrajectory(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InDuration,
        FCk_Time InSampleInterval)
    -> TArray<FVector>
{
    auto Samples = TArray<FVector>{};

    CK_ENSURE_IF_NOT(InSampleInterval > FCk_Time::ZeroSecond(),
        TEXT("SampleInterval [{}] must be positive"), InSampleInterval)
    { return Samples; }

    const auto SampleCount = FMath::FloorToInt32(InDuration / InSampleInterval) + 1;
    Samples.Reserve(SampleCount + 1);

    for (auto Index = 0; Index < SampleCount; ++Index)
    {
        Samples.Emplace(ck::ballistics::Get_PositionAtTime(InInitialConditions, InParams, InSampleInterval * Index));
    }

    Samples.Emplace(ck::ballistics::Get_PositionAtTime(InInitialConditions, InParams, InDuration));

    return Samples;
}

// --------------------------------------------------------------------------------------------------------------------
