#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

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

    // --------------------------------------------------------------------------------------------------------------------

    // One foot in the body's frame: where it is, where it rests, and how much it counts toward the fitted plane.
    struct CKPROCEDURALANIMATION_API FProceduralBodyConformFoot
    {
        CK_GENERATED_BODY(FProceduralBodyConformFoot);

    private:
        FVector _PositionLocal = FVector::ZeroVector;
        FVector _RestLocal = FVector::ZeroVector;
        float _Weight = 0.0f;

    public:
        CK_PROPERTY(_PositionLocal);
        CK_PROPERTY(_RestLocal);
        CK_PROPERTY(_Weight);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralBodyConformFoot, _PositionLocal, _RestLocal, _Weight);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralBodyConformSettings
    {
        CK_GENERATED_BODY(FProceduralBodyConformSettings);

    private:
        float _MaxTiltDegrees = 20.0f;
        float _HeightWeight = 0.5f;
        float _MaxHeight = 10.0f;

    public:
        CK_PROPERTY(_MaxTiltDegrees);
        CK_PROPERTY(_HeightWeight);
        CK_PROPERTY(_MaxHeight);
    };

    // --------------------------------------------------------------------------------------------------------------------

    enum class EProceduralBodyConformResult : uint8
    {
        Fitted,
        Underdetermined,
        Malformed
    };

    // Body-local tilt and height toward the weighted least-squares plane h = a x + b y + c through each foot's height above
    // its rest height, at the foot's body-local x and y. The tilt turns the body's up onto the plane's normal, clamped to
    // MaxTilt; the height is c times HeightWeight, clamped to MaxHeight. Only Fitted writes OutTarget. Underdetermined: fewer
    // than three weighted feet, or weighted feet along one line. Malformed: a non-finite foot, a negative or non-finite
    // weight, a tilt outside [0, 89] degrees, a height weight outside [0, 1], or a negative or non-finite height.
    CKPROCEDURALANIMATION_API auto
        ComputeProceduralBodyConformPose(
            TArrayView<const FProceduralBodyConformFoot> InFeet,
            const FProceduralBodyConformSettings& InSettings,
            FTransform& OutTarget)
        -> EProceduralBodyConformResult;

    // --------------------------------------------------------------------------------------------------------------------

    // How fast the applied conform target may turn, in degrees per second, and move, in centimetres per second.
    struct CKPROCEDURALANIMATION_API FProceduralBodyConformSlewSettings
    {
        CK_GENERATED_BODY(FProceduralBodyConformSlewSettings);

    private:
        float _MaxTiltRateDegrees = 80.0f;
        float _MaxHeightRate = 60.0f;

    public:
        CK_PROPERTY(_MaxTiltRateDegrees);
        CK_PROPERTY(_MaxHeightRate);
    };

    // InApplied moved toward InTarget by at most MaxTiltRate times InDeltaTime of rotation (along the shortest arc) and
    // MaxHeightRate times InDeltaTime of translation. A conform fit swaps its target in one frame when its feet cross a
    // crease; slewed, the target the springs chase turns at a bounded rate instead. Unset on malformed input: a non-finite
    // transform, a non-finite or non-positive rate, or a non-finite or negative delta time.
    CKPROCEDURALANIMATION_API auto
        SlewProceduralBodyConformPose(
            const FTransform& InApplied,
            const FTransform& InTarget,
            const FProceduralBodyConformSlewSettings& InSettings,
            FCk_Time InDeltaTime)
        -> TOptional<FTransform>;
}

// --------------------------------------------------------------------------------------------------------------------
