#include "CkCrowdAgent_OnPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

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
                auto NonConstHandle = InHandle;
                NonConstHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                NonConstHandle.AddOrGet<FTag_CrowdAgent_Walking>();

                // Diagnostic for the "agent orbits its goal" bug: log the path shape so the failing
                // case can be reproduced exactly. A polyline length much larger than the straight-line
                // distance means the path wraps around an obstacle (e.g. a reservation point on the far
                // face of a gondola) — the suspected orbit trigger.
                const auto& Wps = InPathResult.Get_Waypoints();

                // Anchor the FIRST path segment for Steering's plane-crossing waypoint retirement.
                // ExtractWaypoints strips the path's start point, so Waypoints[-1] does not exist and
                // the incoming direction for Waypoints[0] has to come from the agent's own location at
                // install time. A missing Transform mirrors Steering's quiet-bail posture (gym/game
                // code adds Transform before pathing); the Wps[0] fallback degenerates the index-0
                // plane test to proximity-only, which is the old behaviour — never a mis-fire.
                auto TransformHandle = UCk_Utils_Transform_UE::Cast(NonConstHandle);
                InPathFollow._CurrentSegmentStart = ck::IsValid(TransformHandle)
                    ? UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle)
                    : (Wps.Num() > 0 ? Wps[0] : FVector::ZeroVector);

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
                auto NonConstHandle = InHandle;
                NonConstHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
                NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();

                UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                    NonConstHandle,
                    MakePayload(NonConstHandle));

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
