#include "CkInventory_DataOnly_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] static struct FInventory_DataOnly_RepHandlerRegistrar
{
    FInventory_DataOnly_RepHandlerRegistrar()
    {
        const auto DoApplyDataOnlyItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& OldItems)
        {
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            for (auto InventoryHandle : Inventories)
            {
                if (NOT UCk_Utils_Inventory_UE::Get_IsDataOnly(InventoryHandle))
                { continue; }

                InventoryHandle.AddOrGet<ck::FFragment_Inventory_DataOnly_SyncReplication>(NewItems, OldItems);
            }
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Inventory_DataOnly_Items::StaticStruct(); },
            {
                .OnChange = [DoApplyDataOnlyItems](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    DoApplyDataOnlyItems(Entity, New.Get<FCk_RepData_Inventory_DataOnly_Items>().Items, Old.Get<FCk_RepData_Inventory_DataOnly_Items>().Items);
                },
                .OnAdd = [DoApplyDataOnlyItems](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyDataOnlyItems(Entity, Data.Get<FCk_RepData_Inventory_DataOnly_Items>().Items, TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>{});
                }
            });
    }
} GInventory_DataOnly_RepHandlerRegistrar;
