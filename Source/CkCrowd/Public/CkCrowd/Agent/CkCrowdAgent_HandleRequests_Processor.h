#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drains and dispatches FFragment_CrowdAgent_MoveRequests. MoveTo arms PathFollow and fires a
    // CkNavigation FindPath, leaving PathPending for OnPathResolved to finish; Stop returns the
    // agent to Idle; SetMaxSpeed rewrites the params the steering chain reads each frame.
    //
    // Group FGroup_Gameplay, early enough that the path request is enqueued before
    // FProcessor_Nav_HandleRequests runs downstream.
    class CKCROWD_API FProcessor_CrowdAgent_HandleRequests : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_HandleRequests,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            ck::TReadWrite<FFragment_CrowdAgent_MoveRequests>,
            TExclude<FTag_DestroyEntity_Initiate>,
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
            FFragment_CrowdAgent_Params& InParams,
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

        // Ends any blocked episode: OnGoalBlocked may fire again for the new goal, and BlockedRecheck
        // can no longer resume the goal the caller abandoned.
        static auto
        DoClearBlockedState(
            FCk_Handle_CrowdAgent& InAgent) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetMaxSpeed& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed agent's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKCROWD_API FProcessor_CrowdAgent_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_CrowdAgent_CancelPendingRequests,
        FCk_Handle_CrowdAgent,
        ck::TReadOnly<FFragment_CrowdAgent_MoveRequests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_MoveRequests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
