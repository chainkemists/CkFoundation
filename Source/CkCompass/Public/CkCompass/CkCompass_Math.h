#pragma once

#include "CkCore/Enums/CkEnums.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Pure compass math — no entities, no world. Unit-tested in isolation (Tests/Test_CkCompass_Math.cpp).
//
// Convention (documented contract): heading/bearing are world yaws in degrees where 0 = North = +X and
// 90 = East = +Y (UE's yaw growth direction). All signed deltas are shortest-path, in [-180, 180].

namespace ck::compass
{
    // World yaw (degrees) of the direction from InObserverLocation to InTargetLocation, Z ignored.
    CKCOMPASS_API auto
    Get_WorldYawTo(
        const FVector& InObserverLocation,
        const FVector& InTargetLocation) -> float;

    // Signed shortest-path delta (degrees, [-180, 180]) from InHeadingDegrees to the observer->target direction.
    // Negative = target is to the left of the heading, positive = to the right.
    CKCOMPASS_API auto
    Get_SignedBearingDegrees(
        const FVector& InObserverLocation,
        const FVector& InTargetLocation,
        float InHeadingDegrees) -> float;

    // Maps a signed bearing onto a compass arc: result is bearing / (arc/2), clamped to [-1, 1].
    // |unclamped| > 1 means the bearing lies outside the visible arc (the caller decides clamp-vs-hide).
    CKCOMPASS_API auto
    Get_NormalizedArcOffset(
        float InSignedBearingDegrees,
        float InArcDegrees) -> float;

    // True when the signed bearing lies outside the visible arc (|bearing| > arc/2).
    CKCOMPASS_API auto
    Get_IsOutsideArc(
        float InSignedBearingDegrees,
        float InArcDegrees) -> bool;

    // 8-way cardinal/ordinal for an absolute heading (degrees, any range — normalized internally).
    // Buckets are 45 degrees wide, centered on each direction (North covers [-22.5, 22.5)).
    CKCOMPASS_API auto
    Get_CardinalAndOrdinalDirection(
        float InHeadingDegrees) -> ECk_CardinalAndOrdinalDirection;
}

// --------------------------------------------------------------------------------------------------------------------