#include "CkCrowdAgent_OnRouteResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_StationaryMarkup_Processor.h"

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
            const FFragment_Transform& InTransform,
            const FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
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

                    const auto IsCorridorAlreadyInstalled =
                        Installed.Get_NetworkEpoch() == InCorridor.Get_NetworkEpoch() &&
                        FVector::Dist(Installed.Get_GoalLocation(), Result.Get_GoalLocation()) <= GoalMatchEpsilonCm;

                    if (IsCorridorAlreadyInstalled)
                    { return; }
                }

                auto NonConstHandle = InHandle;
                FCk_Nav_Algorithm::InstallExternalPath(
                    NonConstHandle, Result.Get_CompiledWaypoints(), Result.Get_GoalLocation());

                // Fresh polyline, fresh cursor — on a mid-walk swap the old index may point past
                // the new waypoint array.
                InPathFollow._WaypointIndex = 0;

                // The corridor's leading waypoint has no predecessor, so the incoming direction for
                // Steering's plane-crossing retirement comes from where the agent IS at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // Planned against every disc painted up to now — only NEWER discs may trigger a
                // PathRefresh re-path.
                InPathFollow._PathSerial = FProcessor_CrowdAgent_StationaryMarkup::Get_CurrentPaintSerial();

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
                // already-installed waypoints rather than hard-stopping mid-street.
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
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
