#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Processor.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINVENTORY_API FProcessor_Inventory_DataOnly_SyncReplication : public TProcessor_Inventory_SyncReplication_Base<
            FProcessor_Inventory_DataOnly_SyncReplication,
            FCk_Handle_Inventory_DataOnly,
            FFragment_Inventory_DataOnly_SyncReplication,
            FTag_Inventory_DataOnly,
            FCk_InventoryItem_DataOnly_ReplicatedEntry>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FFragment_Inventory_DataOnly_SyncReplication;
        using TProcessor_Inventory_SyncReplication_Base::TProcessor_Inventory_SyncReplication_Base;
        // No OnEntryAdded/OnEntryRemoved overrides — uses base no-ops (DataOnly has no grid).
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINVENTORY_API FProcessor_Inventory_DataOnly_HandleRequests : public TProcessor_Inventory_HandleRequests_Base<
            FProcessor_Inventory_DataOnly_HandleRequests,
            FCk_Handle_Inventory_DataOnly,
            FFragment_Inventory_DataOnly_Requests,
            FTag_Inventory_DataOnly>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Inventory_DataOnly_SyncReplication>;
        using MarkedDirtyBy = FFragment_Inventory_DataOnly_Requests;
        using TProcessor_Inventory_HandleRequests_Base::TProcessor_Inventory_HandleRequests_Base;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives a RESTORED DataOnly inventory after a snapshot load. The shape tag
    // (FTag_Inventory_DataOnly) and the transient PreviousItems diff cache are not snapshotted, and
    // the owner-hosted container entry is missing (Construct is abstained during reconstitution) —
    // re-derive all three from the restored FFragment_Inventory_Params (whose _InventoryType is the
    // shape discriminator), then re-arm FTag_Inventory_MayRequireReplication so the Replicate
    // processor rebuilds the wire entries from the restored item record. SPATIAL inventories
    // re-drive through their own sibling (FProcessor_Inventory_Spatial_ReplicateOnRestore), which
    // additionally re-composes the snapshot-invisible grid state. The view has no shape tag
    // dependency; the body filters on the discriminator. Pairs ck::FTag_Snapshot_JustRestored
    // (shared per-entity, never removed here) with the per-feature done tag, and POINT-QUERIES the
    // in_place marker (listing it in the view would surface tombstones).
    class CKINVENTORY_API FProcessor_Inventory_DataOnly_ReplicateOnRestore : public ck_exp::TProcessor<
            FProcessor_Inventory_DataOnly_ReplicateOnRestore,
            FCk_Handle_Inventory,
            TReadOnly<FFragment_Inventory_Params>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINVENTORY_API FProcessor_Inventory_DataOnly_Replicate : public ck_exp::TProcessor<
            FProcessor_Inventory_DataOnly_Replicate,
            FCk_Handle_Inventory_DataOnly,
            TReadOnly<FFragment_Inventory_Params>,
            FTag_Inventory_DataOnly,
            FTag_Inventory_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_Inventory_MayRequireReplication;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------