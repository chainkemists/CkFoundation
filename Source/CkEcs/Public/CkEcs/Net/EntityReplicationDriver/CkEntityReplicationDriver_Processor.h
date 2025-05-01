#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

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
        using MarkedDirtyBy = FTag_EntityReplicationDriver_FireOnDependentReplicationComplete;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
