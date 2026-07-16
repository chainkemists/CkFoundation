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
    // Copies the steering output into the physics velocity layer.
    //
    // The pipeline beats: Steering writes _DesiredVelocity → Bridge writes _CurrentVelocity →
    // Velocity_Clamp trims by min/max → EulerIntegrator computes _DistanceOffset → ApplyOffset
    // enqueues Request_AddLocationOffset → Transform_HandleRequests advances the SceneNode.
    //
    // Group: FGroup_Physics. RunAfter Steering (read its output) AND AccelClamp (ship the RAMPED
    // velocity, not the raw target): AvoidanceSample overwrites _DesiredVelocity after Steering and
    // AccelClamp bounds the per-frame direction+magnitude change, so the bridge must observe the
    // final, ramped value. Same-group order is registration order, so without this dependency the
    // correct sequencing was luck. RunBefore Velocity_Clamp + EulerIntegrator (so the clamp +
    // integrator see the bridge's value, not last frame's).
    //
    // The bridge calls UCk_Utils_Velocity_UE::Request_OverrideVelocity (public API) rather than
    // touching FFragment_Velocity_Current::_CurrentVelocity directly, so no cross-module friend
    // declaration is needed.
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
