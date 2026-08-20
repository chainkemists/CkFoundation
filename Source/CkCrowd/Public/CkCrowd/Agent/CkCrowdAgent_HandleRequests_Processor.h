#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Request_Nav_FindPath;

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

        static auto
		Request_NavigationPath(
			HandleType InHandle,
			const FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			const FVector& InGoal,
			bool InForcePermissivePlan = false) -> void;

		// Invalidates CkNavigation results issued before this route dispatch. This advances for
		// every provider so a late CkNavigation result cannot replace a newer non-navigation route.
		static auto
		AdvanceNavigationRequestRevision(FFragment_CrowdAgent_PathFollow& InPathFollow) -> int32;

		// Ends the active path episode: advances the revision and releases whichever provider owns
		// the in-flight query, so the shared FFragment_Nav_PathResult never outlives the episode
		// that parked it. The single release for a TERMINAL — Stop and the provider fork both route
		// through here rather than each remembering to clean up, which is how the orphaned-Pending
		// slot arose. (The five sites that re-dispatch a CkNavigation query directly — both
		// fallbacks, BlockDetect's stall re-path, PathRefresh, ForceReplan — advance the revision
		// without it, which is safe only because each dispatches from a state whose prior query has
		// already reached a terminal.)
		// Deliberately does NOT clear _ActiveGoal: a re-dispatch reads it immediately afterwards.
		static auto
		DoAbandonActiveProviderQuery(HandleType InHandle, FFragment_CrowdAgent_PathFollow& InPathFollow) -> int32;

		// Releases the PROVIDER's half of an episode without advancing the revision — for a terminal
		// that must keep the current revision so its own result is still recognised as the answer
		// (the pending watchdog's timeout). Ending an episode and leaving the provider's corridor
		// parked would reproduce the orphan one layer down, on the provider's own result.
		static auto
		DoReleaseProviderQuery(HandleType InHandle, ECk_CrowdAgent_PathProvider InProvider, int32 InRevision) -> void;

		// Dispatches a fresh query at _ActiveGoal through whichever provider owns this agent, and
		// is the ONLY sanctioned way for a framework-internal re-path to reach one — the caller-side
		// same-goal guard would otherwise swallow it. The caller owns the tag transition into
		// PathPending; this advances the request revision and parks the result slot.
		static auto
		RequestPathForActiveGoal(
			HandleType InHandle,
			const FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;

		// Stamps the planning phase and its filter onto a CkNavigation FindPath request. Every
		// FRESH dispatch tries strict first (a crowd-free route may exist now even if it did not a
		// moment ago). The one caller that must not retry strict — OnPathResolved's
		// strict→permissive fallback — passes InForcePermissive, which also leaves
		// _StrictPlanFailed exactly as the fallback set it.
		static auto
		ApplyPlanPhase(
			const FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			FCk_Request_Nav_FindPath& InOutRequest,
			bool InForcePermissive = false) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_MoveTo& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_FollowTarget& InRequest) -> ECk_Request_OperationResult;

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
		DoForceReplan(
			HandleType InHandle,
			const FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity) -> void;

        static auto
		DoHandleRequest(
			HandleType InHandle,
			FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			FFragment_CrowdAgent_DesiredVelocity& InDesired,
			const FCk_Request_CrowdAgent_SetNavQueryFilter& InRequest) -> void;

		static auto
		DoHandleRequest(
			HandleType InHandle,
			FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			FFragment_CrowdAgent_DesiredVelocity& InDesired,
			const FCk_Request_CrowdAgent_SetMaxSpeed& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale& InRequest) -> void;
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
