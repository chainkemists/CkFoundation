#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The wide tiers come last, so they are tried only when every narrower angle crosses.
    inline constexpr float ProceduralPoleSwivelFanDegrees[] = {0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 90.0f, -90.0f, 120.0f, -120.0f,
        150.0f, -150.0f};

    // InPole rotated about the axis from InHip through InFoot by InDegrees (right-handed about hip->foot). A degenerate
    // axis (hip on the foot) or non-finite input returns InPole unchanged.
    CKPROCEDURALANIMATION_API auto
        ComputeProceduralPoleSwivel(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InPole,
            float InDegrees)
        -> FVector;

    // The fan order for one solve: InLastClearDegrees first when it is a fan angle other than 0, then 0, then the fan's
    // remaining angles in fan order. Writes at most the fan's count into OutOrder and returns the count.
    CKPROCEDURALANIMATION_API auto
        Get_ProceduralPoleSwivelOrder(
            float InLastClearDegrees,
            TArrayView<float> OutOrder)
        -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
