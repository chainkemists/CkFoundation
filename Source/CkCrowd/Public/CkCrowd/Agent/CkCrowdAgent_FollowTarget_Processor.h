#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Keeps a FollowTarget request tracking its LIVE target: on the request's repath cadence it
    // re-issues the stored request whenever the target has drifted from the active goal, and
    // re-engages an already-arrived (Idle) agent once the target leaves its arrival radius.
    // Re-issuing the stored request reuses all MoveTo machinery verbatim (same-goal epsilon,
    // blocked-state reset, path-network routing).
    //
    // Group FGroup_Gameplay, before HandleRequests — a repath issued this frame drains in the
    // same frame's request pass.
    class CKCROWD_API FProcessor_CrowdAgent_FollowTarget : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_FollowTarget,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_FollowTarget>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunBefore = TDepList<FProcessor_CrowdAgent_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_FollowTarget& InFollow) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
