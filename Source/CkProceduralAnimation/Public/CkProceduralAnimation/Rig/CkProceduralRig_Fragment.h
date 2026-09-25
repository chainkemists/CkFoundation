#pragma once

#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ProceduralRig_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralRig_Update;

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ProceduralRig_Params = FCk_Fragment_ProceduralRig_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralRig_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralRig_Current);

    public:
        friend class FProcessor_ProceduralRig_Update;
        friend class ::UCk_Utils_ProceduralRig_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        TArray<FVector> _Joints;
        ECk_ProceduralRig_Failure _Failure = ECk_ProceduralRig_Failure::None;
        bool _Ready = false;
        uint64 _DebugGaitSequence = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
