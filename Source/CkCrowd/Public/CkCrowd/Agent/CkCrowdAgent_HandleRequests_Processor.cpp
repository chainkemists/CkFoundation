#include "CkCrowdAgent_HandleRequests_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_HandleRequests);

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
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_MoveRequests& InRequests) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_HandleRequestsProc);

        InHandle.CopyAndRemove(InRequests, [&](const auto& InSnapshot)
        {
            algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InPathFollow, InDesired, InRequest);
            }), policy::DontResetContainer{});
        });
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
        -> void
    {
        // A plain MoveTo takes over from any follow in flight — the caller gave a fixed goal.
        // (The FollowTarget handler delegates here and re-adds its state after.)
        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();

        const auto Goal = InRequest.Get_Target();

        // Same-goal no-op: if a fresh MoveTo lands on (nearly) the goal we're already walking to,
        // ignore it. Re-issuing the same goal resets the waypoint cursor to 0 and preserves stale
        // momentum; a noisy re-issuer (e.g. a state machine that re-fires MoveTo several times a
        // second) thereby prevents the final-stop from ever latching, and the agent orbits its goal.
        // A genuinely different target (beyond the epsilon) re-paths as normal.
        constexpr auto SameGoalEpsilonCm = 20.0f;
        if (InHandle.Has<FTag_CrowdAgent_Walking>() &&
            FVector::Dist(Goal, InPathFollow.Get_ActiveGoal()) <= SameGoalEpsilonCm)
        {
            ck::crowd::Verbose(TEXT("CrowdAgent [{}] MoveTo {} ignored (same goal, already walking)"),
                InHandle, Goal);
            return;
        }

        // Pick the active arrival radius — per-request override wins, otherwise fall back to the
        // params default. Steering reads this each frame for its "stop near final waypoint" check.
        const auto ArrivalRadius = InRequest.Get_ArrivalRadiusOverrideMode() == ECk_Override::Override
            ? InRequest.Get_ArrivalRadiusOverrideValue()
            : InParams.Get_ArrivalRadius();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ActiveArrivalRadius = ArrivalRadius;
        InPathFollow._ActiveGoal = Goal;

        // Intentionally do NOT zero _DesiredVelocity here. Re-targeting mid-walk should preserve
        // the agent's current momentum; once the new path resolves and steering's view fires
        // again, it'll reconcile direction toward the new target through the per-frame
        // acceleration ramp. Zeroing here caused a hard "stop and re-accelerate from zero" when
        // MoveTo was issued back-to-back. Stop, by contrast, DOES zero — that's its intended
        // semantic (see DoHandleRequest for FCk_Request_CrowdAgent_Stop below).

        // State transition: Idle/Walking → PathPending. Use Try_Remove because the agent may not
        // currently hold either tag (e.g. PathPending re-fired before the previous resolved).
        InHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        // An external MoveTo starts a NEW episode: whatever was blocking the previous goal is no longer
        // this agent's problem, and OnGoalBlocked must be allowed to fire again for the new one.
        DoClearBlockedState(InHandle);

        // Followers route through the path network instead of straight CkNavigation: the corridor
        // resolves via FProcessor_CrowdAgent_OnRouteResolved, which installs the compiled waypoints
        // through the same FFragment_Nav_PathResult seam — everything downstream (OnPathResolved,
        // steering) is provider-agnostic. Without the follower feature, behavior is exactly as before.
        if (UCk_Utils_PathNetworkFollower_UE::Has(InHandle))
        {
            // Park the nav-path slot at Pending and forget the previously-installed corridor:
            // no nav request exists to do the former (OnPathResolved would otherwise consume the
            // previous Ready path as if it answered THIS MoveTo), and the corridor fragment
            // persists across MoveTos so the bridge needs the install-identity reset.
            FCk_Nav_Algorithm::MarkPathPending(InHandle);
            InHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(InHandle);
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower,
                FCk_Request_PathNetworkFollower_FindRoute{Goal});
        }
        else
        {
            // Park the nav-path slot at Pending BEFORE enqueueing: OnPathResolved runs after this
            // processor in the same frame, and a previous move's Ready result would otherwise be
            // consumed as if it answered THIS MoveTo — the agent walks the stale corridor to the
            // OLD goal and the fresh result is never installed (same race the follower branch
            // guards against above).
            FCk_Nav_Algorithm::MarkPathPending(InHandle);

            // Enqueue the path request on the agent's own entity. CkNavigation's processor drains it
            // and writes FFragment_Nav_PathResult on the same entity; OnPathResolved sees the result
            // and finalizes the state transition.
            auto Request = FCk_Request_Nav_FindPath{Goal};
            Request.Set_QueryFilter(InParams.Get_NavQueryFilter());

            // Fresh plans are not exempt from the inside-a-cost-band trap: a MoveTo issued while
            // the agent stands inside painted stationary markup (e.g. a gameplay re-target as the
            // queue it pressed into shifts) would otherwise pick "through" — see Get_EscapedQueryStart.
            auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
            if (ck::IsValid(TransformHandle))
            {
                const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                    InHandle,
                    InHandle.Get_Entity(),
                    UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle),
                    InRequest.Get_Target(),
                    InParams.Get_Radius());
                if (Escaped.IsSet())
                {
                    Request.Set_StartOverride(ECk_EnableDisable::Enable)
                           .Set_StartOverrideLocation(*Escaped);
                }
            }

            UCk_Utils_Nav_UE::Request_FindPath(InHandle, Request);
        }

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] MoveTo {} (arrival={})"),
            InHandle, Goal, ArrivalRadius);
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
        -> void
    {
        const auto& TargetPoint = InRequest.Get_TargetPoint();
        if (ck::Is_NOT_Valid(TargetPoint))
        {
            ck::crowd::Warning(TEXT("CrowdAgent [{}] FollowTarget ignored — the target point handle is invalid"),
                InHandle);
            return;
        }
        const auto LiveGoal = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TargetPoint);

        // Delegate the path-begin to the MoveTo handler (same-goal epsilon, arrival override,
        // state transition, blocked reset, path-network routing) with the goal resolved LIVE...
        auto MoveTo = FCk_Request_CrowdAgent_MoveTo{LiveGoal};
        MoveTo.Set_ArrivalRadiusOverrideMode(InRequest.Get_ArrivalRadiusOverrideMode());
        MoveTo.Set_ArrivalRadiusOverrideValue(InRequest.Get_ArrivalRadiusOverrideValue());
        DoHandleRequest(InHandle, InParams, InPathFollow, InDesired, MoveTo);

        // ...then arm/refresh the follow AFTER the delegation (a plain MoveTo clears it). The
        // FollowTarget processor re-paths toward the live point from here on.
        auto& Follow = InHandle.AddOrGet<FFragment_CrowdAgent_FollowTarget>();
        Follow._Request = InRequest;
        Follow._RepathAccumulatorSec = 0.0f;

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] FollowTarget (goal={})"), InHandle, LiveGoal);
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
        InPathFollow._WaypointIndex = 0;

        // Stop abandons any follow — the caller explicitly gave up the goal.
        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
        InHandle.AddOrGet<FTag_CrowdAgent_Idle>();

        // Stop abandons the goal entirely — the agent must not be resumed by BlockedRecheck later.
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

        if (NOT InAgent.Has<FFragment_CrowdAgent_BlockDetect>())
        { return; }

        auto& BlockDetect = InAgent.AddOrGet<FFragment_CrowdAgent_BlockDetect>();
        BlockDetect._BlockedBy = FCk_Handle{};
        BlockDetect._FeetSamples.Reset();
        BlockDetect._NextSampleIdx = 0;
        BlockDetect._SampleAccumulatorSec = 0.0f;
        BlockDetect._RecheckAccumulatorSec = 0.0f;
        BlockDetect._BlockedSignalSent = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
