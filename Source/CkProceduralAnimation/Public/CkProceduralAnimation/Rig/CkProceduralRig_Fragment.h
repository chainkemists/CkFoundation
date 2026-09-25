#pragma once

#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ProceduralRig_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralRig_Setup;
    class FProcessor_ProceduralRig_Update;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_ProceduralRig_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ProceduralRig_Params = FCk_ProceduralRig_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralRig_Failure
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralRig_Failure);

    private:
        ECk_ProceduralRig_Failure _Reason = ECk_ProceduralRig_Failure::None;

    public:
        CK_PROPERTY_GET(_Reason);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralRig_Failure, _Reason);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralRig
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralRig);

    public:
        friend class FProcessor_ProceduralRig_Setup;
        friend class FProcessor_ProceduralRig_Update;
        friend class ::UCk_Utils_ProceduralRig_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        TArray<FVector> _Joints;
        uint64 _PosedSolveSequence = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
