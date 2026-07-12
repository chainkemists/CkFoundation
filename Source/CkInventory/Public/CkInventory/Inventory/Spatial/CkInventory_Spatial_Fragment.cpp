#include "CkInventory_Spatial_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Utils.h" // UCk_Utils_Inventory_Spatial_UE (Has/Cast/Get_ItemPlacement*) — Produce
#include "CkCore/Algorithms/CkAlgorithms.h"                          // ck::algo::ForEachIsValid — Produce

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
                },
                // Produce-only capture (Phase 3A.4): mirror FProcessor_Inventory_Spatial_Replicate's live-state
                // build. Keyed on the INVENTORY entity — emits THAT inventory's own items (the Replicate build reads
                // the inventory's item record; the container's owner-hosted storage is irrelevant to the build). NO
                // SeedContainer — the live ReplicateOnRestore still seeds under Model A.
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT UCk_Utils_Inventory_Spatial_UE::Has(Entity))
                    { return {}; }

                    auto Inventory = UCk_Utils_Inventory_Spatial_UE::Cast(Entity);
                    const auto& Items = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Get_ValidEntries(Inventory);

                    auto Entries = TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>{};
                    Entries.Reserve(Items.Num());

                    ck::algo::ForEachIsValid(Items, [&](const FCk_Handle_Item& InItemHandle)
                    {
                        const auto Coordinate = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementCoordinate(Inventory, InItemHandle);
                        const auto Rotation   = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementRotation(InItemHandle);

                        Entries.Emplace(FCk_InventoryItem_Spatial_ReplicatedEntry(InItemHandle)
                            .Set_Coordinate(Coordinate)
                            .Set_Rotation(Rotation));
                    });

                    auto Data = FCk_RepData_Inventory_Spatial_Items{};
                    Data.Items = MoveTemp(Entries);
                    return FInstancedStruct::Make(Data);
                },
                .Transport = ECk_PersistenceTransport::NetAndSave
            });
    }
} GInventory_Spatial_RepHandlerRegistrar;
