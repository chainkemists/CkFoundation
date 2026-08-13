#include "CkCrowdAgent_OnPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathFollow_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/PlatformTime.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnPathResolved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::OnPathResolved"), STAT_CkCrowd_OnPathResolvedProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_OnPathResolved::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_PathTrouble& InPathTrouble)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_OnPathResolvedProc);

        const auto IsPathNetworkFallback =
            InHandle.Has<FTag_CrowdAgent_PathNetworkFallbackPending>();
        const auto IsTerminalPathResult =
            InPathResult.Get_Status() == ECk_Nav_PathStatus::Ready
            || InPathResult.Get_Status() == ECk_Nav_PathStatus::Partial
            || InPathResult.Get_Status() == ECk_Nav_PathStatus::Failed;
        const auto IsStaleNavigationResult = InPathResult.Get_RequestRevision() != 0
            && InPathResult.Get_RequestRevision() != InPathFollow.Get_ActiveNavigationRequestRevision();
        if (IsTerminalPathResult && IsStaleNavigationResult)
        {
            // CkNavigation may complete a deferred query after a newer policy/path dispatch. Its
            // result must be observationally inert: do not alter route state or emit terminal signals.
            ck::crowd::Verbose(TEXT("CrowdAgent [{}] ignored stale CkNavigation result rev {} (active {})"),
                InHandle,
                InPathResult.Get_RequestRevision(),
                InPathFollow.Get_ActiveNavigationRequestRevision());
            return;
        }
        if (IsPathNetworkFallback && IsTerminalPathResult)
        {
            const auto& Diagnostics = InPathResult.Get_Diagnostics();
            ck::crowd::Display(
                TEXT("[PNDiag] CrowdAgent [{}] PathNetwork fallback result: status [{}], "
                     "raw [{}] -> [{}], projected [{}:{}] -> [{}:{}], destination [{}], "
                     "waypoints [{}], reason [{}], queryMs [{}]"),
                InHandle,
                InPathResult.Get_Status(),
                Diagnostics.Get_LastAgentLocation(),
                Diagnostics.Get_LastTargetLocation(),
                Diagnostics.Get_StartProjected(),
                Diagnostics.Get_LastProjectedStart(),
                Diagnostics.Get_EndProjected(),
                Diagnostics.Get_LastProjectedEnd(),
                InPathResult.Get_DestinationLocation(),
                InPathResult.Get_Waypoints().Num(),
                Diagnostics.Get_LastFailReason(),
                Diagnostics.Get_LastQueryDurationMs());
        }

        const auto IsNavigationTrouble =
            InPathResult.Get_Status() == ECk_Nav_PathStatus::Partial
            || InPathResult.Get_Status() == ECk_Nav_PathStatus::Failed;
        if (IsTerminalPathResult && (IsPathNetworkFallback || IsNavigationTrouble))
        {
            // Preserve the sidewalk failure already recorded by OnRouteResolved, then append the
            // terminal fallback result. Direct CkNavigation Partial/Failed attempts start here.
            if (NOT IsPathNetworkFallback)
            {
                InPathTrouble._PathNetworkFailReason = ECk_PathNetwork_RouteFailReason::None;
                InPathTrouble._HadPathNetworkFailure = false;
                InPathTrouble._UsedNavigationFallback = false;
            }

            InPathTrouble._AgentLocation = InTransform.Get_Transform().GetLocation();
            InPathTrouble._GoalLocation = InPathFollow.Get_ActiveGoal();
            InPathTrouble._EventTimeSeconds = FPlatformTime::Seconds();
            InPathTrouble._NavigationStatus = InPathResult.Get_Status();
            InPathTrouble._NavigationFailReason =
                InPathResult.Get_Diagnostics().Get_LastFailReason();
            InPathTrouble._HasEvent = true;
        }

        switch (InPathResult.Get_Status())
        {
            case ECk_Nav_PathStatus::Ready:
            case ECk_Nav_PathStatus::Partial:
            {
                InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
                InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
                InHandle.AddOrGet<FTag_CrowdAgent_Walking>();

                const auto& Wps = InPathResult.Get_Waypoints();

                // ExtractWaypoints strips the path's start point, so the incoming direction for
                // Waypoints[0] has to come from the agent's own location at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // This path was planned against every disc confirmed on Recast up to now. A disc
                // still waiting on its async tile rebuild receives a newer serial when confirmed
                // and therefore invalidates this path instead of being silently consumed.
                InPathFollow._PathSerial =
                    FProcessor_CrowdAgent_PathRefresh::Get_CurrentConfirmationSerial();

                // Verdict for the final-stop latch: a Partial path whose reachable end is outside
                // the arrival radius of the goal can never produce a genuine arrival — walking it
                // to the end must report OnGoalFailed. Decided here, against nav-space points,
                // so Steering never compares against a caller-supplied (possibly unprojected) goal.
                InPathFollow._ActivePathEndsShortOfGoal = [&]() -> bool
                {
                    if (InPathResult.Get_Status() != ECk_Nav_PathStatus::Partial || Wps.Num() == 0)
                    { return false; }

                    const auto& Diag = InPathResult.Get_Diagnostics();
                    const auto GoalOnMesh = Diag.Get_EndProjected()
                        ? Diag.Get_LastProjectedEnd()
                        : InPathResult.Get_DestinationLocation();
                    return FVector::Dist(Wps.Last(), GoalOnMesh) > InPathFollow.Get_ActiveArrivalRadius();
                }();

                // Pick the STARTING waypoint: skip every leading corner the agent is ALREADY PAST
                // (dot(corner - agent, onwardSegmentDir) < 0 — UPathFollowingComponent's
                // HasReachedCurrentTarget test applied at install). Two ways a stale first corner
                // happens: the path was computed async while the agent kept its momentum (MoveTo
                // deliberately preserves velocity through PathPending — worst under FollowTarget's
                // frequent repaths), or the navmesh start-projection landed behind the agent. Without
                // this, Steering aims at the behind-corner (its plane test anchors on the agent's own
                // install location, so "crossed" never fires) and the agent visibly walks BACKWARD to
                // the corner before turning around — the "360 at path start" bug. The FINAL waypoint
                // is never skipped (loop bound), matching Steering's retirement rule.
                const auto SkippedWaypointCount =
                    ck_crowd_agent_path_follow_algorithm::SkipAlreadyPassedLeadingWaypoints(
                        InPathFollow.Get_CurrentSegmentStart(),
                        Wps,
                        InPathFollow._WaypointIndex,
                        InPathFollow._CurrentSegmentStart,
                        InPathFollow.Get_ProtectedLeadingWaypointCount());

                if (SkippedWaypointCount > 0)
                {
                    ck::crowd::Verbose(
                        TEXT("CrowdAgent [{}] path install skipped {} already-passed leading corner(s)"),
                        InHandle, SkippedWaypointCount);
                }

                auto PolylineLen = 0.0;
                for (auto i = 0; i < Wps.Num() - 1; ++i)
                { PolylineLen += FVector::Dist(Wps[i], Wps[i + 1]); }
                const auto StraightLen = Wps.Num() >= 2 ? FVector::Dist(Wps[0], Wps.Last()) : 0.0;
                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] PathPending → Walking ({} wps, polyline={}cm, straight={}cm, start={}, end={})"),
                    InHandle, Wps.Num(), PolylineLen, StraightLen,
                    Wps.Num() > 0 ? Wps[0] : FVector::ZeroVector,
                    Wps.Num() > 0 ? Wps.Last() : FVector::ZeroVector);
                break;
            }
            case ECk_Nav_PathStatus::Failed:
            {
                InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
                InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
                InHandle.AddOrGet<FTag_CrowdAgent_Idle>();

                UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                    InHandle,
                    MakePayload(InHandle));

                ck::crowd::Warning(TEXT("CrowdAgent [{}] PathPending → Idle (path failed: {})"),
                    InHandle, InPathResult.Get_Diagnostics().Get_LastFailReason());
                break;
            }
            case ECk_Nav_PathStatus::None:
            case ECk_Nav_PathStatus::Pending:
            default:
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
