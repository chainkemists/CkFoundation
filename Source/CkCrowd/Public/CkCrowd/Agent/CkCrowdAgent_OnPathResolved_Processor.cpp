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
                InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                InHandle.AddOrGet<FTag_CrowdAgent_Walking>();

                const auto& Wps = InPathResult.Get_Waypoints();

                // ExtractWaypoints strips the path's start point, so the incoming direction for
                // Waypoints[0] has to come from the agent's own location at install time.
                InPathFollow._CurrentSegmentStart = InTransform.Get_Transform().GetLocation();

                // Planned against every disc painted up to now — only NEWER discs may trigger a
                // PathRefresh re-path.
                InPathFollow._PathSerial = FProcessor_CrowdAgent_StationaryMarkup::Get_CurrentPaintSerial();

                // Without skipping the leading corners the agent is already past, Steering aims
                // behind it and the agent visibly walks BACKWARD before turning around. The FINAL
                // waypoint is never skipped (loop bound), matching Steering's retirement rule.
                const auto AgentLoc = InPathFollow.Get_CurrentSegmentStart();
                while (InPathFollow._WaypointIndex < Wps.Num() - 1)
                {
                    const auto& Corner = Wps[InPathFollow._WaypointIndex];
                    const auto OnwardDir = (Wps[InPathFollow._WaypointIndex + 1] - Corner).GetSafeNormal();
                    if (OnwardDir.IsNearlyZero())
                    { break; }

                    const auto CornerIsStillAhead = FVector::DotProduct(Corner - AgentLoc, OnwardDir) >= 0.0;
                    if (CornerIsStillAhead)
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
                break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
