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
            FFragment_CrowdAgent_Params& InParams,
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
        // A plain MoveTo takes over from any follow in flight; the FollowTarget handler delegates
        // here and re-adds its own state afterwards.
        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();

        const auto Goal = InRequest.Get_Target();

        // Re-issuing the goal we are already walking to resets the waypoint cursor, so a noisy
        // re-issuer would stop the final-stop ever latching and the agent would orbit its goal.
        constexpr auto SameGoalEpsilonCm = 20.0f;
        if (InHandle.Has<FTag_CrowdAgent_Walking>() &&
            FVector::Dist(Goal, InPathFollow.Get_ActiveGoal()) <= SameGoalEpsilonCm)
        {
            ck::crowd::Verbose(TEXT("CrowdAgent [{}] MoveTo {} ignored (same goal, already walking)"),
                InHandle, Goal);
            return;
        }

        const auto ArrivalRadius = InRequest.Get_ArrivalRadiusOverrideMode() == ECk_Override::Override
            ? InRequest.Get_ArrivalRadiusOverrideValue()
            : InParams.Get_ArrivalRadius();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ActiveArrivalRadius = ArrivalRadius;
        InPathFollow._ActiveGoal = Goal;

        // Intentionally do NOT zero _DesiredVelocity: re-targeting mid-walk preserves momentum and
        // the acceleration ramp reconciles direction once the new path resolves. Zeroing made
        // back-to-back MoveTos stop dead and re-accelerate. Stop DOES zero — that is its semantic.

        InHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        // An external MoveTo starts a NEW episode, so OnGoalBlocked may fire again for the new goal.
        DoClearBlockedState(InHandle);

        // Followers route through the path network, which installs its compiled waypoints through
        // the same FFragment_Nav_PathResult seam — everything downstream stays provider-agnostic.
        if (UCk_Utils_PathNetworkFollower_UE::Has(InHandle))
        {
            // Park the slot at Pending and forget the installed corridor: no nav request exists to
            // do the former, and the corridor fragment persists across MoveTos.
            FCk_Nav_Algorithm::MarkPathPending(InHandle);
            InHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(InHandle);
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower,
                FCk_Request_PathNetworkFollower_FindRoute{Goal});
        }
        else
        {
            // Park the slot at Pending BEFORE enqueueing: OnPathResolved runs after this processor
            // in the same frame and would otherwise consume a previous move's Ready result as if
            // it answered THIS MoveTo, walking the agent down the stale corridor.
            FCk_Nav_Algorithm::MarkPathPending(InHandle);

            auto Request = FCk_Request_Nav_FindPath{Goal};
            Request.Set_QueryFilter(InParams.Get_NavQueryFilter());

            // A MoveTo issued while the agent stands inside painted stationary markup would plan
            // "through" the band — see Get_EscapedQueryStart.
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

        // Delegate the path-begin to the MoveTo handler with the goal resolved LIVE...
        auto MoveTo = FCk_Request_CrowdAgent_MoveTo{LiveGoal};
        MoveTo.Set_ArrivalRadiusOverrideMode(InRequest.Get_ArrivalRadiusOverrideMode());
        MoveTo.Set_ArrivalRadiusOverrideValue(InRequest.Get_ArrivalRadiusOverrideValue());
        DoHandleRequest(InHandle, InParams, InPathFollow, InDesired, MoveTo);

        // ...then arm the follow AFTER the delegation, because a plain MoveTo clears it.
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

        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
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
}

// --------------------------------------------------------------------------------------------------------------------
