#include "CkCrowdAgent_OnPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
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
                // Path-follow cursor was pre-zeroed by HandleRequests on MoveTo; we just need to flip
                // the state tags. Walking gates the steering view; once stamped, steering begins
                // consuming the path next frame.
                InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                InHandle.AddOrGet<FTag_CrowdAgent_Walking>();

                // Diagnostic for the "agent orbits its goal" bug: log the path shape so the failing
                // case can be reproduced exactly. A polyline length much larger than the straight-line
                // distance means the path wraps around an obstacle (e.g. a reservation point on the far
                // face of a gondola) — the suspected orbit trigger.
                const auto& Wps = InPathResult.Get_Waypoints();

                // Anchor the FIRST path segment for Steering's plane-crossing waypoint retirement.
                // ExtractWaypoints strips the path's start point, so Waypoints[-1] does not exist and
                // the incoming direction for Waypoints[0] has to come from the agent's own location at
                // install time (the view guarantees the Transform fragment).
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // This path was planned against every disc painted up to now — only NEWER discs
                // may trigger a PathRefresh re-path.
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
                const auto AgentLoc = InPathFollow.Get_CurrentSegmentStart();
                while (InPathFollow._WaypointIndex < Wps.Num() - 1)
                {
                    const auto& Corner = Wps[InPathFollow._WaypointIndex];
                    const auto OnwardDir = (Wps[InPathFollow._WaypointIndex + 1] - Corner).GetSafeNormal();
                    if (OnwardDir.IsNearlyZero())
                    { break; }
                    if (FVector::DotProduct(Corner - AgentLoc, OnwardDir) >= 0.0)
                    { break; }

                    InPathFollow._CurrentSegmentStart = Corner;
                    ++InPathFollow._WaypointIndex;
                }

                if (InPathFollow._WaypointIndex > 0)
                {
                    ck::crowd::Verbose(
                        TEXT("CrowdAgent [{}] path install skipped {} already-passed leading corner(s)"),
                        InHandle, InPathFollow._WaypointIndex);
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
                // Still waiting on CkNavigation to drain the request — no-op.
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
