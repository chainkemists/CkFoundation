#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Consumes the probe overlap set produced by the PREVIOUS frame's FGroup_Overlap pass — one
    // frame of latency, well below the rate at which agents close on each other.
    // Parallel-safe: registry reads plus a write to the agent's OWN cache; no structural mutations.
    class CKCROWD_API FProcessor_CrowdAgent_NeighborSync : public TParallelProcessor<
            FProcessor_CrowdAgent_NeighborSync,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_HasProbe,
            TReadOnly<FFragment_CrowdAgent_Params>,
            TReadOnly<FFragment_CrowdAgent_ProbeRef>,
            TReadOnly<FFragment_Transform>,
            TReadWrite<FFragment_CrowdAgent_NeighborCache>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunBefore = TDepList<FProcessor_CrowdAgent_Steering>;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_ProbeRef& InProbeRef,
            const FFragment_Transform& InTransform,
            FFragment_CrowdAgent_NeighborCache& InNeighborCache) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
