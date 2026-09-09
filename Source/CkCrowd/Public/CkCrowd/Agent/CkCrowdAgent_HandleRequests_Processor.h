#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGroundNav/Query/CkGroundNav_Query_DynamicObstacles.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment_Data.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Request_Nav_FindPath;
struct FCk_Request_PathNetworkFollower_FindRoute;

namespace ck_crowd_agent_handle_requests
{
    /**
     * What ONE planning phase decides about the filter a route is planned under.
     *
     * The phase is the crowd's own idea, but strict standing-crowd treatment differs at the provider
     * boundary: Recast excludes its per-polygon area while GroundNav prices the area and verifies the
     * returned geometry at install. Keep that decision here so every GroundNav dispatch agrees.
     */
    struct FCk_CrowdAgent_PlanPhaseFilter
    {
        ECk_CrowdAgent_PlanPhase _Phase = ECk_CrowdAgent_PlanPhase::Permissive;

        FGameplayTag _QueryFilter;

        // Outranks _QueryFilter where it is set, which is the precedence Recast's own resolver applies.
        FGameplayTag _QueryFilterOverride;

        FCk_Nav_QueryFilterOverlay _QueryFilterOverlay;

        bool _UsesStrictStandingCrowdFilter = false;

        /** The ONE tag a provider carrying a single filter field is planned under. */
        auto Get_EffectiveQueryFilter() const -> FGameplayTag
        {
            return _QueryFilterOverride.IsValid() ? _QueryFilterOverride : _QueryFilter;
        }
    };

    /**
     * The phase decision every FRESH dispatch makes: strict first, because a crowd-free route may
     * exist now even if it did not a moment ago. The one caller that must not retry strict —
     * OnPathResolved's strict→permissive fallback — passes InForcePermissive.
     *
     * _StrictPlanFailed is deliberately NOT reset here. Only a dispatch carrying NEW evidence retries
     * strict — a fresh MoveTo, a BlockedRecheck resume (the pack drained), a PathRefresh trigger (a
     * new disc confirmed), a caller ForceReplan — and those sites reset the flag themselves. The stall
     * ladder's re-paths carry no new evidence: retrying strict there re-fails against the same plugged
     * route and doubles every rung's Pending stop-start cycle, which the body visibly tracks (measured
     * as a facing-whip regression).
     *
     * Declared here rather than kept .cpp-local because every FRESH path dispatch — the two
     * providers' plans and the A/B shadow that has to mirror whichever one ran — derives its filter
     * from this one decision, and a site that hardcodes the agent's base filter instead plans
     * straight THROUGH painted standing-crowd markup: the base filter prices an agent disc, while
     * GroundNav applies the strict geometric verdict at install and Recast excludes it. The PathNetwork route requests deliberately
     * do NOT come through here — that provider announces a strict route MISS as a route FAILURE at
     * resolution, so it has no strict-then-permissive retry and must plan under the base filter,
     * leaving confirmed discs to the markup bypass helpers.
     */
    auto Get_PlanPhaseFilter(
        FCk_Handle_CrowdAgent                      InHandle,
        const ck::FFragment_CrowdAgent_Params&     InParams,
        const ck::FFragment_CrowdAgent_PathFollow& InPathFollow,
        ECk_CrowdAgent_PathProvider                InProvider,
        bool                                       InForcePermissive) -> FCk_CrowdAgent_PlanPhaseFilter;
}

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

        static auto
		Request_NavigationPath(
			HandleType InHandle,
			const FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			const FVector& InGoal,
			bool InForcePermissivePlan = false,
			ECk_GroundNav_PlanMode InPlanMode = ECk_GroundNav_PlanMode::Cold) -> void;

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
			HandleType InHandle,
			const FFragment_CrowdAgent_Params& InParams,
			FFragment_CrowdAgent_PathFollow& InPathFollow,
			FCk_Request_Nav_FindPath& InOutRequest,
			bool InForcePermissive = false) -> void;

        static auto
        Get_ShouldPlanStrict(
            HandleType InHandle,
            const FFragment_CrowdAgent_PathFollow& InPathFollow) -> bool;

        // Captures the complete confirmed strict geometry as one immutable GroundNav query value.
        // Unset is terminal: callers must not discard malformed records and continue permissively.
        static auto
        Try_GetStrictDynamicObstacles(
            FCk_Handle InSelf,
            const FVector& InGoal,
            float InAgentRadius,
            float InArrivalRadius = 0.0f) -> TOptional<ck::groundnav::FCk_GroundNav_DynamicObstacleSnapshot>;

        static auto
        FailStrictGroundNavDispatch(HandleType InHandle, int32 InRevision) -> void;

        static auto
        ApplyMarkupEscapeStart(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FVector& InGoal,
            FCk_Request_PathNetworkFollower_FindRoute& InOutRequest) -> void;

        static auto
        GetPlanQueryFilterTag(
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PathFollow& InPathFollow)
            -> FGameplayTag;

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
