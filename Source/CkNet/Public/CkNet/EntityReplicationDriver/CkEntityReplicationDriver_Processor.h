#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNET_API FProcessor_ReplicationDriver_HandleRequests
        : public TProcessor<FProcessor_ReplicationDriver_HandleRequests,
                    FFragment_ReplicationDriver_Requests,
                    TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>
    {
    public:
        using MarkedDirtyBy = FFragment_ReplicationDriver_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InEntity,
            FFragment_ReplicationDriver_Requests& InRequestsComp,
            const TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>& InRepDriver) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
