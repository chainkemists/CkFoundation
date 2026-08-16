#include "CkCrowdAgent_OnRouteResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathFollow_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "HAL/PlatformTime.h"

#include "NavigationSystem.h"
#include "NavigationData.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnRouteResolved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::OnRouteResolved"), STAT_CkCrowd_OnRouteResolvedProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_on_route_resolved_processor
{
    // Same NavData the navmesh clamp walks against, so the install-time skip and the clamp agree
    // on what is walkable. Null means no nav data or the gate switched off, and the skip reverts
    // to its projection-only form.
    auto Get_NavDataForChordGate(const FCk_Handle& InAgent) -> ANavigationData*
    {
        if (UCk_Utils_Crowd_Settings_UE::Get_WaypointRetirementLineOfSight() ==
            ECk_CrowdWaypointRetirementLineOfSightMode::Disabled)
        { return nullptr; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAgent);
        const auto NavSys = ck::IsValid(World)
            ? UNavigationSystemV1::GetCurrent(World)
            : static_cast<UNavigationSystemV1*>(nullptr);

        return ck::IsValid(NavSys)
            ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)
            : static_cast<ANavigationData*>(nullptr);
    }
}

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

        // A policy/goal replan can retain the same goal while superseding an already
        // queued path-network request. Do not let an older result alter this episode.
        if (Result.Get_RequestRevision() != InPathFollow.Get_ActiveNavigationRequestRevision())
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
                const auto HasRouteWaypoints = NOT WaypointsToInstall.IsEmpty();
                CK_ENSURE_IF_NOT(
                    HasRouteWaypoints,
                    TEXT("CrowdAgent [{}] received a Ready PathNetwork route with no waypoints"),
                    InHandle)
                {
                    if (IsPathPending &&
                        NOT InHandle.Has<FTag_CrowdAgent_PathNetworkFallbackPending>())
                    {
                        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathNetworkFallbackPending>();
                        InPathFollow._ProtectedLeadingWaypointCount = 0;
                        FProcessor_CrowdAgent_HandleRequests::AdvanceNavigationRequestRevision(InPathFollow);
                        FProcessor_CrowdAgent_HandleRequests::Request_NavigationPath(
                            NonConstHandle,
                            InParams,
                            InPathFollow,
                            InPathFollow.Get_ActiveGoal());
                    }
                    break;
                }

                const auto AgentLocation = InTransform.Get_Transform().GetLocation();
                const auto ActiveGoal = InPathFollow.Get_ActiveGoal();
                const auto EscapedStart =
                    FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                        NonConstHandle,
                        InHandle.Get_Entity(),
                        AgentLocation,
                        ActiveGoal,
                        InParams.Get_Radius());
                auto EscapeWaypoints = TArray<FVector>{};
                const auto UsedNavigableEscapePrefix =
                    EscapedStart.IsSet() &&
                    FProcessor_CrowdAgent_PathRefresh::
                    Try_BuildStationaryMarkupEscapePath(
                        NonConstHandle,
                        InHandle.Get_Entity(),
                        AgentLocation,
                        EscapedStart.GetValue(),
                        InParams,
                        EscapeWaypoints);
                const auto DetourStart = UsedNavigableEscapePrefix
                    ? EscapeWaypoints.Last()
                    : AgentLocation;
                auto DetouredWaypoints = TArray<FVector>{};
                const auto UsedStationaryMarkupDetour =
                    FProcessor_CrowdAgent_PathRefresh::
                    Try_BuildStationaryMarkupDetour(
                        NonConstHandle,
                        InHandle.Get_Entity(),
                        DetourStart,
                        ActiveGoal,
                        InParams,
                        InPathFollow.Get_ActiveArrivalRadius(),
                        WaypointsToInstall,
                        DetouredWaypoints);
                if (UsedStationaryMarkupDetour)
                { WaypointsToInstall = MoveTemp(DetouredWaypoints); }

                InPathFollow._ProtectedLeadingWaypointCount = 0;
                if (UsedNavigableEscapePrefix)
                {
                    constexpr auto MergeDistanceUu = 1.0f;
                    auto CombinedWaypoints = TArray<FVector>{};
                    CombinedWaypoints.Reserve(
                        EscapeWaypoints.Num() + WaypointsToInstall.Num());
                    const auto AppendDistinct = [&](const FVector& InPoint)
                    {
                        if (CombinedWaypoints.IsEmpty() ||
                            FVector::DistSquared(CombinedWaypoints.Last(), InPoint) >
                                FMath::Square(MergeDistanceUu))
                        {
                            CombinedWaypoints.Add(InPoint);
                        }
                    };
                    for (const auto& EscapeWaypoint : EscapeWaypoints)
                    { AppendDistinct(EscapeWaypoint); }

                    // Every point in the navigable egress belongs to the physical escape. Preserve
                    // it from install-time stale-corner normalization until Steering consumes it.
                    InPathFollow._ProtectedLeadingWaypointCount =
                        CombinedWaypoints.Num();
                    for (const auto& RouteWaypoint : WaypointsToInstall)
                    { AppendDistinct(RouteWaypoint); }

                    const auto CombinedPathIsValid =
                        InPathFollow.Get_ProtectedLeadingWaypointCount() > 0 &&
                        CombinedWaypoints.Num() >
                            InPathFollow.Get_ProtectedLeadingWaypointCount();
                    CK_ENSURE_IF_NOT(
                        CombinedPathIsValid,
                        TEXT("CrowdAgent [{}] could not append its PathNetwork route after a "
                             "stationary-markup escape prefix"),
                        InHandle)
                    {
                        InPathFollow._ProtectedLeadingWaypointCount = 0;
                    }
                    else
                    { WaypointsToInstall = MoveTemp(CombinedWaypoints); }
                }

                FCk_Nav_Algorithm::InstallExternalPath(
                    NonConstHandle,
                    MoveTemp(WaypointsToInstall),
                    Result.Get_GoalLocation(),
                    InPathFollow.Get_ActiveNavigationRequestRevision());
                const auto& InstalledWaypoints =
                    NonConstHandle.Get<FFragment_Nav_PathResult>().Get_Waypoints();

                // Fresh polyline, fresh cursor — on a mid-walk swap the old index may point past
                // the new waypoint array.
                InPathFollow._WaypointIndex = 0;

                // The corridor's leading waypoint has no predecessor, so the incoming direction for
                // Steering's plane-crossing retirement comes from where the agent IS at install time.
                InPathFollow._CurrentSegmentStart = AgentLocation;

                // PathPending installs are normalized by OnPathResolved before they begin walking.
                // A rebuild swap is already Walking, so it bypasses that processor: retire any
                // request-time prefix that is behind the agent at install time here. Otherwise the
                // reset cursor aims back at an obsolete lateral projection before turning forward.
                if (IsWalking)
                {
                    // The projection test is laterally blind: a corner BESIDE the agent reads as
                    // passed while the chord onward from where it stands cuts the geometry the
                    // route was spliced around, which installs an aim the navmesh clamp then eats.
                    const auto NavDataForGate =
                        ck_crowd_agent_on_route_resolved_processor::Get_NavDataForChordGate(InHandle);

                    auto IsChordNavigable = [&](const FVector& InFrom, const FVector& InTo) -> bool
                    {
                        if (ck::Is_NOT_Valid(NavDataForGate))
                        { return true; }

                        auto HitLocation = FVector::ZeroVector;
                        return NOT NavDataForGate->Raycast(InFrom, InTo, HitLocation, FSharedConstNavQueryFilter{});
                    };

                    const auto SkippedWaypointCount =
                        ck_crowd_agent_path_follow_algorithm::SkipAlreadyPassedLeadingWaypoints(
                            InPathFollow.Get_CurrentSegmentStart(),
                            InstalledWaypoints,
                            InPathFollow._WaypointIndex,
                            InPathFollow._CurrentSegmentStart,
                            InPathFollow.Get_ProtectedLeadingWaypointCount(),
                            IsChordNavigable);

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
                         "stationary detour={}, escape prefix={}, cost={}, epoch={}) — installed as nav path"),
                    InHandle,
                    Result.Get_CompiledWaypoints().Num(),
                    InstalledWaypoints.Num(),
                    UsedStationaryMarkupDetour,
                    UsedNavigableEscapePrefix,
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
                InPathFollow._ProtectedLeadingWaypointCount = 0;
                FProcessor_CrowdAgent_HandleRequests::AdvanceNavigationRequestRevision(InPathFollow);
                FProcessor_CrowdAgent_HandleRequests::Request_NavigationPath(
                    NonConstHandle,
                    InParams,
                    InPathFollow,
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
