#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCROWD_API FProcessor_CrowdAgent_TransientPersonalSpace : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_TransientPersonalSpace,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_CrowdAgent_TransientPersonalSpace>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_CrowdAgent_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_CrowdAgent,
            FFragment_CrowdAgent_TransientPersonalSpace& InPersonalSpace) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
