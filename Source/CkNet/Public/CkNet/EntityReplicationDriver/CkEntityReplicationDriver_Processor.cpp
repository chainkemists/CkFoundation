#include "CkEntityReplicationDriver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ReplicationDriver_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEntity,
            FFragment_ReplicationDriver_Requests& InRequestsComp,
            const TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>& InRepDriver) const
        -> void
    {
        algo::ForEachRequest(InRequestsComp._Requests,
            [&](const FFragment_ReplicationDriver_Requests::RequestType& InRequest) -> void
            {
                InRepDriver->Request_ReplicateEntity_OnServer(InRequest.Get_ConstructionInfo());
            });

        InEntity.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

