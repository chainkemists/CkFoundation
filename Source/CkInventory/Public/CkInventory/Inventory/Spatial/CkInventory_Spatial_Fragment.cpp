#include "CkInventory_Spatial_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] static struct FInventory_Spatial_RepHandlerRegistrar
{
    FInventory_Spatial_RepHandlerRegistrar()
    {
        // Stamps the sync fragment consumed by the Spatial SyncReplication processor (which owns
        // the actual diff/apply). NotReady until at least one Spatial inventory is composed.
        const auto DoApplySpatialItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& OldItems) -> ECk_RepFragment_ApplyResult
        {
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            auto Result = ECk_RepFragment_ApplyResult::NotReady;

            for (auto InventoryHandle : Inventories)
            {
                if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InventoryHandle))
                { continue; }

                InventoryHandle.AddOrGet<ck::FFragment_Inventory_Spatial_SyncReplication>(NewItems, OldItems);
                Result = ECk_RepFragment_ApplyResult::Applied;
            }

            return Result;
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Inventory_Spatial_Items::StaticStruct(); },
            {
                .Apply = [DoApplySpatialItems](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    return DoApplySpatialItems(Entity,
                        New.Get<FCk_RepData_Inventory_Spatial_Items>().Items,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_Inventory_Spatial_Items>().Items
                            : TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>{});
                }
            });
    }
} GInventory_Spatial_RepHandlerRegistrar;
