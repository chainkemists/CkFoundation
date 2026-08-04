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
    // A short-range reactive nudge, NOT the avoidance system — that is
    // FProcessor_CrowdAgent_AvoidanceSample. Only acts inside _SeparationRadius.
    //
    // A flying agent is excluded: the force is zeroed in Z by construction, so it would answer a
    // vertical overlap with a horizontal shove.
    class CKCROWD_API FProcessor_CrowdAgent_Separation : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_Separation,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_SeparationForce>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
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
