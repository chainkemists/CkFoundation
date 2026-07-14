#include "CkInventory_Spatial_Processor.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"
#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_CancelOnEndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_SyncReplication);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    auto
        FProcessor_Inventory_Spatial_SyncReplication::
        OnEntryAdded(
            FCk_Handle_Inventory_Spatial& InHandle,
            const FCk_InventoryItem_Spatial_ReplicatedEntry& InEntry,
            FCk_Handle_Item& InItem) -> void
    {
        if (InEntry.Get_Coordinate().X >= 0 && InEntry.Get_Coordinate().Y >= 0)
        {
            UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
                InHandle, InItem, InEntry.Get_Coordinate(), InEntry.Get_Rotation());
        }
    }

    auto
        FProcessor_Inventory_Spatial_SyncReplication::
        OnEntryRemoved(
            FCk_Handle_Inventory_Spatial& InHandle,
            FCk_Handle_Item& InItem) -> void
    {
        UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(InHandle, InItem);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_Spatial_Replicate::
        ForEachEntity(
            TimeType,
            HandleType& InHandle,
            const FFragment_Inventory_Params&) -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto& Items = FInventoryItemRecord::Get_ValidEntries(InHandle);

        auto Entries = TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>{};
        Entries.Reserve(Items.Num());

        algo::ForEachIsValid(Items, [&](const auto& ItemHandle)
        {
            const auto Coordinate = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementCoordinate(InHandle, ItemHandle);
            const auto Rotation   = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementRotation(ItemHandle);

            Entries.Emplace(FCk_InventoryItem_Spatial_ReplicatedEntry(ItemHandle)
                .Set_Coordinate(Coordinate)
                .Set_Rotation(Rotation));
        });

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Inventory_Spatial_Items>(
            LifetimeOwner, [&](FCk_RepData_Inventory_Spatial_Items& Data)
        {
            Data.Items = MoveTemp(Entries);
        });

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------