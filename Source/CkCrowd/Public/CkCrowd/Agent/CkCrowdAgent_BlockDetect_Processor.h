#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Detects that an agent cannot reach its goal and reports it, via two non-redundant detectors —
    // geometric (occupied goal) and no-progress-along-path (everything else) — and answers a stall
    // with a bounded re-path ladder before escalating it to a block. Rationale and the
    // catches/misses table: CkCrowd/CLAUDE.md § "Blocked goals".
    class CKCROWD_API FProcessor_CrowdAgent_BlockDetect : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_BlockDetect,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_Walking,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_NeighborSync>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_Steering>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const -> void;

    private:
        auto
        DoBlock(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            ECk_CrowdAgent_BlockedReason InReason,
            FCk_Handle InBlocker,
            float InDistanceToGoal) const -> void;

        // Replans at the goal already active, bypassing the caller-facing same-goal guard. The
        // frozen Recast polyline is what most stalls are actually about: the geometry the planner
        // saw is not the geometry the agent is pressing against.
        auto
        DoRepathAtActiveGoal(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The other half of HoldAndRetry: re-checks on a cadence and resumes the moment the goal clears,
    // which is what keeps a GoalOccupied block from being terminal. A NoProgress block has no
    // blocker that can clear, so its re-checks are bounded and end in OnGoalFailed.
    class CKCROWD_API FProcessor_CrowdAgent_BlockedRecheck : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_BlockedRecheck,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_GoalBlocked,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_CrowdAgent_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const -> void;

    private:
        // Terminal end of a NoProgress hold: the retry budget is spent, so the caller is told the
        // move failed instead of the agent waiting on an obstruction that will never move.
        auto
        DoFailMove(
            HandleType InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
