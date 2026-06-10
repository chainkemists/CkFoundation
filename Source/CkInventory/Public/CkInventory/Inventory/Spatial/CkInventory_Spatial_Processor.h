#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Processor.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINVENTORY_API FProcessor_Inventory_Spatial_SyncReplication : public TProcessor_Inventory_SyncReplication_Base<
            FProcessor_Inventory_Spatial_SyncReplication,
            FCk_Handle_Inventory_Spatial,
            FFragment_Inventory_Spatial_SyncReplication,
            FTag_Inventory_Spatial,
            FCk_InventoryItem_Spatial_ReplicatedEntry>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FFragment_Inventory_Spatial_SyncReplication;
        using TProcessor_Inventory_SyncReplication_Base::TProcessor_Inventory_SyncReplication_Base;

    public:
        // Per-shape hooks override the base's no-op defaults via CRTP shadowing.
        static auto OnEntryAdded(
            FCk_Handle_Inventory_Spatial& InHandle,
            const FCk_InventoryItem_Spatial_ReplicatedEntry& InEntry,
            FCk_Handle_Item& InItem) -> void;
        static auto OnEntryRemoved(
            FCk_Handle_Inventory_Spatial& InHandle,
            FCk_Handle_Item& InItem) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINVENTORY_API FProcessor_Inventory_Spatial_HandleRequests : public TProcessor_Inventory_HandleRequests_Base<
            FProcessor_Inventory_Spatial_HandleRequests,
            FCk_Handle_Inventory_Spatial,
            FFragment_Inventory_Spatial_Requests,
            FTag_Inventory_Spatial>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Inventory_Spatial_SyncReplication>;
        using MarkedDirtyBy = FFragment_Inventory_Spatial_Requests;
        using TProcessor_Inventory_HandleRequests_Base::TProcessor_Inventory_HandleRequests_Base;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives a RESTORED Spatial inventory after a snapshot load (DataOnly sibling:
    // FProcessor_Inventory_DataOnly_ReplicateOnRestore). On top of the DataOnly recipe (shape tag +
    // PreviousItems re-derive, owner-hosted container re-create, item re-replication via
    // Request_TryReplicateExisting, MayRequireReplication re-arm), Spatial must re-COMPOSE live grid
    // state the snapshot can never carry:
    //   - the inventory's own 2dGridSystem (its cells live in a PRIVATE nested registry) is re-built
    //     from the restored Params' _Dimensions, exactly like Setup_PerShape does on first composition;
    //   - each item's Dimensions-trait shape grid is re-built from the item's restored Definition
    //     (Construct is abstained during reconstitution, so traits never re-ran);
    //   - each item is re-STAMPED onto the rebuilt grid from its restored placement decision record
    //     (FFragment_Item_SpatialPlacement) via the ordinary Request_PlaceItemOnGrid path.
    // All re-compose steps are Has-guarded and run BEFORE the driver gate (even a never-replicated
    // restored inventory needs a working grid); the replication half retries until the owner's driver
    // and every item's re-replication have landed. The view has no shape tag dependency; the body
    // filters on the Params discriminator and POINT-QUERIES the shared restored marker.
    class CKINVENTORY_API FProcessor_Inventory_Spatial_ReplicateOnRestore : public ck_exp::TProcessor<
            FProcessor_Inventory_Spatial_ReplicateOnRestore,
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

    class CKINVENTORY_API FProcessor_Inventory_Spatial_Replicate : public ck_exp::TProcessor<
            FProcessor_Inventory_Spatial_Replicate,
            FCk_Handle_Inventory_Spatial,
            TReadOnly<FFragment_Inventory_Params>,
            FTag_Inventory_Spatial,
            FTag_Inventory_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_Inventory_MayRequireReplication;
        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------