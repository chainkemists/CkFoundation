#include "CkSpring_Utils.h"

#include <Engine/SpringInterpolator.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spring_Vec3_UE::
    Get_IsInMotion(
        const FVectorSpringState& InSpringState)
    -> bool
{
    return NOT InSpringState.Velocity.IsNearlyZero(RK4_SPRING_INTERPOLATOR_VELOCITY_TOLERANCE);
}

auto
    UCk_Utils_Spring_Vec3_UE::
    Break_SpringState(
        const FVectorSpringState& InSpringState,
        FVector& OutPrevTarget,
        FVector& OutVelocity,
        ECk_ValidInvalid& OutPrevTargetValidity)
    -> void
{
    OutPrevTarget = InSpringState.PrevTarget;
    OutVelocity = InSpringState.Velocity;
    OutPrevTargetValidity = InSpringState.bPrevTargetValid ? ECk_ValidInvalid::Valid : ECk_ValidInvalid::Invalid;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spring_Float_UE::
    Break_SpringState(
        const FFloatSpringState& InSpringState,
        float& OutPrevTarget,
        float& OutVelocity,
        ECk_ValidInvalid& OutPrevTargetValidity)
    -> void
{
    OutPrevTarget = InSpringState.PrevTarget;
    OutVelocity = InSpringState.Velocity;
    OutPrevTargetValidity = InSpringState.bPrevTargetValid ? ECk_ValidInvalid::Valid : ECk_ValidInvalid::Invalid;
}

auto
    UCk_Utils_Spring_Float_UE::
    Get_IsInMotion(
        const FFloatSpringState& InSpringState)
    -> bool
{
    return NOT FMath::IsNearlyZero(InSpringState.Velocity, RK4_SPRING_INTERPOLATOR_VELOCITY_TOLERANCE);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spring_Quat_UE::
    Break_SpringState(
        const FQuaternionSpringState& InSpringState,
        FQuat& OutPrevTarget,
        FVector& OutAngularVelocity,
        ECk_ValidInvalid& OutPrevTargetValidity)
    -> void
{
    OutPrevTarget = InSpringState.PrevTarget;
    OutAngularVelocity = InSpringState.AngularVelocity;
    OutPrevTargetValidity = InSpringState.bPrevTargetValid ? ECk_ValidInvalid::Valid : ECk_ValidInvalid::Invalid;
}

auto
    UCk_Utils_Spring_Quat_UE::
    Get_IsInMotion(
        const FQuaternionSpringState& InSpringState)
    -> bool
{
    return NOT InSpringState.AngularVelocity.IsNearlyZero(RK4_SPRING_INTERPOLATOR_VELOCITY_TOLERANCE);
}

// --------------------------------------------------------------------------------------------------------------------
