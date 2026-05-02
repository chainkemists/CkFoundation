#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gate 3 — Sub-task 3A. Each frame, reads the agent's probe-child overlap set, transforms each
    // overlap into a FCk_CrowdAgent_Neighbor (excluding self via the lifetime-owner mapping back from
    // the other probe's host entity), sorts by distance ascending, and trims to _MaxNeighborsForSteering.
    //
    // Group: FGroup_Physics. RunBefore Steering so Gate 3C's combined force calculation reads the same
    // frame's cache. The probe overlap set itself is updated by FProcessor_Probe_HandleRequests in
    // FGroup_Overlap (TG_PostPhysics); NeighborSync reads it from the *previous* frame's PostPhysics
    // pass — one frame of latency, well below the rate at which agents close on each other.
    class CKCROWD_API FProcessor_CrowdAgent_NeighborSync : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_NeighborSync,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_ProbeRef>,
            ck::TReadWrite<FFragment_CrowdAgent_NeighborCache>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunBefore = TDepList<FProcessor_CrowdAgent_Steering>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_ProbeRef& InProbeRef,
            FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
