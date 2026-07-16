#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drains move-related requests from FFragment_CrowdAgent_MoveRequests
    // and dispatches them.
    //
    // - MoveTo: cache the per-request _ActiveArrivalRadius on PathFollow (params default OR override),
    //   reset _WaypointIndex, transition Idle/Walking → PathPending, fire CkNavigation FindPath. The
    //   downstream OnPathResolved processor flips PathPending → Walking (or Idle + OnGoalFailed) once
    //   CkNavigation lands the FFragment_Nav_PathResult.
    // - Stop: zero _DesiredVelocity, transition Walking/PathPending → Idle. Steering's view filter
    //   (which also requires Walking) stops firing; bridge writes zero velocity → agent halts.
    //
    // Group: FGroup_Gameplay (early in the frame so the path request is enqueued before
    // FProcessor_Nav_HandleRequests runs in FGroup_Gameplay's downstream group).
    class CKCROWD_API FProcessor_CrowdAgent_HandleRequests : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_HandleRequests,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            ck::TReadWrite<FFragment_CrowdAgent_MoveRequests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_CrowdAgent_MoveRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_MoveRequests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_MoveTo& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_FollowTarget& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_Stop& InRequest) -> void;

        // A new order from gameplay ends any blocked episode: drop GoalBlocked and reset the detector,
        // so OnGoalBlocked can fire again for the new goal and BlockedRecheck cannot resume a goal the
        // caller has abandoned.
        static auto
        DoClearBlockedState(
            FCk_Handle_CrowdAgent& InAgent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
