#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-agent separation force solver. Reads the trimmed neighbor cache
    // populated by NeighborSync; for each neighbor inside _SeparationRadius, contributes a
    // quadratic-falloff push directed away from the neighbor. The result lives in
    // FFragment_CrowdAgent_SeparationForce, scaled by _SeparationWeight, ready for the
    // steering-side combination with the path-follow direction.
    //
    // Group: FGroup_Physics. RunAfter NeighborSync (cache must be fresh), RunBefore Steering
    // (3C reads SeparationForce).
    //
    // Stalemate avoidance: two agents perfectly mirroring each other on a head-on path produce
    // equal-and-opposite forces and stand still. A tiny per-agent jitter keyed off the handle's
    // entity index breaks the symmetry deterministically — different agents always pick the same
    // tie-breaker, so behavior is reproducible.
    class CKCROWD_API FProcessor_CrowdAgent_Separation : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_Separation,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_SeparationForce>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter  = TDepList<FProcessor_CrowdAgent_NeighborSync>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_Steering>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_SeparationForce& InSeparationForce) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
