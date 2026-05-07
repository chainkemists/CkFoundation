#include "CkInventory_Spatial_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] static struct FInventory_Spatial_RepHandlerRegistrar
{
    FInventory_Spatial_RepHandlerRegistrar()
    {
        const auto DoApplySpatialItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& OldItems)
        {
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            for (auto InventoryHandle : Inventories)
            {
                if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InventoryHandle))
                { continue; }

                InventoryHandle.AddOrGet<ck::FFragment_Inventory_Spatial_SyncReplication>(NewItems, OldItems);
            }
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Inventory_Spatial_Items::StaticStruct(); },
            {
                .OnChange = [DoApplySpatialItems](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    DoApplySpatialItems(Entity, New.Get<FCk_RepData_Inventory_Spatial_Items>().Items, Old.Get<FCk_RepData_Inventory_Spatial_Items>().Items);
                },
                .OnAdd = [DoApplySpatialItems](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplySpatialItems(Entity, Data.Get<FCk_RepData_Inventory_Spatial_Items>().Items, TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>{});
                }
            });
    }
} GInventory_Spatial_RepHandlerRegistrar;
