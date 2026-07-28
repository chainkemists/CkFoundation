#include "CkCrowdAgent_FollowTarget_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_FollowTarget);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::FollowTarget"), STAT_CkCrowd_FollowTargetProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_FollowTarget::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_FollowTarget& InFollow) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_FollowTargetProc);

        const auto& Request = InFollow.Get_Request();

        InFollow._RepathAccumulatorSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InFollow._RepathAccumulatorSec < static_cast<float>(Request.Get_RepathPeriod().Get_Seconds()))
        { return; }
        InFollow._RepathAccumulatorSec = 0.0f;

        const auto& TargetPoint = Request.Get_TargetPoint();
        if (ck::Is_NOT_Valid(TargetPoint))
        {
            // The agent keeps walking to its last resolved goal, as if it were a plain MoveTo.
            InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();
            ck::crowd::Verbose(TEXT("CrowdAgent [{}] follow target died — follow ended"), InHandle);
            return;
        }

        if (InHandle.Has<FTag_CrowdAgent_PathPending>())
        { return; }

        const auto LiveTargetLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TargetPoint);

        auto ShouldRepath = false;
        if (InHandle.Has<FTag_CrowdAgent_Walking>() || InHandle.Has<FTag_CrowdAgent_GoalBlocked>())
        {
            // GoalBlocked is included so BlockedRecheck cannot resume toward a spot the target has
            // since vacated — the fresh request resets the blocked episode.
            ShouldRepath = FVector::Dist(LiveTargetLoc, InPathFollow.Get_ActiveGoal())
                > Request.Get_RepathThresholdCm();
        }
        else if (InHandle.Has<FTag_CrowdAgent_Idle>())
        {
            const auto SelfLoc = InTransform.Get_Transform().GetLocation();
            ShouldRepath = FVector::Dist(SelfLoc, LiveTargetLoc)
                > InPathFollow.Get_ActiveArrivalRadius() + Request.Get_ResumeSlackCm();
        }

        if (NOT ShouldRepath)
        { return; }

        // HandleRequests re-resolves the live goal and re-arms this fragment, so the cadence
        // continues from the repath.
        UCk_Utils_CrowdAgent_UE::Request_FollowTarget(InHandle, Request, {});
    }
}

// --------------------------------------------------------------------------------------------------------------------
