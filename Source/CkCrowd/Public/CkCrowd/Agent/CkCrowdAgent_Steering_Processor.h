#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gate 2 — Sub-task 2B path follower / steering.
    // Reads the agent's current location + path result, advances the waypoint cursor as the agent
    // crosses each waypoint's arrival radius, and writes a desired velocity sized by:
    //   - direction toward next waypoint
    //   - speed ramp clamped by _MaxAcceleration
    //   - braking ramp keyed off remaining distance to FINAL waypoint (sqrt(2 * decel * d))
    //
    // Output is FFragment_CrowdAgent_DesiredVelocity. Sub-task 2C will copy this into
    // FFragment_Velocity_Current via a velocity-bridge processor; until then it's purely advisory
    // and observable via UCk_Utils_CrowdAgent_UE::Get_DesiredVelocity.
    //
    // Group: FGroup_Physics. RunBefore EulerIntegrator_Update so the velocity-bridge (when it lands)
    // can write Velocity_Current before the integrator reads it.
    class CKCROWD_API FProcessor_CrowdAgent_Steering : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_Steering,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunBefore = TDepList<FProcessor_EulerIntegrator_Update>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
