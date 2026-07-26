#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Processor.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Processor.h"

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

    class CKINVENTORY_API FProcessor_Inventory_Spatial_CancelOnEndPlay : public TProcessor_Inventory_CancelOnEndPlay_Base<
            FProcessor_Inventory_Spatial_CancelOnEndPlay,
            FCk_Handle_Inventory_Spatial,
            FFragment_Inventory_Spatial_Requests,
            FTag_Inventory_Spatial>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor_Inventory_CancelOnEndPlay_Base::TProcessor_Inventory_CancelOnEndPlay_Base;
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
        // Shares MarkedDirtyBy with the DataOnly sibling, which filters to a disjoint shape — ordering
        // is immaterial, declared only to silence the scheduler's dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_Inventory_DataOnly_Replicate>;
        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------