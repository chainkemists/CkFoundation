#include "CkGait/Bob/CkBob_Kernel.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_bob_kernel
{
    inline constexpr auto kMaxStrideRotationDeg = 45.0f;

    auto
        DoGet_IsFiniteAtLeast(
            float InValue,
            float InMin)
        -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= InMin;
    }

    auto
        DoGet_IsFiniteInRange(
            float InValue,
            float InMin,
            float InMax)
        -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= InMin && InValue <= InMax;
    }

    auto
        DoGet_IsSpringValid(
            const FCk_Bob_SpringResponse& InResponse)
        -> bool
    {
        return FMath::IsFinite(InResponse.Get_FrequencyHz()) && InResponse.Get_FrequencyHz() > 0.0f
            && DoGet_IsFiniteAtLeast(InResponse.Get_DampingRatio(), 0.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Get_IsStrideValid(
        const FCk_Bob_StrideParams& InStride)
    -> bool
{
    return ck_bob_kernel::DoGet_IsFiniteAtLeast(InStride.Get_VerticalCm(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InStride.Get_LateralCm(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InStride.Get_ForwardCm(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteInRange(InStride.Get_RollDeg(), 0.0f, ck_bob_kernel::kMaxStrideRotationDeg)
        && ck_bob_kernel::DoGet_IsFiniteInRange(InStride.Get_PitchDeg(), 0.0f, ck_bob_kernel::kMaxStrideRotationDeg);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Get_IsAirValid(
        const FCk_Bob_AirParams& InAir)
    -> bool
{
    return ck_bob_kernel::DoGet_IsFiniteAtLeast(InAir.Get_LiftCmPerFallSpeed(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InAir.Get_MaxLiftCm(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InAir.Get_LandKickPerImpactSpeed(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InAir.Get_MaxLandKick(), 0.0f)
        && ck_bob_kernel::DoGet_IsSpringValid(InAir.Get_Spring());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Get_IsSpecValid(
        const FCk_Bob_Spec& InSpec)
    -> bool
{
    return Get_IsStrideValid(InSpec.Get_Stride())
        && Get_IsAirValid(InSpec.Get_Air())
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InSpec.Get_LagRate(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InSpec.Get_BreathCm(), 0.0f)
        && ck_bob_kernel::DoGet_IsFiniteInRange(InSpec.Get_Intensity(), 0.0f, 1.0f)
        && ck_bob_kernel::DoGet_IsFiniteAtLeast(InSpec.Get_MaxOffsetCm(), 0.0f);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Compute_StrideTarget(
        const FCk_Bob_Spec& InSpec,
        const gait::FClockState& InClock)
    -> FTarget
{
    const auto& Stride = InSpec.Get_Stride();
    const auto A = InClock._Amount;
    const auto Sway = FMath::Sin(InClock._Phase);
    const auto Step = FMath::Abs(Sway);
    // Breath fades out as the stride fades in. Amount passes 1 above the reference speed, so the weight is clamped:
    // at a sprint the breath is off, never inverted.
    const auto Breath = InSpec.Get_BreathCm() * FMath::Sin(InClock._BreathPhase) * FMath::Max(1.0f - A, 0.0f);

    auto Target = FTarget{};
    Target._LocationCm = FVector{
        Stride.Get_ForwardCm() * A,
        Stride.Get_LateralCm() * Sway * A,
        -Stride.Get_VerticalCm() * Step * A + Breath};
    Target._RotationDeg = FVector{
        Stride.Get_RollDeg() * Sway * A,
        -Stride.Get_PitchDeg() * Step * A,
        0.0f};
    return Target;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Compute_AirLiftTarget(
        const FCk_Bob_Spec& InSpec,
        const FCk_Gait_Motion& InMotion)
    -> float
{
    if (InMotion.Get_Footing() != ECk_Gait_Footing::Airborne)
    { return 0.0f; }

    const auto& Air = InSpec.Get_Air();
    const auto MaxLift = Air.Get_MaxLiftCm();
    return FMath::Clamp(static_cast<float>(-InMotion.Get_Velocity().Z) * Air.Get_LiftCmPerFallSpeed(), -MaxLift, MaxLift);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Compute_LandKick(
        const FCk_Bob_Spec& InSpec,
        float InImpactSpeed)
    -> float
{
    const auto& Air = InSpec.Get_Air();
    return FMath::Max(FMath::Min(InImpactSpeed * Air.Get_LandKickPerImpactSpeed(), Air.Get_MaxLandKick()), 0.0f);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Step_Spring(
        FSpringState& InOutState,
        float InTargetCm,
        float InDeltaSeconds,
        const FCk_Bob_SpringResponse& InResponse)
    -> void
{
    if (NOT FMath::IsFinite(InDeltaSeconds) || InDeltaSeconds <= 0.0f)
    { return; }

    // Fixed substeps (semi-implicit Euler) so a hitch or a throttled editor frame cannot blow the spring up.
    const auto Omega = gait::kTwoPi * InResponse.Get_FrequencyHz();
    const auto DampingRatio = InResponse.Get_DampingRatio();
    auto Remaining = FMath::Min(InDeltaSeconds, kMaxSpringFrameSeconds);
    while (Remaining > 0.0f)
    {
        const auto Step = FMath::Min(Remaining, kSpringStepSeconds);
        const auto Accel = Omega * Omega * (InTargetCm - InOutState._Offset) - 2.0f * DampingRatio * Omega * InOutState._Velocity;
        InOutState._Velocity += Accel * Step;
        InOutState._Offset += InOutState._Velocity * Step;
        Remaining -= Step;
    }

    if (NOT FMath::IsFinite(InOutState._Offset) || NOT FMath::IsFinite(InOutState._Velocity)
        || FMath::Abs(InOutState._Offset) > kSpringRunawayCm)
    { InOutState = {}; }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Compute_Target(
        const FCk_Bob_Spec& InSpec,
        const gait::FClockState& InClock,
        float InSpringOffsetCm)
    -> FTarget
{
    auto Target = Compute_StrideTarget(InSpec, InClock);
    Target._LocationCm.Z += InSpringOffsetCm;

    const auto Intensity = InSpec.Get_Intensity();
    Target._LocationCm *= Intensity;
    Target._RotationDeg *= Intensity;
    return Target;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Smooth(
        const FTarget& InCurrent,
        const FTarget& InTarget,
        float InLagRate,
        float InDeltaSeconds)
    -> FTarget
{
    if (InLagRate <= 0.0f || InDeltaSeconds <= 0.0f)
    { return InTarget; }

    const auto Alpha = 1.0f - FMath::Exp(-InLagRate * InDeltaSeconds);

    auto Smoothed = FTarget{};
    Smoothed._LocationCm = InCurrent._LocationCm + (InTarget._LocationCm - InCurrent._LocationCm) * Alpha;
    Smoothed._RotationDeg = InCurrent._RotationDeg + (InTarget._RotationDeg - InCurrent._RotationDeg) * Alpha;
    return Smoothed;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Clamp_Location(
        const FVector& InLocationCm,
        float InMaxCm)
    -> FVector
{
    return InLocationCm.GetClampedToMaxSize(InMaxCm);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Compose_Offset(
        const FTarget& InTarget)
    -> FTransform
{
    return FTransform{FRotator{InTarget._RotationDeg.Y, InTarget._RotationDeg.Z, InTarget._RotationDeg.X}, InTarget._LocationCm};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::bob::Compose_NodeOffset(
        const FTransform& InBobOffset,
        const FTransform& InRestOffset)
    -> FTransform
{
    // FTransform A * B applies A first, then B: the bob is expressed in the rest frame.
    return InBobOffset * InRestOffset;
}

// --------------------------------------------------------------------------------------------------------------------
