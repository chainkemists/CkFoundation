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
    // dtCrowd-style penalty-scored velocity sampling; overwrites _DesiredVelocity with the
    // lowest-penalty candidate. PARALLEL-SAFE: every cross-entity access is a read (the zone-tag
    // walk), and the only write is the agent's own DesiredVelocity.
    //
    // A flying agent is excluded: the velocity-obstacle quadratic, its penalties and its side
    // preference are all FVector2D, so sampling would flatten a 3D route's desired velocity onto the
    // XY plane. Flyers get no avoidance until a volumetric sampler exists.
    class CKCROWD_API FProcessor_CrowdAgent_AvoidanceSample : public TParallelProcessor<
            FProcessor_CrowdAgent_AvoidanceSample,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Walking,
            FTag_CrowdAgent_HasProbe,
            TReadOnly<FFragment_CrowdAgent_Params>,
            TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
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
