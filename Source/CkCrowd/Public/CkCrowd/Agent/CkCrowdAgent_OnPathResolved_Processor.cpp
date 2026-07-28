#include "CkCrowdAgent_OnPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathFollow_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_StationaryMarkup_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

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
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_OnPathResolvedProc);

        switch (InPathResult.Get_Status())
        {
            case ECk_Nav_PathStatus::Ready:
            case ECk_Nav_PathStatus::Partial:
            {
                InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                InHandle.AddOrGet<FTag_CrowdAgent_Walking>();

                const auto& Wps = InPathResult.Get_Waypoints();

                // ExtractWaypoints strips the path's start point, so the incoming direction for
                // Waypoints[0] has to come from the agent's own location at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // Planned against every disc painted up to now — only NEWER discs may trigger a
                // PathRefresh re-path.
                InPathFollow._PathSerial = FProcessor_CrowdAgent_StationaryMarkup::Get_CurrentPaintSerial();

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
                        InPathFollow._CurrentSegmentStart);

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
