#include "CkEntityReplicationDriver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ReplicationDriver_ReplicateEntityScript::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Request_EntityScript_Replicate& InRequest) const
        -> void
    {
        UCk_Utils_EntityReplicationDriver_UE::Add(InHandle);
        UCk_Utils_EntityReplicationDriver_UE::Request_Replicate(InHandle, InRequest.Get_Owner(), InRequest.Get_EntityScriptClass());

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

