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
            FFragment_Inventory_DataOnly_Requests>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Inventory_DataOnly_SyncReplication>;
        using MarkedDirtyBy = FFragment_Inventory_DataOnly_Requests;
        using TProcessor_Inventory_HandleRequests_Base::TProcessor_Inventory_HandleRequests_Base;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINVENTORY_API FProcessor_Inventory_DataOnly_Replicate : public ck_exp::TProcessor<
            FProcessor_Inventory_DataOnly_Replicate,
            FCk_Handle_Inventory_DataOnly,
            TReadOnly<FFragment_Inventory_Params>,
            FTag_Inventory_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_Inventory_MayRequireReplication;
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------