#include "CkEntityReplicationDriver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ReplicationDriver_FireOnDependentReplicationComplete::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();
        UUtils_Signal_OnDependentsReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------

