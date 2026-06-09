#include "CkFloatAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_ReplicateOnRestore);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_Refill);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_FloatAttribute_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            FCk_Handle_FloatAttribute InHandle,
            const FFragment_FloatAttribute_Current& /*InCurrent*/) const
        -> void
    {
        // Point-query the in_place restore marker (NOT in the view — that would surface tombstones). Only restored
        // attributes carry it; everything else early-outs cheaply.
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        // Re-arm only once the owner's replication driver is re-established by the snapshot respawn pass — otherwise the
        // downstream Replicate processor's container update would target a missing driver. Leaving the marker (no
        // Remove) retries on the next tick until the driver exists.
        if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(LifetimeOwner))
        { return; }

        // Re-create the per-owner replication container entry. Add() creates it at Construct-time, but the snapshot
        // respawn re-materializes the owner with Construct abstained, so the entry is missing on the respawned owner —
        // and the downstream Replicate processor's UPDATE path requires it to already exist ("No container fragment
        // entry found" otherwise). Idempotent (no-op if already present) + host-gated. Mirrors the Add() ordering:
        // container-add on the owner, then re-arm the value trigger on the attribute.
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_FloatAttributes>(LifetimeOwner);

        InHandle.Add<FFragment_FloatAttribute_Current::FTag_MayRequireReplication>();
        if (InHandle.Has<FFragment_FloatAttribute_Min>())
        { InHandle.AddOrGet<FFragment_FloatAttribute_Min::FTag_MayRequireReplication>(); }
        if (InHandle.Has<FFragment_FloatAttribute_Max>())
        { InHandle.AddOrGet<FFragment_FloatAttribute_Max::FTag_MayRequireReplication>(); }

        InHandle.Remove<FTag_Snapshot_JustRestored>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
