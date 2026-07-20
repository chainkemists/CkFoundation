#include "CkCompass_Math.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::compass
{
    auto
        Get_WorldYawTo(
            const FVector& InObserverLocation,
            const FVector& InTargetLocation)
        -> float
    {
        const auto Delta = InTargetLocation - InObserverLocation;
        return FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    }

    auto
        Get_SignedBearingDegrees(
            const FVector& InObserverLocation,
            const FVector& InTargetLocation,
            float InHeadingDegrees)
        -> float
    {
        const auto WorldYawToTarget = Get_WorldYawTo(InObserverLocation, InTargetLocation);
        return FRotator::NormalizeAxis(WorldYawToTarget - InHeadingDegrees);
    }

    auto
        Get_NormalizedArcOffset(
            float InSignedBearingDegrees,
            float InArcDegrees)
        -> float
    {
        const auto HalfArc = InArcDegrees * 0.5f;

        if (HalfArc <= 0.0f)
        { return 0.0f; }

        return FMath::Clamp(InSignedBearingDegrees / HalfArc, -1.0f, 1.0f);
    }

    auto
        Get_IsOutsideArc(
            float InSignedBearingDegrees,
            float InArcDegrees)
        -> bool
    {
        return FMath::Abs(InSignedBearingDegrees) > InArcDegrees * 0.5f;
    }

    auto
        Get_CardinalAndOrdinalDirection(
            float InHeadingDegrees)
        -> ECk_CardinalAndOrdinalDirection
    {
        const auto NormalizedHeading = FRotator::ClampAxis(InHeadingDegrees);
        const auto BucketIndex = FMath::FloorToInt32(FRotator::ClampAxis(NormalizedHeading + 22.5f) / 45.0f) % 8;

        return static_cast<ECk_CardinalAndOrdinalDirection>(BucketIndex);
    }
}

// --------------------------------------------------------------------------------------------------------------------