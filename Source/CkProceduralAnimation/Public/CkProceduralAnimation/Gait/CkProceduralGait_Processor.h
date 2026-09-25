#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralGait_Update : public ck_exp::TProcessor<
        FProcessor_ProceduralGait_Update, FCk_Handle_ProceduralGait,
        TReadOnly<FFragment_ProceduralGait_Params>, TReadWrite<FFragment_ProceduralGait_Current>,
        TReadOnly<FFragment_Transform>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using TProcessor::TProcessor;
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Params& InParams,
            FFragment_ProceduralGait_Current& InCurrent,
            const FFragment_Transform& InTransform) -> void;
    };
}
