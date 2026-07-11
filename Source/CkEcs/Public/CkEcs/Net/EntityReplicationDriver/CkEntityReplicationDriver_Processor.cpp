#include "CkEntityReplicationDriver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ReplicationDriver_FireOnDependentReplicationComplete);

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ReplicationDriver_FireOnDependentReplicationComplete::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        // Fire-gating (spec §4.4, CTO note N2): do not fire until every replicated-fragment / hydration payload on
        // this entity AND its lifetime-dependents has drained — otherwise an OnReplicationComplete /
        // OnDependentsReplicationComplete consumer reads pre-apply state (esp. the §2.4 ConstructedThisFrame defer
        // window on initial subtree replication). Leave the fire tag (also MarkedDirtyBy) so the same-frame pump /
        // next tick retries; the dispatcher's 5s/2s timeout bounds a stuck entry so this cannot hang forever.
        if (UCk_Utils_EntityReplicationDriver_UE::Get_HasUndrainedReplicatedFragments_IncludingDependents(InHandle))
        { return; }

        InHandle.Remove<MarkedDirtyBy>();

        if (NOT UUtils_Signal_OnReplicationComplete::HasFiredAtLeastOnce(InHandle))
        { UUtils_Signal_OnReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle)); }

        UUtils_Signal_OnDependentsReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------

