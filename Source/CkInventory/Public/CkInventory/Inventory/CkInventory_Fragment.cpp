#include "CkInventory_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Inventory

static struct FInventoryRepHandlerRegistrar
{
    FInventoryRepHandlerRegistrar()
    {
        const auto DoApplyInventoryItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_ReplicatedEntry>& OldItems)
        {
            // Find inventory entities owned by this entity and apply the sync fragment
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            for (auto InventoryHandle : Inventories)
            {
                InventoryHandle.AddOrGet<ck::FFragment_Inventory_SyncReplication>(NewItems, OldItems);
            }
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_InventoryItems::StaticStruct(); },
            {
                .OnChange = [DoApplyInventoryItems](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    DoApplyInventoryItems(Entity, New.Get<FCk_RepData_InventoryItems>().Items, Old.Get<FCk_RepData_InventoryItems>().Items);
                },
                .OnAdd = [DoApplyInventoryItems](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyInventoryItems(Entity, Data.Get<FCk_RepData_InventoryItems>().Items, TArray<FCk_InventoryItem_ReplicatedEntry>{});
                }
            });
    }
} GInventoryRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
