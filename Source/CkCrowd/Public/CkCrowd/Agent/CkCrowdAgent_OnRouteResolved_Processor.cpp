#include "CkCrowdAgent_OnRouteResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnRouteResolved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::OnRouteResolved"), STAT_CkCrowd_OnRouteResolvedProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_OnRouteResolved::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_OnRouteResolvedProc);

        const auto& Result = InCorridor.Get_Result();

        // Only agents with an active goal consume corridors; Idle/Failed agents have nothing the
        // corridor could answer (the fragment persists after arrival/stop).
        const auto IsPathPending = InHandle.Has<FTag_CrowdAgent_PathPending>();
        const auto IsWalking     = InHandle.Has<FTag_CrowdAgent_Walking>();

        if (NOT IsPathPending && NOT IsWalking)
        { return; }

        // Only consume a result that answers the goal THIS MoveTo asked for. A leftover result for
        // the previous goal (the follower processor hasn't drained the fresh FindRoute yet — its
        // status was parked at Pending by Request_FindRoute, but the goal is still the old one)
        // must not transition the agent.
        constexpr auto GoalMatchEpsilonCm = 25.0f;
        if (FVector::Dist(Result.Get_GoalLocation(), InPathFollow.Get_ActiveGoal()) > GoalMatchEpsilonCm)
        { return; }

        switch (Result.Get_Status())
        {
            case ECk_PathNetwork_RouteStatus::Ready:
            {
                // Skip corridors already installed: goal pins the MoveTo, epoch pins the network
                // build. A rebuild replans the same goal at a new epoch and lands here again —
                // that's the mid-walk corridor swap.
                if (InHandle.Has<FFragment_CrowdAgent_InstalledRoute>())
                {
                    const auto& Installed = InHandle.Get<FFragment_CrowdAgent_InstalledRoute>();

                    if (Installed.Get_NetworkEpoch() == InCorridor.Get_NetworkEpoch() &&
                        FVector::Dist(Installed.Get_GoalLocation(), Result.Get_GoalLocation()) <= GoalMatchEpsilonCm)
                    { return; }
                }

                // Install the corridor as the agent's nav path. For PathPending agents,
                // OnPathResolved's poll sees the Ready result and performs the → Walking
                // transition; for Walking agents (rebuild swap) steering just reads the fresh
                // waypoints — either way the corridor is indistinguishable from a navmesh path.
                auto NonConstHandle = InHandle;
                FCk_Nav_Algorithm::InstallExternalPath(
                    NonConstHandle, Result.Get_CompiledWaypoints(), Result.Get_GoalLocation());

                // Fresh polyline, fresh cursor — for the mid-walk swap the old index may point
                // past (or into the wrong stretch of) the new waypoint array.
                InPathFollow._WaypointIndex = 0;

                // Anchor the FIRST corridor segment for Steering's plane-crossing retirement. The
                // corridor's leading waypoint has no predecessor in the array, so the incoming
                // direction comes from where the agent actually IS at install time. This covers the
                // mid-walk rebuild swap too: the agent may be deep into the old corridor, and the
                // new segment starts from there, not from some stale origin.
                auto TransformHandle = UCk_Utils_Transform_UE::Cast(NonConstHandle);
                const auto& CompiledWaypoints = Result.Get_CompiledWaypoints();
                InPathFollow._CurrentSegmentStart = ck::IsValid(TransformHandle)
                    ? UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle)
                    : (CompiledWaypoints.Num() > 0 ? CompiledWaypoints[0] : FVector::ZeroVector);

                auto& Installed = NonConstHandle.AddOrGet<FFragment_CrowdAgent_InstalledRoute>();
                Installed._GoalLocation = Result.Get_GoalLocation();
                Installed._NetworkEpoch = InCorridor.Get_NetworkEpoch();

                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] network route ready ({} wps, cost={}, epoch={}) — installed as nav path"),
                    InHandle, Result.Get_CompiledWaypoints().Num(), Result.Get_TotalCost(),
                    InCorridor.Get_NetworkEpoch());
                break;
            }
            case ECk_PathNetwork_RouteStatus::Failed:
            {
                // Failed while WALKING = a rebuild replan came back unroutable; keep walking the
                // already-installed world-space waypoints rather than hard-stopping mid-street.
                if (NOT IsPathPending)
                { break; }

                auto NonConstHandle = InHandle;
                NonConstHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();

                UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                    NonConstHandle,
                    MakePayload(NonConstHandle));

                ck::crowd::Warning(TEXT("CrowdAgent [{}] network route failed ({}) — PathPending → Idle"),
                    InHandle, Result.Get_FailReason());
                break;
            }
            case ECk_PathNetwork_RouteStatus::None:
            case ECk_PathNetwork_RouteStatus::Pending:
            default:
                // Route still being planned — no-op until the follower processor completes it.
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
