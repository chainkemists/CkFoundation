#include "CkInventory_DataOnly_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Utils.h" // UCk_Utils_Inventory_DataOnly_UE::Has — Produce
#include "CkCore/Algorithms/CkAlgorithms.h"                            // ck::algo::ForEachIsValid — Produce

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"              // Request_TransferLifetimeOwner — load hydration

// --------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] static struct FInventory_DataOnly_RepHandlerRegistrar
{
    FInventory_DataOnly_RepHandlerRegistrar()
    {
        // The payload is keyed on the INVENTORY entity, which the owner-keyed net Apply never resolves —
        // so the load path connects the saved items into THIS inventory itself; clients still converge
        // through the ordinary SyncReplication path. All-or-nothing: any unrebuilt item retries next tick.
        const auto DoHydrateDataOnlyItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& NewItems) -> ECk_Persistence_ApplyResult
        {
            if (NOT UCk_Utils_Inventory_DataOnly_UE::Has(Entity))
            { return ECk_Persistence_ApplyResult::NotReady; }

            if (ck::algo::AnyOf(NewItems, [](const FCk_InventoryItem_DataOnly_ReplicatedEntry& InEntry)
                { return ck::Is_NOT_Valid(InEntry.Get_ItemHandle()); }))
            { return ECk_Persistence_ApplyResult::NotReady; }

            for (const auto& NewEntry : NewItems)
            {
                auto ItemHandle = NewEntry.Get_ItemHandle();
                UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
                    Entity, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);
                UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(ItemHandle, Entity);
            }

            // The connect above is server-local; without re-arming, a fresh post-travel client's container
            // stays at its empty Construct-time value forever.
            auto InventoryHandle = UCk_Utils_Inventory_UE::Cast(Entity);
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InventoryHandle);
            return ECk_Persistence_ApplyResult::Applied;
        };

        // Only stamps the sync fragment — the DataOnly SyncReplication processor owns the diff/apply.
        const auto DoApplyDataOnlyItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>& OldItems) -> ECk_Persistence_ApplyResult
        {
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            auto Result = ECk_Persistence_ApplyResult::NotReady;

            for (auto InventoryHandle : Inventories)
            {
                if (NOT UCk_Utils_Inventory_UE::Get_IsDataOnly(InventoryHandle))
                { continue; }

                InventoryHandle.AddOrGet<ck::FFragment_Inventory_DataOnly_SyncReplication>(NewItems, OldItems);
                Result = ECk_Persistence_ApplyResult::Applied;
            }

            return Result;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_Inventory_DataOnly_Items>({
                // Keyed on the INVENTORY entity — emits THAT inventory's items. Capture-only, no mutation.
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
                .NetApply = [DoApplyDataOnlyItems](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    return DoApplyDataOnlyItems(Entity,
                        New.Get<FCk_RepData_Inventory_DataOnly_Items>().Items,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_Inventory_DataOnly_Items>().Items
                            : TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>{});
                },
                .HydrationApply = [DoHydrateDataOnlyItems](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    return DoHydrateDataOnlyItems(Entity, New.Get<FCk_RepData_Inventory_DataOnly_Items>().Items);
                }});
    }
} GInventory_DataOnly_RepHandlerRegistrar;
