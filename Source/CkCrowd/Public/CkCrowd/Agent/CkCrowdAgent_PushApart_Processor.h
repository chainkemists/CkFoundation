#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_ApplyOffset_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Phase 2 — post-hoc physical-resolution pass. Direct port of DetourCrowd.cpp:updateStepMove
    // 1601-1662. Iterates 1 or 4 times (project-controlled) over each agent's neighbor cache,
    // accumulating a displacement vector, and stages it into
    // FFragment_CrowdAgent_PendingDisplacement. It does NOT write the Transform — the single
    // Transform writer for a crowd agent is FProcessor_CrowdAgent_ConstrainToNavmesh, which walks
    // the accumulated displacement along the navmesh surface before applying it. Resolves residual
    // overlap that the sampler couldn't prevent (cluster pile-up, fast closing, sampler latency,
    // anything else).
    //
    // No FTag_CrowdAgent_Walking requirement — push-apart fires on idle agents too. That's what
    // handles cluster-at-goal overlap: 5 agents converge to centre, transition Idle, push-apart
    // physically separates them.
    //
    // Group: FGroup_Physics. RunAfter ApplyOffset (shared PendingDisplacement staging).
    class CKCROWD_API FProcessor_CrowdAgent_PushApart : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PushApart,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_ApplyOffset>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_PendingDisplacement& InPending) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
