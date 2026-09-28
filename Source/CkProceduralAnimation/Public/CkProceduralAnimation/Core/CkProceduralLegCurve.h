#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Places N rigid links hip-first along a planar cubic Bezier from InHip to InFoot bent toward InBendDirection, choosing the
    // arch height so the chain ends on the foot. A folded curve that misses the foot receives a bounded, curve-seeded planar
    // closure pass (0.01 cm target), first keeping the curve's proximal link if its suffix can close. If that suffix does not
    // close within 64 passes, a full-chain retry adds at most 64 passes (128 total). An already closing curve keeps its
    // original shape. Beyond reach
    // the chain lies straight toward the foot. Returns false on malformed input: N < 1 or > 8, non-finite points,
    // non-positive lengths, OutJoints.Num() != N + 1, or a bend direction
    // (anti)parallel to hip->foot. The caller supplies its own fallback bend; the core never picks one.
    // Targets inside the rigid chain's inner reach cannot be closed. At a coincident hip and foot the bend plane is undefined,
    // so the existing finite rigid-link curve is retained. Bounded correction returns the best found pose and does not
    // guarantee exact closure for every reachable target. The curve's branch switch can still cause a joint-pose discontinuity
    // near the folded-to-unfolded boundary.
    CKPROCEDURALANIMATION_API auto
        SolveProceduralLegCurve(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InBendDirection,
            TArrayView<const float> InLengths,
            TArrayView<FVector> OutJoints)
        -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
