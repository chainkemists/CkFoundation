#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Places N rigid links hip-first along a planar cubic Bezier from InHip to InFoot bent toward InBendDirection, choosing the
    // arch height so the chain ends on the foot. Beyond reach the chain lies straight toward the foot. Returns false on
    // malformed input: N < 1 or > 8, non-finite points, non-positive lengths, OutJoints.Num() != N + 1, or a bend direction
    // (anti)parallel to hip->foot. The caller supplies its own fallback bend; the core never picks one.
    // A folded chain, whose foot lies within about 40 % of the chain length of the hip, can have no arch that closes on the
    // foot (the fewer and less even the links, the wider that band); its chain then ends off the foot.
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
