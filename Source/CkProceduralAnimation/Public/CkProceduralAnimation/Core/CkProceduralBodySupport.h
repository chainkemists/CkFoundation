#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPROCEDURALANIMATION_API FProceduralBodySupportLeg
    {
        CK_GENERATED_BODY(FProceduralBodySupportLeg);

    private:
        FVector _HipLocal = FVector::ZeroVector;
        float _Weight = 1.0f;

    public:
        CK_PROPERTY(_HipLocal);
        CK_PROPERTY(_Weight);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralBodySupportLeg, _HipLocal, _Weight);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralBodySupportSettings
    {
        CK_GENERATED_BODY(FProceduralBodySupportSettings);

    private:
        float _CollapseDrop = 40.0f;
        float _MaxTiltDegrees = 25.0f;

    public:
        CK_PROPERTY(_CollapseDrop);
        CK_PROPERTY(_MaxTiltDegrees);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Body-local offset the presentation should settle toward. Unset on malformed input: no legs, a non-finite
    // hip, a weight outside [0, 1], a negative or non-finite drop, or a tilt outside [0, 89] degrees.
    CKPROCEDURALANIMATION_API auto
        ComputeProceduralBodySupportPose(
            TArrayView<const FProceduralBodySupportLeg> InLegs,
            const FProceduralBodySupportSettings& InSettings)
        -> TOptional<FTransform>;
}

// --------------------------------------------------------------------------------------------------------------------
