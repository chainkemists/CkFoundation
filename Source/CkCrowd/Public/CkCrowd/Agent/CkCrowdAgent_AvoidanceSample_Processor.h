#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Phase 2 — dtCrowd-style penalty-scored velocity sampling. Runs only when triggered
    // (project-wide trigger mode + per-agent override + zone tags), only on 1 in N frames per
    // agent (round-robin), only on agents with at least 1 neighbor in cache. When it runs,
    // overwrites _DesiredVelocity with the lowest-penalty candidate from a 16-sample pattern.
    //
    // See Gate_03_Separation_Addendum.md §5 for design rationale and dtCrowd citations.
    //
    // Group: FGroup_Physics. RunAfter Steering (we override its output) + NeighborSync (we read
    // the cache it just wrote). RunBefore AccelClamp + VelocityBridge (so the override gets ramped
    // and shipped). RunBefore is enforced by AccelClamp's RunAfter on this processor (Step 5.5).
    //
    // PARALLEL: the Samples × Neighbors scoring loop — the module's only nested-loop hot spot —
    // is pure math over the agent's own cache; all cross-entity access is reads (zone-tag walk),
    // the only write is the agent's OWN DesiredVelocity.
    class CKCROWD_API FProcessor_CrowdAgent_AvoidanceSample : public TParallelProcessor<
            FProcessor_CrowdAgent_AvoidanceSample,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Walking,
            FTag_CrowdAgent_HasProbe,
            TReadOnly<FFragment_CrowdAgent_Params>,
            TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering, FProcessor_CrowdAgent_NeighborSync>;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
