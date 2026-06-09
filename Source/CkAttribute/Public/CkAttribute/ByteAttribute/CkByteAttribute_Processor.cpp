#include "CkByteAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_ReplicateOnRestore);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ByteAttribute_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            FCk_Handle_ByteAttribute InHandle,
            const FFragment_ByteAttribute_Current& /*InCurrent*/) const
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
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_ByteAttributes>(LifetimeOwner);

        InHandle.Add<FFragment_ByteAttribute_Current::FTag_MayRequireReplication>();
        if (InHandle.Has<FFragment_ByteAttribute_Min>())
        { InHandle.AddOrGet<FFragment_ByteAttribute_Min::FTag_MayRequireReplication>(); }
        if (InHandle.Has<FFragment_ByteAttribute_Max>())
        { InHandle.AddOrGet<FFragment_ByteAttribute_Max::FTag_MayRequireReplication>(); }

        InHandle.Remove<FTag_Snapshot_JustRestored>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
