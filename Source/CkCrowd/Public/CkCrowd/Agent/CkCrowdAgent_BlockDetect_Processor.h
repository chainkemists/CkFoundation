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
    // THE TIER CkCrowd WAS MISSING: noticing that an agent cannot reach its goal.
    //
    // Without this, an agent whose destination is occupied presses against the obstruction forever.
    // Nothing in the system is aware, and the only way the agent ever "arrives" is by shoving the
    // blocker off the spot — which is exactly what it did.
    //
    // Stock UE does not solve this in the solver either: Detour has no impatience, stagnation,
    // minimum-speed or randomisation mechanism anywhere. UE detects a blocked agent one tier ABOVE the
    // solver (UPathFollowingComponent block detection — feet samples on a cadence, all within a small
    // radius of their centroid => OnPathFinished(Blocked)) and hands it to the behaviour tree. We put
    // the detector in the module so gameplay gets a signal instead of a frozen NPC, but the principle
    // is the same: DETECT AND REPORT — do not teach the cost function to give up.
    //
    // TWO detectors, because neither alone is sufficient:
    //
    //   GEOMETRIC (primary, exact, immediate). A neighbour is standing still ON the goal, so the
    //   closest this agent can physically get is (SelfRadius + NbrRadius) from that neighbour's
    //   centre — further out than its arrival radius. No timeout, no guessing, and it names the
    //   blocker. Only catches agent-occupied goals.
    //
    //   NO-PROGRESS (safety net, general). Feet samples on a cadence; if they all sit within a small
    //   radius of their centroid, the agent is going nowhere — for ANY reason (a plug of several
    //   agents, a dynamic prop, a pathological corridor).
    //
    // Why both: a blocked agent that has committed to a side can ORBIT the blocker at the radius sum
    // rather than sit still. An orbiting agent's samples are spread around a ~168cm circle, so the
    // centroid test never trips — the geometric test is the only one that reliably catches an occupied
    // goal. Conversely the geometric test is blind to walls and multi-agent plugs. Neither is redundant.
    class CKCROWD_API FProcessor_CrowdAgent_BlockDetect : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_BlockDetect,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_Walking,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
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
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const -> void;

    private:
        auto
        DoBlock(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            ECk_CrowdAgent_BlockedReason InReason,
            FCk_Handle InBlocker,
            float InDistanceToGoal) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The other half of HoldAndRetry: an agent holding at a blocked goal re-checks on a cadence and
    // resumes the moment the goal clears. Without this, "blocked" would be terminal — which is exactly
    // the flaw in the tempting shortcut of silently widening the arrival radius: an agent that has
    // "arrived" 100cm out is Idle and never walks the last metre when the blocker leaves.
    class CKCROWD_API FProcessor_CrowdAgent_BlockedRecheck : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_BlockedRecheck,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_GoalBlocked,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
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
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
