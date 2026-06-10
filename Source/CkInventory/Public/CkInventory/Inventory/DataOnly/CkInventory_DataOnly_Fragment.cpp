#include "CkInventory_DataOnly_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] static struct FInventory_DataOnly_RepHandlerRegistrar
{
    FInventory_DataOnly_RepHandlerRegistrar()
    {
        // Stamps the sync fragment consumed by the DataOnly SyncReplication processor (which owns
        // the actual diff/apply). NotReady until at least one DataOnly inventory is composed.
        const auto DoApplyDataOnlyItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& OldItems) -> ECk_RepFragment_ApplyResult
        {
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            auto Result = ECk_RepFragment_ApplyResult::NotReady;

            for (auto InventoryHandle : Inventories)
            {
                if (NOT UCk_Utils_Inventory_UE::Get_IsDataOnly(InventoryHandle))
                { continue; }

                InventoryHandle.AddOrGet<ck::FFragment_Inventory_DataOnly_SyncReplication>(NewItems, OldItems);
                Result = ECk_RepFragment_ApplyResult::Applied;
            }

            return Result;
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Inventory_DataOnly_Items::StaticStruct(); },
            {
                .Apply = [DoApplyDataOnlyItems](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    return DoApplyDataOnlyItems(Entity,
                        New.Get<FCk_RepData_Inventory_DataOnly_Items>().Items,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_Inventory_DataOnly_Items>().Items
                            : TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>{});
                }
            });
    }
} GInventory_DataOnly_RepHandlerRegistrar;
