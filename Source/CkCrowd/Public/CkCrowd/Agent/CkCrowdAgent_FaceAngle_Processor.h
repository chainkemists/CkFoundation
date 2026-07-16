#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Reads the steering processor's _DesiredVelocity, derives a target yaw from its XY heading,
    // and lerps the agent's SceneNode rotation toward it at _MaxTurnRate rad/sec (clamped per-frame
    // by InDeltaT). Decoupled from movement direction — the agent can briefly walk sideways while
    // turning, which is the natural look during sharp waypoint corners.
    //
    // Group: FGroup_Transform_SyncFrom — runs AFTER FGroup_Physics so all steering / velocity-bridge
    // / integrator pumping has settled, and BEFORE FGroup_Transform so the Request_AddRotationOffset
    // enqueued here is drained by FProcessor_Transform_HandleRequests in the same frame.
    //
    // PumpPolicy::SkipPump — the processor enqueues a fixed-value rotation request from cached
    // SceneNode state; pumping with DeltaT=0 would re-enqueue the same delta and double the applied
    // rotation per frame. Same pattern as FProcessor_CrowdAgent_ApplyOffset.
    class CKCROWD_API FProcessor_CrowdAgent_FaceAngle : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_FaceAngle,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
            ck::TReadWrite<FFragment_CrowdAgent_FaceAngle>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_FaceAngle& InFaceAngle) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
