#include "CkIntegerAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_ReplicateOnRestore);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_Refill);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IntegerAttribute_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            FCk_Handle_IntegerAttribute InHandle,
            const FFragment_IntegerAttribute_Current& /*InCurrent*/) const
        -> void
    {
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(LifetimeOwner))
        { return; }

        // Re-create the per-owner replication container entry — the snapshot respawn re-materializes the owner with
        // Construct abstained, so the entry Add() normally makes is missing and the Replicate processor's UPDATE path
        // would fail ("No container fragment entry found"). Idempotent + host-gated. See the Float processor for detail.
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_IntegerAttributes>(LifetimeOwner);

        InHandle.Add<FFragment_IntegerAttribute_Current::FTag_MayRequireReplication>();
        if (InHandle.Has<FFragment_IntegerAttribute_Min>())
        { InHandle.AddOrGet<FFragment_IntegerAttribute_Min::FTag_MayRequireReplication>(); }
        if (InHandle.Has<FFragment_IntegerAttribute_Max>())
        { InHandle.AddOrGet<FFragment_IntegerAttribute_Max::FTag_MayRequireReplication>(); }

        InHandle.Remove<FTag_Snapshot_JustRestored>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
