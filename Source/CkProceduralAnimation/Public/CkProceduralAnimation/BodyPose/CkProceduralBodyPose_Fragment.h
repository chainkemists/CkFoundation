#pragma once

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ProceduralBodyPose_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralBodyPose_Update;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralBodyPose_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralBodyPose_Params);

    private:
        FCk_Handle_Transform _Presentation;
        FCk_ProceduralBodyPose_Spring _Spring;
        FCk_ProceduralBodyPose_Support _Support;

    public:
        CK_PROPERTY_GET(_Presentation);
        CK_PROPERTY_GET(_Spring);
        CK_PROPERTY_GET(_Support);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralBodyPose_Params, _Presentation, _Spring, _Support);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Present only while the body pose conforms to its planted feet.
    struct CKPROCEDURALANIMATION_API FFragment_ProceduralBodyPose_Conform
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralBodyPose_Conform);

    private:
        float _MaxTilt = 20.0f;
        float _HeightWeight = 0.5f;
        float _MaxHeight = 10.0f;

    public:
        CK_PROPERTY_GET(_MaxTilt);
        CK_PROPERTY_GET(_HeightWeight);
        CK_PROPERTY_GET(_MaxHeight);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralBodyPose_Conform, _MaxTilt, _HeightWeight, _MaxHeight);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The last fitted conform target, body-local; the update holds it while the feet cannot determine a plane.
    struct CKPROCEDURALANIMATION_API FFragment_ProceduralBodyPose_ConformState
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralBodyPose_ConformState);

    public:
        friend class FProcessor_ProceduralBodyPose_Update;

    private:
        FTransform _HeldTarget = FTransform::Identity;

    public:
        CK_PROPERTY_GET(_HeldTarget);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralBodyPose_Failure
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralBodyPose_Failure);

    private:
        ECk_ProceduralBodyPose_Failure _Reason = ECk_ProceduralBodyPose_Failure::None;

    public:
        CK_PROPERTY_GET(_Reason);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralBodyPose_Failure, _Reason);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Hip positions of the gait's captured legs, by captured index, taken at Add. A detached leg's entity is
    // gone, so its hip must come from here for the support gap to keep pointing at it.
    struct CKPROCEDURALANIMATION_API FFragment_ProceduralBodyPose_SupportLayout
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralBodyPose_SupportLayout);

    private:
        TArray<FVector> _HipLocals;

    public:
        CK_PROPERTY_GET(_HipLocals);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralBodyPose_SupportLayout, _HipLocals);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralBodyPose
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralBodyPose);

    public:
        friend class FProcessor_ProceduralBodyPose_Update;
        friend class ::UCk_Utils_ProceduralBodyPose_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        // Body-local; the presentation entity is posed at _Offset * Body.
        FTransform _Offset = FTransform::Identity;
        // Body-local; the pose the springs followed on the last update.
        FTransform _TargetOffset = FTransform::Identity;
        FVectorSpringState _TranslationSpring;
        FQuaternionSpringState _RotationSpring;
        FQuat _LastBodyRotation = FQuat::Identity;

    public:
        CK_PROPERTY_GET(_Offset);
        CK_PROPERTY_GET(_TargetOffset);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralBodyPose, _LastBodyRotation);
    };
}

// --------------------------------------------------------------------------------------------------------------------
