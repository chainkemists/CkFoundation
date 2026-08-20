#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_VelocityBridge_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Immediate-mode marker over any agent whose planar speed sits inside the sub-floor band
    // (0, ck.Crowd.Debug.SubFloorSpeedCm]: a small ring above the head plus a fixed-length needle
    // along the instantaneous velocity heading. This is the diag recorder's exact view of the
    // agent — a needle whipping over a visibly still body means the recorder is counting heading
    // in a state the eye cannot detect. Purely observational; off by default.
    class CKCROWD_API FProcessor_CrowdAgent_DrawSubFloorSpeed : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawSubFloorSpeed,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_Velocity_Current>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_VelocityBridge>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Velocity_Current& InVelocity) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
