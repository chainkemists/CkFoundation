#pragma once

#include "CkGait/Bob/CkBob_Fragment_Data.h"
#include "CkGait/Gait/CkGait_Kernel.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::bob
{
    inline constexpr auto kSpringStepSeconds = 1.0f / 120.0f;
    inline constexpr auto kMaxSpringFrameSeconds = 0.1f;
    inline constexpr auto kSpringRunawayCm = 100.0f;

    struct CKGAIT_API FSpringState
    {
        float _Offset = 0.0f;    // cm, +Z
        float _Velocity = 0.0f;  // cm/s
    };

    struct CKGAIT_API FTarget
    {
        FVector _LocationCm = FVector::ZeroVector;   // rest frame
        FVector _RotationDeg = FVector::ZeroVector;  // (Roll, Pitch, Yaw)
    };

    /** All fields finite and non-negative; Roll/Pitch <= 45. */
    CKGAIT_API auto Get_IsStrideValid(const FCk_Bob_StrideParams& InStride) -> bool;

    /** All fields finite and non-negative; Spring FrequencyHz > 0, DampingRatio >= 0. */
    CKGAIT_API auto Get_IsAirValid(const FCk_Bob_AirParams& InAir) -> bool;

    /** Every tunable, not the gait: Get_IsStrideValid, Get_IsAirValid, LagRate/BreathCm/MaxOffsetCm finite and non-negative, Intensity in [0,1]. */
    CKGAIT_API auto Get_IsSpecValid(const FCk_Bob_Spec& InSpec) -> bool;

    /** Stride shapes from the gait clock, WITHOUT spring and intensity: Step = |sin phase|, Sway = sin phase. Breath is weighted by max(1 - Amount, 0): off, never inverted, once Amount passes 1. */
    CKGAIT_API auto Compute_StrideTarget(const FCk_Bob_Spec& InSpec, const gait::FClockState& InClock) -> FTarget;

    /** Airborne: clamp(-Velocity.Z * Air.LiftCmPerFallSpeed, +-Air.MaxLiftCm); Grounded: 0. */
    CKGAIT_API auto Compute_AirLiftTarget(const FCk_Bob_Spec& InSpec, const FCk_Gait_Motion& InMotion) -> float;

    /** min(InImpactSpeed * Air.LandKickPerImpactSpeed, Air.MaxLandKick), >= 0 (the caller subtracts it from the spring velocity). */
    CKGAIT_API auto Compute_LandKick(const FCk_Bob_Spec& InSpec, float InImpactSpeed) -> float;

    /** Semi-implicit Euler toward InTargetCm in fixed kSpringStepSeconds substeps over min(dt, kMaxSpringFrameSeconds); a non-finite or > kSpringRunawayCm state reseeds at rest. No-op when dt <= 0. */
    CKGAIT_API auto Step_Spring(FSpringState& InOutState, float InTargetCm, float InDeltaSeconds, const FCk_Bob_SpringResponse& InResponse) -> void;

    /** Stride target plus (0,0,InSpringOffsetCm), then both location and rotation scaled by InSpec.Intensity. */
    CKGAIT_API auto Compute_Target(const FCk_Bob_Spec& InSpec, const gait::FClockState& InClock, float InSpringOffsetCm) -> FTarget;

    /** First-order lag of both channels: current += (target - current) * (1 - exp(-InLagRate * dt)). InLagRate <= 0 or dt <= 0 returns InTarget. */
    CKGAIT_API auto Smooth(const FTarget& InCurrent, const FTarget& InTarget, float InLagRate, float InDeltaSeconds) -> FTarget;

    /** Clamps the location's magnitude to InMaxCm. */
    CKGAIT_API auto Clamp_Location(const FVector& InLocationCm, float InMaxCm) -> FVector;

    /** FTransform{FRotator{Pitch = Rot.Y, Yaw = Rot.Z, Roll = Rot.X}, Location}. */
    CKGAIT_API auto Compose_Offset(const FTarget& InTarget) -> FTransform;

    /** Node offset = InBobOffset * InRestOffset (the bob applied in the rest frame, then the rest). */
    CKGAIT_API auto Compose_NodeOffset(const FTransform& InBobOffset, const FTransform& InRestOffset) -> FTransform;
}

// --------------------------------------------------------------------------------------------------------------------
