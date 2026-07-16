#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Monitors PathPending agents for nav-result resolution.
    //
    // Polls each agent's FFragment_Nav_PathResult while the agent is in the PathPending state. When
    // the status moves to Ready/Partial → transition to Walking (steering's view starts firing).
    // When status is Failed → transition to Idle and broadcast OnGoalFailed.
    //
    // Cheaper than binding a delegate per move-request because it's view-iteration driven and only
    // touches agents that are actually waiting for a path. Once the transition out of PathPending
    // happens, the view filter excludes the entity until the next MoveTo.
    //
    // PumpPolicy::SkipPump — the body broadcasts OnGoalFailed on Failed status; pumping with
    // DeltaT=0 would re-broadcast on the next pump pass before the tag transition takes effect.
    class CKCROWD_API FProcessor_CrowdAgent_OnPathResolved : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_OnPathResolved,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_PathPending,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        // Both this and HandleRequests write FFragment_CrowdAgent_PathFollow; declaration order put
        // HandleRequests first. RunAfter it to make that write-ordering explicit.
        using RunAfter = TDepList<FProcessor_CrowdAgent_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
