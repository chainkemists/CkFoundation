#pragma once

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralBodyPose_Update : public ck_exp::TProcessor<
        FProcessor_ProceduralBodyPose_Update,
        FCk_Handle_ProceduralBodyPose,
        TReadOnly<FFragment_ProceduralBodyPose_Params>,
        TReadWrite<FFragment_ProceduralBodyPose>,
        TReadOnly<FFragment_ProceduralGait>,
        TReadOnly<FFragment_Transform>,
        TExclude<FFragment_ProceduralBodyPose_Failure>,
        TExclude<FTag_ProceduralGait_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralGait_Update>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralBodyPose_Params& InParams,
            FFragment_ProceduralBodyPose& InPoseComp,
            const FFragment_ProceduralGait& InGaitComp,
            const FFragment_Transform& InTransform)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
