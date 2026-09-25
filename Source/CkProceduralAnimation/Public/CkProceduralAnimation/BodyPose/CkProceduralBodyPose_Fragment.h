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

    using FFragment_ProceduralBodyPose_Params = FCk_ProceduralBodyPose_Spec;

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
        FVectorSpringState _TranslationSpring;
        FQuaternionSpringState _RotationSpring;

    public:
        CK_PROPERTY_GET(_Offset);
    };
}

// --------------------------------------------------------------------------------------------------------------------
