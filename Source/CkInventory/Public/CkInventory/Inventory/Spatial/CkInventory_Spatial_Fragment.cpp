#include "CkInventory_Spatial_Fragment.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Utils.h" // UCk_Utils_Inventory_Spatial_UE (Has/Cast/Get_ItemPlacement*) — Produce
#include "CkCore/Algorithms/CkAlgorithms.h"                          // ck::algo::ForEachIsValid — Produce

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"              // Request_TransferLifetimeOwner — load hydration

// --------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] static struct FInventory_Spatial_RepHandlerRegistrar
{
    FInventory_Spatial_RepHandlerRegistrar()
    {
        // Load-path authority hydration: the payload is keyed on the INVENTORY entity, but the net Apply is
        // owner-keyed and never resolves it here. Connect each saved item into THIS inventory's item record and
        // re-place it on the grid; the authority rebuilds its inventory graph and clients converge via the
        // ordinary SyncReplication path. All-or-nothing: if any item handle is not yet rebuilt, retry.
        const auto DoHydrateSpatialItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& NewItems) -> ECk_Persistence_ApplyResult
        {
            if (NOT UCk_Utils_Inventory_Spatial_UE::Has(Entity))
            { return ECk_Persistence_ApplyResult::NotReady; }

            if (ck::algo::AnyOf(NewItems, [](const FCk_InventoryItem_Spatial_ReplicatedEntry& InEntry)
                { return ck::Is_NOT_Valid(InEntry.Get_ItemHandle()); }))
            { return ECk_Persistence_ApplyResult::NotReady; }

            auto SpatialInventory = UCk_Utils_Inventory_Spatial_UE::Cast(Entity);
            for (const auto& NewEntry : NewItems)
            {
                auto ItemHandle = NewEntry.Get_ItemHandle();
                UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
                    Entity, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);
                UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(ItemHandle, Entity);
                if (NewEntry.Get_Coordinate().X >= 0 && NewEntry.Get_Coordinate().Y >= 0)
                {
                    UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
                        SpatialInventory, ItemHandle, NewEntry.Get_Coordinate(), NewEntry.Get_Rotation());
                }
            }

            // Re-arm replication so the authority pushes the rebuilt item list + placements to clients: the
            // load-path connect/place above is server-local, and without this the fresh post-travel client's
            // container stays at the empty Construct-time initial value (it converges via the ordinary ClientOnly
            // SyncReplication — which carries coordinate + rotation — once pushed).
            auto InventoryHandle = UCk_Utils_Inventory_UE::Cast(Entity);
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InventoryHandle);
            return ECk_Persistence_ApplyResult::Applied;
        };

        // Stamps the sync fragment consumed by the Spatial SyncReplication processor (which owns
        // the actual diff/apply). NotReady until at least one Spatial inventory is composed.
        const auto DoApplySpatialItems = [](FCk_Handle& Entity, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& NewItems, const TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>& OldItems) -> ECk_Persistence_ApplyResult
        {
            const auto Inventories = UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Get_ValidEntries(Entity);

            auto Result = ECk_Persistence_ApplyResult::NotReady;

            for (auto InventoryHandle : Inventories)
            {
                if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InventoryHandle))
                { continue; }

                InventoryHandle.AddOrGet<ck::FFragment_Inventory_Spatial_SyncReplication>(NewItems, OldItems);
                Result = ECk_Persistence_ApplyResult::Applied;
            }

            return Result;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_Inventory_Spatial_Items>({
                // Produce-only capture (Phase 3A.4): mirror FProcessor_Inventory_Spatial_Replicate's live-state
                // build. Keyed on the INVENTORY entity — emits THAT inventory's own items (the Replicate build reads
                // the inventory's item record; the container's owner-hosted storage is irrelevant to the build).
                // Produce is capture-only.
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
                .NetApply = [DoApplySpatialItems](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    return DoApplySpatialItems(Entity,
                        New.Get<FCk_RepData_Inventory_Spatial_Items>().Items,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_Inventory_Spatial_Items>().Items
                            : TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>{});
                },
                .HydrationApply = [DoHydrateSpatialItems](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    return DoHydrateSpatialItems(Entity, New.Get<FCk_RepData_Inventory_Spatial_Items>().Items);
                }});
    }
} GInventory_Spatial_RepHandlerRegistrar;
