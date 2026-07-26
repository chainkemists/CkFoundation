#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"
#include "CkPhysics/Velocity/CkVelocity_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Copies the steering output into the physics velocity layer. RunAfter BOTH Steering and
    // AccelClamp — AvoidanceSample overwrites _DesiredVelocity after Steering, so only the ramped
    // post-clamp value is safe to ship, and same-group order alone is just registration order.
    class CKCROWD_API FProcessor_CrowdAgent_VelocityBridge : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_VelocityBridge,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering, FProcessor_CrowdAgent_AccelClamp>;
        using RunBefore = TDepList<FProcessor_Velocity_Clamp, FProcessor_EulerIntegrator_Update>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
