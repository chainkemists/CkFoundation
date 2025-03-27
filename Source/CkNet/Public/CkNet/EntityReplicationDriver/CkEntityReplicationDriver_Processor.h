#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNET_API FProcessor_ReplicationDriver_ReplicateEntityScript : public
        ck_exp::TProcessor<FProcessor_ReplicationDriver_ReplicateEntityScript,
            FCk_Handle,
            FCk_Request_EntityScript_Replicate>
    {
    public:
        using MarkedDirtyBy = FCk_Request_EntityScript_Replicate;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Request_EntityScript_Replicate& InRequest) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
