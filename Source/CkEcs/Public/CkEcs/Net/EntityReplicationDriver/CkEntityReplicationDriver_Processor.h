#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_ReplicationDriver_FireOnDependentReplicationComplete : public
        ck_exp::TProcessor<FProcessor_ReplicationDriver_FireOnDependentReplicationComplete,
            FCk_Handle,
            FTag_EntityReplicationDriver_FireOnDependentReplicationComplete,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        using MarkedDirtyBy = FTag_EntityReplicationDriver_FireOnDependentReplicationComplete;
        static constexpr auto LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad; // load-gate kernel (spec §4.3)
        static constexpr auto HydrationQuarantinePolicy = ECk_ProcessorHydrationQuarantine::Exempt; // load-gate kernel

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
