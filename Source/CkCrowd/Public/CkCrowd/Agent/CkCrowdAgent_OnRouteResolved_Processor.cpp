#include "CkCrowdAgent_OnRouteResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathFollow_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "HAL/PlatformTime.h"

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
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_PathTrouble& InPathTrouble)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_OnRouteResolvedProc);

        const auto& Result = InCorridor.Get_Result();

        // The corridor fragment persists after arrival/stop, so only an agent with an active goal
        // may consume it.
        const auto IsPathPending = InHandle.Has<FTag_CrowdAgent_PathPending>();
        const auto IsWalking     = InHandle.Has<FTag_CrowdAgent_Walking>();

        if (NOT IsPathPending && NOT IsWalking)
        { return; }

        // A result left over from the previous goal (the follower has not drained the fresh
        // FindRoute yet) must never transition the agent.
        constexpr auto GoalMatchEpsilonCm = 25.0f;
        if (FVector::Dist(Result.Get_GoalLocation(), InPathFollow.Get_ActiveGoal()) > GoalMatchEpsilonCm)
        { return; }

        switch (Result.Get_Status())
        {
            case ECk_PathNetwork_RouteStatus::Ready:
            {
                if (InHandle.Has<FFragment_CrowdAgent_InstalledRoute>())
                {
                    const auto& Installed = InHandle.Get<FFragment_CrowdAgent_InstalledRoute>();

                    if (Installed.Get_NetworkEpoch() == InCorridor.Get_NetworkEpoch() &&
                        Installed.Get_TuningRevision() == Result.Get_TuningRevision() &&
                        FVector::Dist(Installed.Get_GoalLocation(), Result.Get_GoalLocation()) <= GoalMatchEpsilonCm)
                    { return; }
                }

                auto NonConstHandle = InHandle;
                auto WaypointsToInstall = Result.Get_CompiledWaypoints();
                auto DetouredWaypoints = TArray<FVector>{};
                const auto UsedStationaryMarkupDetour =
                    FProcessor_CrowdAgent_PathRefresh::
                    Try_BuildStationaryMarkupDetour(
                        NonConstHandle,
                        InHandle.Get_Entity(),
                        InTransform.Get_Transform().GetLocation(),
                        Result.Get_GoalLocation(),
                        InParams,
                        InPathFollow.Get_ActiveArrivalRadius(),
                        WaypointsToInstall,
                        DetouredWaypoints);
                if (UsedStationaryMarkupDetour)
                { WaypointsToInstall = MoveTemp(DetouredWaypoints); }

                FCk_Nav_Algorithm::InstallExternalPath(
                    NonConstHandle,
                    MoveTemp(WaypointsToInstall),
                    Result.Get_GoalLocation());
                const auto& InstalledWaypoints =
                    NonConstHandle.Get<FFragment_Nav_PathResult>().Get_Waypoints();

                // Fresh polyline, fresh cursor — on a mid-walk swap the old index may point past
                // the new waypoint array.
                InPathFollow._WaypointIndex = 0;

                // The corridor's leading waypoint has no predecessor, so the incoming direction for
                // Steering's plane-crossing retirement comes from where the agent IS at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // PathPending installs are normalized by OnPathResolved before they begin walking.
                // A rebuild swap is already Walking, so it bypasses that processor: retire any
                // request-time prefix that is behind the agent at install time here. Otherwise the
                // reset cursor aims back at an obsolete lateral projection before turning forward.
                if (IsWalking)
                {
                    const auto SkippedWaypointCount =
                        ck_crowd_agent_path_follow_algorithm::SkipAlreadyPassedLeadingWaypoints(
                            InPathFollow.Get_CurrentSegmentStart(),
                            InstalledWaypoints,
                            InPathFollow._WaypointIndex,
                            InPathFollow._CurrentSegmentStart);

                    if (SkippedWaypointCount > 0)
                    {
                        ck::crowd::Verbose(
                            TEXT("CrowdAgent [{}] network route swap skipped {} already-passed "
                                 "leading corner(s)"),
                            InHandle,
                            SkippedWaypointCount);
                    }
                }

                // This corridor was spliced against every disc confirmed on Recast up to now. A
                // painted disc still awaiting its async tile rebuild receives a newer serial on
                // confirmation and invalidates this route instead of being silently consumed.
                InPathFollow._PathSerial =
                    FProcessor_CrowdAgent_PathRefresh::Get_CurrentConfirmationSerial();

                auto& Installed = NonConstHandle.AddOrGet<FFragment_CrowdAgent_InstalledRoute>();
                Installed._GoalLocation = Result.Get_GoalLocation();
                Installed._NetworkEpoch = InCorridor.Get_NetworkEpoch();
                Installed._TuningRevision = Result.Get_TuningRevision();

                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] network route ready ({} raw wps, {} installed wps, "
                         "stationary detour={}, cost={}, epoch={}) — installed as nav path"),
                    InHandle,
                    Result.Get_CompiledWaypoints().Num(),
                    InstalledWaypoints.Num(),
                    UsedStationaryMarkupDetour,
                    Result.Get_TotalCost(),
                    InCorridor.Get_NetworkEpoch());
                break;
            }
            case ECk_PathNetwork_RouteStatus::Failed:
            {
                // Failed while WALKING = a rebuild replan came back unroutable; keep walking the
                // already-installed waypoints rather than hard-stopping mid-street.
                if (NOT IsPathPending)
                { break; }

                // The failed corridor persists. Without this guard the fallback would enqueue a
                // fresh nav request every frame until OnPathResolved consumed one of them.
                if (InHandle.Has<FTag_CrowdAgent_PathNetworkFallbackPending>())
                { break; }

                InPathTrouble._AgentLocation = InTransform.Get_Transform().GetLocation();
                InPathTrouble._GoalLocation = InPathFollow.Get_ActiveGoal();
                InPathTrouble._EventTimeSeconds = FPlatformTime::Seconds();
                InPathTrouble._PathNetworkFailReason = Result.Get_FailReason();
                InPathTrouble._NavigationStatus = ECk_Nav_PathStatus::Pending;
                InPathTrouble._NavigationFailReason = ECk_Nav_PathFailReason::None;
                InPathTrouble._HasEvent = true;
                InPathTrouble._HadPathNetworkFailure = true;
                InPathTrouble._UsedNavigationFallback = true;

                auto NonConstHandle = InHandle;
                NonConstHandle.AddOrGet<FTag_CrowdAgent_PathNetworkFallbackPending>();
                FProcessor_CrowdAgent_HandleRequests::Request_NavigationPath(
                    NonConstHandle,
                    InParams,
                    InPathFollow.Get_ActiveGoal());

                ck::crowd::Display(
                    TEXT("CrowdAgent [{}] network route failed ({}) — trying CkNavigation fallback"),
                    InHandle,
                    Result.Get_FailReason());
                break;
            }
            case ECk_PathNetwork_RouteStatus::None:
            case ECk_PathNetwork_RouteStatus::Pending:
            default:
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
