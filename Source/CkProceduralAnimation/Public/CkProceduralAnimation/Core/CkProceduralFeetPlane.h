#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // A plane through the supporting feet that tilts further than this from the support up is a wall or a steep slope the
    // feet happen to stand on, not ground the body can ride: the gait publishes no such plane, and surface motion ignores
    // one this far from its own support normal.
    constexpr auto ProceduralFeetPlaneMaxAngleDegrees = 45.0f;

    // --------------------------------------------------------------------------------------------------------------------

    // One foot in the support frame, relative to the body, and how much it counts toward the plane.
    struct CKPROCEDURALANIMATION_API FProceduralFeetPlaneFoot
    {
        CK_GENERATED_BODY(FProceduralFeetPlaneFoot);

    private:
        FVector _Position = FVector::ZeroVector;
        float _Weight = 0.0f;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Weight);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralFeetPlaneFoot, _Position, _Weight);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The fitted plane in the support frame: where it crosses the frame's up axis, (0, 0, c), and its unit normal.
    struct CKPROCEDURALANIMATION_API FProceduralFeetPlane
    {
        CK_GENERATED_BODY(FProceduralFeetPlane);

    private:
        FVector _Point = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;

    public:
        CK_PROPERTY(_Point);
        CK_PROPERTY(_Normal);
    };

    // --------------------------------------------------------------------------------------------------------------------

    enum class EProceduralFeetPlaneResult : uint8
    {
        Fitted,
        Underdetermined,
        TooSteep,
        Malformed
    };

    // Weighted least squares z = a x + b y + c through the feet with weight > 0. Only Fitted writes OutPlane.
    // Underdetermined: fewer than three weighted feet or weighted feet along one line. TooSteep: the fitted normal is more
    // than InMaxAngleDegrees from +Z. Malformed: a non-finite position, a negative or non-finite weight, or a max angle
    // outside [0, 90].
    CKPROCEDURALANIMATION_API auto
        FitProceduralFeetPlane(
            TArrayView<const FProceduralFeetPlaneFoot> InFeet,
            float InMaxAngleDegrees,
            FProceduralFeetPlane& OutPlane)
        -> EProceduralFeetPlaneResult;
}

// --------------------------------------------------------------------------------------------------------------------
