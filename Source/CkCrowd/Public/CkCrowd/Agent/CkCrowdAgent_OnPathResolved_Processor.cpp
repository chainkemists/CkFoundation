#include "CkCrowdAgent_OnPathResolved_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_OnPathResolved);

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
                ck::crowd::Verbose(TEXT("CrowdAgent [{}] PathPending → Walking ({} waypoints)"),
                    InHandle, InPathResult.Get_Waypoints().Num());
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
