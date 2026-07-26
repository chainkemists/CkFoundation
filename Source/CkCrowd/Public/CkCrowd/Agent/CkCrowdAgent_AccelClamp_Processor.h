#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Clamps the per-frame velocity delta on FFragment_CrowdAgent_DesiredVelocity: Steering writes a
    // fresh Direction * TargetSpeed each frame, so capping |dv| forces direction flips to ramp.
    class CKCROWD_API FProcessor_CrowdAgent_AccelClamp : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_AccelClamp,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<
            FProcessor_CrowdAgent_Steering,
            FProcessor_CrowdAgent_AvoidanceSample>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
