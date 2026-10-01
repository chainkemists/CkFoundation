#include "CkGait/Gait/CkGait_Kernel.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_gait_kernel
{
    auto
        DoGet_IsFiniteAtLeast(
            float InValue,
            float InMin)
        -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= InMin;
    }

    auto
        DoGet_IsFiniteInUnitRange(
            float InValue)
        -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= 0.0f && InValue <= 1.0f;
    }

    // 1 - exp(-rate * dt): the fraction of the remaining distance a first-order follower covers this step.
    auto
        DoGet_FollowAlpha(
            float InRate,
            float InDeltaSeconds)
        -> float
    {
        return 1.0f - FMath::Exp(-InRate * InDeltaSeconds);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Get_IsStrideValid(
        const FCk_Gait_StrideParams& InStride)
    -> bool
{
    return FMath::IsFinite(InStride.Get_ReferenceSpeed()) && InStride.Get_ReferenceSpeed() > 0.0f
        && ck_gait_kernel::DoGet_IsFiniteAtLeast(InStride.Get_StridesPerSecond(), 0.0f)
        && ck_gait_kernel::DoGet_IsFiniteAtLeast(InStride.Get_MaxAmountScale(), 0.0f)
        && ck_gait_kernel::DoGet_IsFiniteAtLeast(InStride.Get_AmountInterpSpeed(), 0.0f)
        && ck_gait_kernel::DoGet_IsFiniteInUnitRange(InStride.Get_CrouchScale())
        && ck_gait_kernel::DoGet_IsFiniteInUnitRange(InStride.Get_MinCadenceScale());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Get_AreTunablesValid(
        const FCk_Gait_Spec& InSpec)
    -> bool
{
    return Get_IsStrideValid(InSpec.Get_Stride())
        && FMath::IsFinite(InSpec.Get_BreathPeriodSeconds()) && InSpec.Get_BreathPeriodSeconds() > 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Get_IsSpecValid(
        const FCk_Gait_Spec& InSpec)
    -> bool
{
    return Get_AreTunablesValid(InSpec) && InSpec.Get_MovementComponent().IsValid();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Get_RestMotion()
    -> FCk_Gait_Motion
{
    return FCk_Gait_Motion{FVector::ZeroVector, ECk_Gait_Footing::Grounded, ECk_Gait_Stance::Standing};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Compute_GroundSpeed(
        const FCk_Gait_Motion& InMotion)
    -> float
{
    if (InMotion.Get_Footing() == ECk_Gait_Footing::Airborne)
    { return 0.0f; }

    return static_cast<float>(InMotion.Get_Velocity().Size2D());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Wrap_Phase(
        float InPhase)
    -> float
{
    // A non-finite phase cannot be wrapped; restart the cycle rather than poisoning every consumer.
    if (NOT FMath::IsFinite(InPhase))
    { return 0.0f; }

    auto Wrapped = FMath::Fmod(InPhase, kTwoPi);
    if (Wrapped < 0.0f)
    { Wrapped += kTwoPi; }

    // -epsilon + 2pi rounds to exactly 2pi in float; keep the range half-open.
    if (Wrapped >= kTwoPi)
    { Wrapped = 0.0f; }

    return Wrapped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Step_Clock(
        FClockState& InOutState,
        const FCk_Gait_Spec& InSpec,
        const FCk_Gait_Motion& InMotion,
        float InDeltaSeconds)
    -> void
{
    if (NOT FMath::IsFinite(InDeltaSeconds) || InDeltaSeconds <= 0.0f)
    { return; }

    const auto& Stride = InSpec.Get_Stride();
    const auto SpeedRatio = Compute_GroundSpeed(InMotion) / Stride.Get_ReferenceSpeed();
    const auto StanceScale = InMotion.Get_Stance() == ECk_Gait_Stance::Crouched ? Stride.Get_CrouchScale() : 1.0f;
    const auto TargetAmount = FMath::Min(SpeedRatio, Stride.Get_MaxAmountScale()) * StanceScale;

    InOutState._SpeedRatio = SpeedRatio;
    InOutState._Amount += (TargetAmount - InOutState._Amount) * ck_gait_kernel::DoGet_FollowAlpha(Stride.Get_AmountInterpSpeed(), InDeltaSeconds);

    const auto Cadence = FMath::Max(SpeedRatio, Stride.Get_MinCadenceScale());
    InOutState._Phase = Wrap_Phase(InOutState._Phase + kTwoPi * Stride.Get_StridesPerSecond() * Cadence * InDeltaSeconds);
    InOutState._BreathPhase = Wrap_Phase(InOutState._BreathPhase + kTwoPi * InDeltaSeconds / InSpec.Get_BreathPeriodSeconds());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::gait::Detect_Landing(
        const FCk_Gait_Motion& InPrev,
        const FCk_Gait_Motion& InCurr)
    -> TOptional<float>
{
    if (InPrev.Get_Footing() != ECk_Gait_Footing::Airborne || InCurr.Get_Footing() != ECk_Gait_Footing::Grounded)
    { return {}; }

    return FMath::Max(static_cast<float>(-InPrev.Get_Velocity().Z), 0.0f);
}

// --------------------------------------------------------------------------------------------------------------------
