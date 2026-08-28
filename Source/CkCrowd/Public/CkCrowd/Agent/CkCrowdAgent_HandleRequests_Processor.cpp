#include "CkCrowdAgent_HandleRequests_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_NavQueryFilter.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment.h"
#include "CkVoxelNav/Path/CkVoxelNavPath_Utils.h"

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::HandleRequests"), STAT_CkCrowd_HandleRequestsProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_MoveRequests& InRequests) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_HandleRequestsProc);

        InHandle.CopyAndRemove(InRequests, [&](const auto& InSnapshot)
        {
            // Policy is setup, not a sequential movement command: choose the final value before
            // dispatching this batch so SetPolicy + MoveTo is order-independent within one frame.
            const auto* LastPolicyRequest = static_cast<const FCk_Request_CrowdAgent_SetNavQueryFilter*>(nullptr);
            for (const auto& Request : InSnapshot._Requests)
            {
                if (const auto* Policy = std::get_if<FCk_Request_CrowdAgent_SetNavQueryFilter>(&Request))
                { LastPolicyRequest = Policy; }
            }
            if (LastPolicyRequest != nullptr)
            { InParams._NavQueryFilter = LastPolicyRequest->Get_NavQueryFilter(); }
            // Whether a movement command actually dispatched a route. Asked per-request rather
            // than by diffing the revision across the whole batch, because ENDING an episode
            // advances the revision too — a Stop in this batch would otherwise read as "a route
            // was dispatched" and silently suppress the force-replan below.
            auto MovementCommandDispatched = false;

            algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
            [&](const auto& InRequest)
            {
                using RequestType = std::decay_t<decltype(InRequest)>;
                constexpr auto CanDispatchRoute =
                    std::is_same_v<RequestType, FCk_Request_CrowdAgent_MoveTo>
                    || std::is_same_v<RequestType, FCk_Request_CrowdAgent_FollowTarget>;

                const auto RevisionBeforeRequest = InPathFollow.Get_ActiveNavigationRequestRevision();

                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                if constexpr (CanDispatchRoute)
                {
                    Result = DoHandleRequest(
                        InHandle, InParams, InPathFollow, InDesired, InRequest);

                    if (InPathFollow.Get_ActiveNavigationRequestRevision() != RevisionBeforeRequest)
                    { MovementCommandDispatched = true; }
                }
                else
                {
                    // These overloads are void and have no rejection path, so reaching the line
                    // after the call IS the success condition.
                    DoHandleRequest(InHandle, InParams, InPathFollow, InDesired, InRequest);
                    Result = ECk_Request_OperationResult::Succeeded;
                }
            }), policy::DontResetContainer{});

            // A MoveTo/FollowTarget that actually dispatched a route already used the resolved
            // policy. If it was a same-goal no-op (or no movement command appeared), force-replan
            // the current episode without changing its goal/correlation/follow ownership.
            if (LastPolicyRequest != nullptr
                && LastPolicyRequest->Get_ForceReplan() == ECk_EnableDisable::Enable
                && NOT MovementCommandDispatched)
            {
                DoForceReplan(InHandle, InParams, InPathFollow, InDesired);
            }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        GetPlanQueryFilterClass(
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> TSubclassOf<UNavigationQueryFilter>
    {
        if (InPathFollow.Get_PlanPhase() == ECk_CrowdAgent_PlanPhase::Strict)
        {
            if (InParams.Get_NavQueryFilterStrict().IsValid())
            {
                return UCk_Utils_Nav_Settings_UE::Get_QueryFilterClass(
                    InParams.Get_NavQueryFilterStrict());
            }
            return UCk_NavQueryFilter_AvoidStandingCrowds::StaticClass();
        }
        return UCk_Utils_Nav_Settings_UE::Get_QueryFilterClass(InParams.Get_NavQueryFilter());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        ApplyMarkupEscapeStart(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FVector& InGoal,
            FCk_Request_PathNetworkFollower_FindRoute& InOutRequest)
        -> void
    {
        auto Transform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (NOT ck::IsValid(Transform))
        { return; }

        const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
            InHandle,
            InHandle.Get_Entity(),
            UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Transform),
            InGoal,
            InParams.Get_Radius());
        if (Escaped.IsSet())
        {
            InOutRequest.Set_StartOverride(ECk_EnableDisable::Enable)
                        .Set_StartOverrideLocation(Escaped.GetValue());
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_MoveTo& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto Goal = InRequest.Get_Target();

        // Re-issuing the goal we are already walking to resets the waypoint cursor, so a noisy
        // re-issuer would stop the final-stop ever latching and the agent would orbit its goal.
        constexpr auto SameGoalEpsilonCm = 20.0f;
        const auto IsSameUnforcedClaim = NOT InRequest.Get_ForceRepath()
            && FVector::Dist(Goal, InPathFollow.Get_ActiveGoal()) <= SameGoalEpsilonCm
            && (InRequest.Get_CorrelationId() == 0
                || InRequest.Get_CorrelationId() == InPathFollow.Get_ActiveMoveCorrelationId());

        if (InHandle.Has<FTag_CrowdAgent_GoalFailedHold>() && IsSameUnforcedClaim)
        {
            ck::crowd::Log(
                TEXT("CrowdAgent [{}] MoveTo {} rejected (same failed-held goal without a new nonzero correlation {} or ForceRepath)"),
                InHandle, Goal, InRequest.Get_CorrelationId());
            // The request reached and was drained by this processor, so Failed_NotEnqueued would
            // be false. Failed means the caller's requested movement episode was not dispatched.
            return ECk_Request_OperationResult::Failed;
        }

        // A plain MoveTo takes over from any follow in flight; the FollowTarget handler delegates
        // here and re-adds its own state afterwards. Preserve the legacy active-walk no-op contract.
        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();

        if (InHandle.Has<FTag_CrowdAgent_Walking>() && IsSameUnforcedClaim)
        {
            ck::crowd::Log(
                TEXT("CrowdAgent [{}] MoveTo {} ignored (same active goal without a new nonzero correlation {} or ForceRepath)"),
                InHandle, Goal, InRequest.Get_CorrelationId());
            return ECk_Request_OperationResult::Succeeded;
        }

        InPathFollow._ActiveMoveEpisode = InPathFollow._ActiveMoveEpisode == MAX_int32
            ? 1
            : InPathFollow._ActiveMoveEpisode + 1;
        InPathFollow._ActiveMoveCorrelationId = InRequest.Get_CorrelationId();

        const auto ArrivalRadius = InRequest.Get_ArrivalRadiusOverrideMode() == ECk_Override::Override
            ? InRequest.Get_ArrivalRadiusOverrideValue()
            : InParams.Get_ArrivalRadius();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ProtectedLeadingWaypointCount = 0;
        InPathFollow._ActiveArrivalRadius = ArrivalRadius;
        InPathFollow._ActiveGoal = Goal;

        // The same-goal walking no-op returned above, so this is a genuinely new movement episode.
        InHandle.Get<FFragment_CrowdAgent_PathTrouble>() = FFragment_CrowdAgent_PathTrouble{};

        // Intentionally do NOT zero _DesiredVelocity: re-targeting mid-walk preserves momentum and
        // the acceleration ramp reconciles direction once the new path resolves. Zeroing made
        // back-to-back MoveTos stop dead and re-accelerate. Stop DOES zero — that is its semantic.

        InHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_GoalFailedHold>();
        InHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        // An external MoveTo starts a NEW episode, so OnGoalBlocked may fire again for the new goal
        // and the strict planning phase gets a fresh attempt.
        DoClearBlockedState(InHandle);
        InPathFollow._StrictPlanFailed = false;

        RequestPathForActiveGoal(InHandle, InParams, InPathFollow);

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] MoveTo {} (arrival={})"),
            InHandle, Goal, ArrivalRadius);
        return ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        Request_NavigationPath(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FVector& InGoal,
            bool InForcePermissivePlan)
        -> void
    {
        // Park the slot at Pending BEFORE enqueueing: OnPathResolved runs after this processor
        // in the same frame and would otherwise consume a previous move's Ready result as if
        // it answered THIS MoveTo, walking the agent down the stale corridor.
        FCk_Nav_Algorithm::MarkPathPending(
            InHandle, InPathFollow.Get_ActiveNavigationRequestRevision());

        auto Request = FCk_Request_Nav_FindPath{InGoal};
        ApplyPlanPhase(InParams, InPathFollow, Request, InForcePermissivePlan);
        Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());
        InPathFollow._PendingEscapePrefix.Reset();
        InPathFollow._ProtectedLeadingWaypointCount = 0;

        // A MoveTo issued while the agent stands inside painted stationary markup would plan
        // "through" the band — see Get_EscapedQueryStart.
        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::IsValid(TransformHandle))
        {
            const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                InHandle,
                InHandle.Get_Entity(),
                UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle),
                InGoal,
                InParams.Get_Radius());
            if (Escaped.IsSet())
            {
                auto EscapePrefix = TArray<FVector>{};
                if (FProcessor_CrowdAgent_PathRefresh::Try_BuildStationaryMarkupEscapePath(
                    InHandle,
                    InHandle.Get_Entity(),
                    UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle),
                    Escaped.GetValue(),
                    InParams,
                    EscapePrefix))
                {
                    Request.Set_StartOverride(ECk_EnableDisable::Enable)
                           .Set_StartOverrideLocation(EscapePrefix.Last());
                    InPathFollow._PendingEscapePrefix = MoveTemp(EscapePrefix);
                }
            }
        }

        // Recorded HERE rather than at the provider fork: five other sites dispatch a CkNavigation
        // query directly (both fallbacks, BlockDetect's stall re-path, PathRefresh, ForceReplan).
        // Recording at the fork alone left the episode still labelled with the provider that just
        // gave up, so the overlay named a sidewalk stall for what was by then an Unreal-nav query.
        InPathFollow._ActiveProvider = ECk_CrowdAgent_PathProvider::Navigation;

        UCk_Utils_Nav_UE::Request_FindPath(InHandle, Request, {});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        ApplyPlanPhase(
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FCk_Request_Nav_FindPath& InOutRequest,
            bool InForcePermissive)
        -> void
    {
        InOutRequest.Set_QueryFilterOverlay(
            UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay());

        if (InForcePermissive)
        {
            InPathFollow._PlanPhase = ECk_CrowdAgent_PlanPhase::Permissive;
            InOutRequest.Set_QueryFilter(InParams.Get_NavQueryFilter());
            return;
        }

        // _StrictPlanFailed is deliberately NOT reset here. Only a dispatch carrying NEW evidence
        // retries strict — a fresh MoveTo, a BlockedRecheck resume (the pack drained), a
        // PathRefresh trigger (a new disc confirmed), a caller ForceReplan — and those sites reset
        // the flag themselves. The stall ladder's re-paths carry no new evidence: retrying strict
        // there re-fails against the same plugged route and doubles every rung's Pending stop-start
        // cycle, which the body visibly tracks (measured as a facing-whip regression).
        const auto StrictWanted =
            (NOT InPathFollow.Get_StrictPlanFailed()) &&
            UCk_Utils_Crowd_Settings_UE::Get_PlanAroundStandingCrowds() ==
                ECk_CrowdPlanAroundStandingCrowdsMode::Enabled &&
            UCk_Utils_Crowd_Settings_UE::Get_StationaryMarkupMode() ==
                ECk_CrowdStationaryMarkupMode::Enabled;

        if (NOT StrictWanted)
        {
            InPathFollow._PlanPhase = ECk_CrowdAgent_PlanPhase::Permissive;
            InOutRequest.Set_QueryFilter(InParams.Get_NavQueryFilter());
            return;
        }

        InPathFollow._PlanPhase = ECk_CrowdAgent_PlanPhase::Strict;

        if (InParams.Get_NavQueryFilterStrict().IsValid())
        {
            InOutRequest.Set_QueryFilter(InParams.Get_NavQueryFilterStrict());
            return;
        }

        InOutRequest.Set_QueryFilterClassOverride(UCk_NavQueryFilter_AvoidStandingCrowds::StaticClass());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        AdvanceNavigationRequestRevision(FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> int32
    {
        InPathFollow._ActiveNavigationRequestRevision =
            InPathFollow._ActiveNavigationRequestRevision == MAX_int32
                ? 1
                : InPathFollow._ActiveNavigationRequestRevision + 1;
        return InPathFollow._ActiveNavigationRequestRevision;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoAbandonActiveProviderQuery(
            HandleType InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> int32
    {
        const auto Revision = AdvanceNavigationRequestRevision(InPathFollow);
        const auto Provider = InPathFollow.Get_ActiveProvider();
        InPathFollow._ActiveProvider = ECk_CrowdAgent_PathProvider::None;

        auto NonConstHandle = InHandle;

        // The shared nav slot is parked by EVERY provider, so it is released for every provider —
        // not only when CkNavigation owned the query. Releasing it is what stops a stopped agent
        // reading as Pending forever.
        UCk_Utils_Nav_UE::Request_AbandonPath(
            NonConstHandle, FCk_Request_Nav_AbandonPath{Revision}, {});

        DoReleaseProviderQuery(InHandle, Provider, Revision);

        return Revision;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoReleaseProviderQuery(
            HandleType InHandle,
            ECk_CrowdAgent_PathProvider InProvider,
            int32 InRevision)
        -> void
    {
        auto NonConstHandle = InHandle;

        switch (InProvider)
        {
            case ECk_CrowdAgent_PathProvider::PathNetwork:
            {
                if (UCk_Utils_PathNetworkFollower_UE::Has(NonConstHandle))
                {
                    auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(NonConstHandle);
                    UCk_Utils_PathNetworkFollower_UE::Request_AbandonRoute(
                        Follower, FCk_Request_PathNetworkFollower_AbandonRoute{InRevision}, {});
                }
                break;
            }
            case ECk_CrowdAgent_PathProvider::VoxelNav:
            {
                if (UCk_Utils_VoxelNavPath_UE::Has(NonConstHandle))
                {
                    auto Path = UCk_Utils_VoxelNavPath_UE::CastChecked(NonConstHandle);
                    UCk_Utils_VoxelNavPath_UE::Request_AbandonPath(
                        Path, FCk_Request_VoxelNavPath_AbandonPath{}, {});
                }
                break;
            }
            case ECk_CrowdAgent_PathProvider::Navigation:
            case ECk_CrowdAgent_PathProvider::None:
            default:
            {
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        RequestPathForActiveGoal(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        const auto Goal = InPathFollow.Get_ActiveGoal();

        // Every new episode begins by ending the previous one: the revision advances (so an
        // already-queued result cannot replace this newer route) AND the provider that owned the
        // last query is released, rather than being left to compute an answer nobody will consume.
        DoAbandonActiveProviderQuery(InHandle, InPathFollow);

        const auto HasValidVoxelVolume = UCk_Utils_VoxelNavPath_UE::Has(InHandle)
            && ck::IsValid(InHandle.Get<FFragment_VoxelNavPath_Params>().Get_Volume());
        const auto IsActiveVoxelProvider = HasValidVoxelVolume
            && (InHandle.Has<FFragment_CrowdAgent_InstalledVoxelPath>()
                || (InHandle.Has<FTag_CrowdAgent_PathPending>()
                    && NOT InHandle.Has<FTag_CrowdAgent_VoxelPathFallbackPending>()));
        if (IsActiveVoxelProvider)
        {
            FCk_Nav_Algorithm::MarkPathPending(
                InHandle, InPathFollow.Get_ActiveNavigationRequestRevision());
            InHandle.Try_Remove<FFragment_CrowdAgent_InstalledVoxelPath>();

            InPathFollow._ActiveProvider = ECk_CrowdAgent_PathProvider::VoxelNav;

            auto Path = UCk_Utils_VoxelNavPath_UE::CastChecked(InHandle);
            const auto From = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
                UCk_Utils_Transform_UE::CastChecked(InHandle));
            UCk_Utils_VoxelNavPath_UE::Request_FindPath(
                Path,
                FCk_Request_VoxelNavPath_FindPath{
                    InHandle.Get<FFragment_VoxelNavPath_Params>().Get_Volume(), From, Goal},
                {});
            return;
        }

        if (UCk_Utils_PathNetworkFollower_UE::Has(InHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(
                InHandle, InPathFollow.Get_ActiveNavigationRequestRevision());
            InHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            InPathFollow._ActiveProvider = ECk_CrowdAgent_PathProvider::PathNetwork;

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(InHandle);
            auto Request = FCk_Request_PathNetworkFollower_FindRoute{Goal};
            Request.Set_NavQueryFilter(InParams.Get_NavQueryFilter());
            Request.Set_QueryFilterOverlay(
                UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay());
            ApplyMarkupEscapeStart(InHandle, InParams, Goal, Request);
            Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower, Request, {});
            return;
        }

        Request_NavigationPath(InHandle, InParams, InPathFollow, Goal);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_FollowTarget& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& TargetPoint = InRequest.Get_TargetPoint();
        if (ck::Is_NOT_Valid(TargetPoint))
        {
            ck::crowd::Warning(TEXT("CrowdAgent [{}] FollowTarget ignored — the target point handle is invalid"),
                InHandle);
            return ECk_Request_OperationResult::Failed;
        }
        const auto LiveGoal = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TargetPoint);

        // Delegate the path-begin to the MoveTo handler with the goal resolved LIVE...
        auto MoveTo = FCk_Request_CrowdAgent_MoveTo{LiveGoal};
        MoveTo.Set_ArrivalRadiusOverrideMode(InRequest.Get_ArrivalRadiusOverrideMode());
        MoveTo.Set_ArrivalRadiusOverrideValue(InRequest.Get_ArrivalRadiusOverrideValue());
        const auto MoveResult = DoHandleRequest(
            InHandle, InParams, InPathFollow, InDesired, MoveTo);
        if (MoveResult != ECk_Request_OperationResult::Succeeded)
        { return MoveResult; }

        // ...then arm the follow AFTER the delegation, because a plain MoveTo clears it.
        auto& Follow = InHandle.AddOrGet<FFragment_CrowdAgent_FollowTarget>();
        Follow._Request = InRequest;
        Follow._RepathAccumulatorSec = 0.0f;

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] FollowTarget (goal={})"), InHandle, LiveGoal);
        return ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_Stop& InRequest)
        -> void
    {
        InDesired._Velocity = FVector::ZeroVector;
        // AccelClamp reconstructs this frame's output from _LastVelocity. Clear its baseline too,
        // otherwise a Stop followed by a MoveTo in the same request batch inherits the old heading
        // and can arc away from the new goal despite Stop's immediate-halt contract.
        InDesired._LastVelocity = FVector::ZeroVector;
        InPathFollow._WaypointIndex = 0;
        InPathFollow._ProtectedLeadingWaypointCount = 0;
        InHandle.Get<FFragment_CrowdAgent_PathTrouble>() = FFragment_CrowdAgent_PathTrouble{};

        // Stop is a terminal for the movement episode, so it releases what the episode acquired.
        // Without this the shared nav-path slot keeps the Pending its dispatch parked there: the
        // provider result that lands afterwards is refused by OnRouteResolved's tag gate (which
        // the tag removals below have just closed) and nothing else ever writes it, so every
        // Get_PathStatus consumer is told a query is in flight for the entity's whole life.
        DoAbandonActiveProviderQuery(InHandle, InPathFollow);
        InPathFollow._ActiveGoal = FVector::ZeroVector;

        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_GoalFailedHold>();
        InHandle.AddOrGet<FTag_CrowdAgent_Idle>();

        // Stop abandons the goal entirely — BlockedRecheck must never resume it.
        DoClearBlockedState(InHandle);

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] Stop"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoClearBlockedState(
            FCk_Handle_CrowdAgent& InAgent)
        -> void
    {
        InAgent.Try_Remove<FTag_CrowdAgent_GoalBlocked>();
        InAgent.Try_Remove<FTag_CrowdAgent_GoalFailedHold>();

        if (NOT InAgent.Has<FFragment_CrowdAgent_BlockDetect>())
        { return; }

        auto& BlockDetect = InAgent.AddOrGet<FFragment_CrowdAgent_BlockDetect>();
        BlockDetect._BlockedBy = FCk_Handle{};
        BlockDetect._BlockedCause = ECk_CrowdAgent_BlockedReason::GoalOccupied;
        BlockDetect._RecheckAccumulatorSec = 0.0f;
        BlockDetect._BlockedSignalSent = false;
        BlockDetect._StallRepathCount = 0;
        BlockDetect._BlockedRetryCount = 0;
        BlockDetect._CrowdedGoalDepth = 0;
        BlockDetect.DoResetProgressWindow();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoForceReplan(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity)
        -> void
    {
        const auto HasActiveGoal = InHandle.Has<FTag_CrowdAgent_Walking>()
            || InHandle.Has<FTag_CrowdAgent_PathPending>()
            || InHandle.Has<FTag_CrowdAgent_GoalBlocked>();
        if (NOT HasActiveGoal)
        { return; }

        // A caller force-replan says the world changed — the strict phase gets a fresh attempt.
        InPathFollow._StrictPlanFailed = false;

        // Query filters are a Recast policy. A Voxel route cannot apply one,
        // and replacing its in-flight job without a provider-owned revision
        // would create a false stale-result guarantee. Persist the policy for
        // future Recast fallback but leave the current volumetric route intact.
        if (UCk_Utils_VoxelNavPath_UE::Has(InHandle)
            && ck::IsValid(InHandle.Get<FFragment_VoxelNavPath_Params>().Get_Volume()))
        {
            ck::crowd::Verbose(TEXT("CrowdAgent [{}] stored nav-query policy without replanning its Voxel route"),
                InHandle);
            return;
        }

        InHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
        InHandle.AddOrGet<FTag_CrowdAgent_PathPending>();
        InHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();
        InHandle.Try_Remove<FFragment_CrowdAgent_InstalledVoxelPath>();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ActivePathEndsShortOfGoal = false;
        DoClearBlockedState(InHandle);

        // Preserve _ActiveGoal, _ActiveArrivalRadius, move episode/correlation, FollowTarget and
        // desired velocity. This is a route replacement, not a terminal movement transition.
        RequestPathForActiveGoal(InHandle, InParams, InPathFollow);
        ck::crowd::Verbose(TEXT("CrowdAgent [{}] nav-query policy forced replan to {}"),
            InHandle, InPathFollow.Get_ActiveGoal());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetNavQueryFilter& InRequest)
        -> void
    {
        // The batch prepass wrote the last policy before any movement command was dispatched.
        // Keep an overload so this request participates in ordinary completion/cancellation flow.
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetMaxSpeed& InRequest)
        -> void
    {
        InParams._MaxSpeed = InRequest.Get_MaxSpeed();

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] SetMaxSpeed {}"), InHandle, InRequest.Get_MaxSpeed());
    }

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale& InRequest)
        -> void
    {
        auto& PersonalSpace = InHandle.AddOrGet<FFragment_CrowdAgent_TransientPersonalSpace>();
        PersonalSpace._Scale = InRequest.Get_Scale();
        PersonalSpace._RemainingSeconds = InRequest.Get_DurationSeconds();
        ck::crowd::Verbose(TEXT("CrowdAgent [{}] transient personal-space scale {} for {}s"), InHandle, PersonalSpace._Scale, PersonalSpace._RemainingSeconds);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_MoveRequests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
