#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_accel_clamp_algorithm
{
    // Pure projection shared by candidate scoring and the processor that emits the selected
    // command. Keeping one implementation prevents the sampler from ranking a velocity that the
    // downstream actuator cannot produce this frame.
    inline auto ProjectVelocity(
        const FVector& InLastVelocity,
        const FVector& InRequestedVelocity,
        const float InMaxSpeed,
        const float InMaxAcceleration,
        const float InMaxTurnRate,
        const float InDeltaSeconds) -> FVector
    {
        const auto InputsAreValid =
            NOT InLastVelocity.ContainsNaN() &&
            NOT InRequestedVelocity.ContainsNaN() &&
            FMath::IsFinite(InMaxSpeed) &&
            InMaxSpeed >= 0.0f &&
            FMath::IsFinite(InMaxAcceleration) &&
            InMaxAcceleration >= 0.0f &&
            FMath::IsFinite(InMaxTurnRate) &&
            InMaxTurnRate >= 0.0f &&
            FMath::IsFinite(InDeltaSeconds) &&
            InDeltaSeconds >= 0.0f;
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd acceleration projection inputs "
                 "(last [{}], requested [{}], max_speed [{}], max_acceleration [{}], "
                 "max_turn_rate [{}], delta_seconds [{}])"),
            InLastVelocity,
            InRequestedVelocity,
            InMaxSpeed,
            InMaxAcceleration,
            InMaxTurnRate,
            InDeltaSeconds)
        {}
        if (NOT InputsAreValid)
        { return FVector::ZeroVector; }

        const auto RequestedVelocity = InRequestedVelocity.GetClampedToMaxSize(InMaxSpeed);
        const auto LastSpeed = static_cast<float>(InLastVelocity.Size());
        const auto RequestedSpeed = static_cast<float>(RequestedVelocity.Size());

        // Speed clamp: change at most MaxAcceleration * DeltaSeconds cm/s per frame.
        const auto MaxSpeedDelta = InMaxAcceleration * InDeltaSeconds;
        const auto TargetSpeed = FMath::Clamp(
            RequestedSpeed,
            FMath::Max(0.0f, LastSpeed - MaxSpeedDelta),
            LastSpeed + MaxSpeedDelta);

        // Compute RequestedDirection first so LastDirection starts target-aligned when the
        // previous speed is zero. This avoids a first-frame rotate-from-world-forward artifact.
        const auto RequestedDirection = RequestedSpeed > KINDA_SMALL_NUMBER
            ? RequestedVelocity / RequestedSpeed
            : (LastSpeed > KINDA_SMALL_NUMBER
                ? InLastVelocity / LastSpeed
                : FVector::ForwardVector);
        const auto LastDirection = LastSpeed > KINDA_SMALL_NUMBER
            ? InLastVelocity / LastSpeed
            : RequestedDirection;

        // Direction clamp: change at most MaxTurnRate * DeltaSeconds radians per frame.
        const auto MaxAngleDelta = InMaxTurnRate * InDeltaSeconds;
        const auto CosAngle = static_cast<float>(
            FVector::DotProduct(LastDirection, RequestedDirection));
        const auto Angle = FMath::Acos(FMath::Clamp(CosAngle, -1.0f, 1.0f));

        auto TargetDirection = FVector::ZeroVector;
        if (Angle <= MaxAngleDelta || Angle < KINDA_SMALL_NUMBER)
        {
            TargetDirection = RequestedDirection;
        }
        else
        {
            const auto T = MaxAngleDelta / Angle;
            const auto SinAngle = FMath::Sin(Angle);
            if (SinAngle > KINDA_SMALL_NUMBER)
            {
                TargetDirection =
                    (LastDirection * FMath::Sin((1.0f - T) * Angle) +
                     RequestedDirection * FMath::Sin(T * Angle)) /
                    SinAngle;
                TargetDirection.Normalize();
            }
            else
            {
                TargetDirection = RequestedDirection;
            }
        }

        return TargetDirection * TargetSpeed;
    }
}

// --------------------------------------------------------------------------------------------------------------------
