#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Turns the agent toward its desired-velocity heading at _MaxTurnRate. Facing is decoupled from
    // travel direction, so the agent may briefly walk sideways through a sharp corner.
    //
    // The heading is only TRACKED while the agent is genuinely moving (_FacingSpeedFloorCm, with
    // hysteresis) and a large change must persist before it is committed — see the module's Claude.md
    // "Facing" section for why a body must not transcribe the sampler's per-frame winner.
    //
    // Group FGroup_Transform_SyncFrom: after FGroup_Physics so steering/integrator state has settled,
    // before FGroup_Transform so the rotation request drains in the same frame.
    // PumpPolicy::SkipPump: the enqueued delta is a fixed value, so a DeltaT=0 pump would re-enqueue
    // it and double the applied rotation.
    //
    // A flying agent is excluded and faced by FProcessor_CrowdAgent_FaceAngle3D instead: a climb or a
    // dive is invisible to a yaw-only heading, and the two views partition the agent population so
    // only one of them ever rotates a given agent.
    class CKCROWD_API FProcessor_CrowdAgent_FaceAngle : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_FaceAngle,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
            ck::TReadWrite<FFragment_CrowdAgent_FaceAngle>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            // GoalFailedHold is terminal until an explicit wake: translating and turning are both
            // observable motion, so never consume residual/avoidance velocity to re-face it.
            TExclude<FTag_CrowdAgent_GoalFailedHold>,
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
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_FaceAngle& InFaceAngle) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
