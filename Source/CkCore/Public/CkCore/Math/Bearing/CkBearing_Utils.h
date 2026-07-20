#pragma once

#include "CkCore/Enums/CkEnums.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Pure bearing math — no entities, no world. Unit-tested in isolation (CkCompass/Tests/CkBearing_Utils.spec.cpp).
//
// Convention (documented contract): heading/bearing are world yaws in degrees where 0 = North = +X and
// 90 = East = +Y (UE's yaw growth direction). All signed deltas are shortest-path, in [-180, 180].

namespace ck::bearing
{
    // World yaw (degrees) of the direction from InObserverLocation to InTargetLocation, Z ignored.
    CKCORE_API auto
    Get_WorldYawTo(
        const FVector& InObserverLocation,
        const FVector& InTargetLocation) -> float;

    // Signed shortest-path delta (degrees, [-180, 180]) from InHeadingDegrees to the observer->target direction.
    // Negative = target is to the left of the heading, positive = to the right.
    CKCORE_API auto
    Get_SignedBearingDegrees(
        const FVector& InObserverLocation,
        const FVector& InTargetLocation,
        float InHeadingDegrees) -> float;

    // Maps a signed bearing onto a compass arc: result is bearing / (arc/2), clamped to [-1, 1].
    // |unclamped| > 1 means the bearing lies outside the visible arc (the caller decides clamp-vs-hide).
    CKCORE_API auto
    Get_NormalizedArcOffset(
        float InSignedBearingDegrees,
        float InArcDegrees) -> float;

    // True when the signed bearing lies outside the visible arc (|bearing| > arc/2).
    CKCORE_API auto
    Get_IsOutsideArc(
        float InSignedBearingDegrees,
        float InArcDegrees) -> bool;

    // 8-way cardinal/ordinal for an absolute heading (degrees, any range — normalized internally).
    // Buckets are 45 degrees wide, centered on each direction (North covers [-22.5, 22.5)).
    CKCORE_API auto
    Get_CardinalAndOrdinalDirection(
        float InHeadingDegrees) -> ECk_CardinalAndOrdinalDirection;
}

// --------------------------------------------------------------------------------------------------------------------
