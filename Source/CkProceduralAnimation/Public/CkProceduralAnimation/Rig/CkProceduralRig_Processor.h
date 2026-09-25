#pragma once

#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Processor.h"

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralRig_Update : public ck_exp::TProcessor<
        FProcessor_ProceduralRig_Update, FCk_Handle_ProceduralRig,
        TReadOnly<FFragment_ProceduralRig_Params>, TReadWrite<FFragment_ProceduralRig_Current>,
        TReadOnly<FFragment_ProceduralGait_Params>, TReadOnly<FFragment_ProceduralGait_Current>,
        TReadOnly<FFragment_Transform>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralGait_Update>;
        using TProcessor::TProcessor;
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralRig_Params& InParams,
            FFragment_ProceduralRig_Current& InCurrent,
            const FFragment_ProceduralGait_Params& InGaitParams,
            const FFragment_ProceduralGait_Current& InGait,
            const FFragment_Transform& InTransform) -> void;
    };
}
