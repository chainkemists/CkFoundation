#include "CkInventory_DataOnly_Processor.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_RequestTraits.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_SyncReplication);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_DataOnly_Replicate::
        DoTick(
            TimeType InDeltaT) -> void
    {
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Inventory_DataOnly_Replicate::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_Inventory_Params&) -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        FCk_Handle_Inventory BaseHandle = InHandle;
        const auto& Items = FInventoryItemRecord::Get_ValidEntries(BaseHandle);

        TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry> Entries;
        Entries.Reserve(Items.Num());

        ck::algo::ForEachIsValid(Items, [&](const auto& ItemHandle)
        {
            Entries.Emplace(FCk_InventoryItem_DataOnly_ReplicatedEntry(ItemHandle));
        });

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Inventory_DataOnly_Items>(
            LifetimeOwner, [&](FCk_RepData_Inventory_DataOnly_Items& Data)
        {
            Data.Items = MoveTemp(Entries);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
