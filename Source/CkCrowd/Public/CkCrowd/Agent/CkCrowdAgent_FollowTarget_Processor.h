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
    // Keeps a FollowTarget request tracking its LIVE target: on the request's repath cadence,
    // re-resolves the target point's position and re-issues the follow when it has drifted from
    // the active goal — so an agent chasing a moving entity re-paths smoothly instead of walking
    // to a stale snapshot. Also re-engages an agent that already "arrived" (Idle) when the target
    // has since walked back out of the arrival radius.
    //
    // The re-issue is the stored FollowTarget request itself, so all MoveTo machinery is reused
    // verbatim (same-goal epsilon, blocked-state reset, path-network routing). A dead target ends
    // the follow: the agent keeps its last goal and behaves like a plain MoveTo.
    //
    // View: FFragment_CrowdAgent_FollowTarget presence IS the filter; the agent's own transform
    // fragment is part of the view (a crowd agent always has one — its moving transform is the
    // feature's substrate), so the arrived-distance check reads it without a per-entity cast.
    //
    // Group: FGroup_Gameplay, before HandleRequests — a repath issued this frame drains in the
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
