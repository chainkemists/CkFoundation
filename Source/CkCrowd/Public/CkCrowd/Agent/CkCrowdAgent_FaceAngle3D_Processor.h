#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The flying counterpart of FProcessor_CrowdAgent_FaceAngle: turns the agent toward its
    // desired-velocity heading in yaw AND pitch, both at _MaxTurnRate. No roll — banking is a
    // presentation choice that needs its own turn model, not a facing one.
    //
    // Requires FTag_CrowdAgent_Flying, which the yaw-only processor excludes, so exactly one of the
    // two rotates any given agent. Group, pump policy and turn-rate constant are the yaw-only
    // processor's verbatim — this differs in the axes it may turn about, and in writing the reached
    // orientation absolutely rather than as a delta (see the body).
    //
    // It is a rotation writer and never touches translation, so it does not compete with the single
    // Transform-translation writer (ConstrainToNavmesh for a grounded agent, ApplyDisplacement3D for
    // a flying one): the two enqueue different Transform requests.
    class CKCROWD_API FProcessor_CrowdAgent_FaceAngle3D : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_FaceAngle3D,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Flying,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
            ck::TReadWrite<FFragment_CrowdAgent_FaceAngle>,
            TExclude<FTag_CrowdAgent_Asleep>,
            // Keep the terminal hold semantically stationary for flyers too; a later explicit
            // wake re-enters this view and resumes normal yaw/pitch facing.
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
