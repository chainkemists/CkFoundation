#include "CkInventory_DataOnly_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Utils.h" // UCk_Utils_Inventory_DataOnly_UE::Has — Produce
#include "CkCore/Algorithms/CkAlgorithms.h"                            // ck::algo::ForEachIsValid — Produce

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
                },
                // Produce-only capture (Phase 3A.4): mirror FProcessor_Inventory_DataOnly_Replicate's live-state
                // build. Keyed on the INVENTORY entity — emits THAT inventory's items (entries carry only the item
                // handle). NO SeedContainer — Produce is capture/oracle-only; the Model-A re-drive was retired in Phase 5.
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT UCk_Utils_Inventory_DataOnly_UE::Has(Entity))
                    { return {}; }

                    const auto& Items = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Get_ValidEntries(Entity);

                    auto Entries = TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>{};
                    Entries.Reserve(Items.Num());

                    ck::algo::ForEachIsValid(Items, [&](const FCk_Handle_Item& InItemHandle)
                    {
                        Entries.Emplace(FCk_InventoryItem_DataOnly_ReplicatedEntry(InItemHandle));
                    });

                    auto Data = FCk_RepData_Inventory_DataOnly_Items{};
                    Data.Items = MoveTemp(Entries);
                    return FInstancedStruct::Make(Data);
                },
                .Transport = ECk_PersistenceTransport::NetAndSave
            });
    }
} GInventory_DataOnly_RepHandlerRegistrar;
