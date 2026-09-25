#pragma once

#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"

class UCk_Utils_ProceduralRig_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

namespace ck
{
    using FFragment_ProceduralRig_Params = FCk_Fragment_ProceduralRig_ParamsData;

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralRig_Current
    {
        CK_GENERATED_BODY(FFragment_ProceduralRig_Current);
        friend class FProcessor_ProceduralRig_Update;
        friend class UCk_Utils_ProceduralRig_UE;
        friend class UCk_Utils_ProceduralAnimation_Debug_UE;
    private:
        TArray<int32> _GaitLegIndices;
        TArray<bool> _HasFoot;
        uint64 _DebugGaitSequence = 0;
        ECk_ProceduralRig_Failure _Failure = ECk_ProceduralRig_Failure::None;
        bool _Ready = false;
    };
}
