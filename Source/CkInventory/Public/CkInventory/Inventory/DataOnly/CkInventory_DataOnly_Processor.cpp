#include "CkInventory_DataOnly_Processor.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_ReplicateOnRestore);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_SyncReplication);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_DataOnly_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams) const
        -> void
    {
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        if (InHandle.Has<FTag_Inventory_DataOnly_RestoreReplicated>())
        { return; }

        // Spatial inventories re-drive through their own sibling (FProcessor_Inventory_Spatial_ReplicateOnRestore).
        if (InParams.Get_InventoryType() != ECk_InventoryType::DataOnly)
        { return; }

        // Re-derive BEFORE the driver gate: even a never-replicated restored inventory needs its
        // shape tag and PreviousItems cache back for the request/query paths. Idempotent across
        // retries.
        InHandle.AddOrGet<FTag_Inventory_DataOnly>();
        InHandle.AddOrGet<FFragment_Inventory_PreviousItems>();

        // Owner-hosted container: the driver rides the LIFETIME OWNER (see CreateInventory /
        // FProcessor_Inventory_DataOnly_Replicate). Not re-established yet -> retry next tick.
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(LifetimeOwner))
        { return; }

        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Inventory_DataOnly_Items>(LifetimeOwner);

        // Re-replicate the restored ITEM entities: the wire entries only carry item HANDLES, so the
        // client materializes items from each item's own replication driver + construction info —
        // none of which survives a save (items are born via Request_BuildAndReplicate). Skip items
        // whose driver already exists (idempotent across retries); hold the done tag back while any
        // item's own lifetime-owner driver is still pending so the whole body retries next tick.
        auto AllItemsRelinked = true;
        const auto& Items = FInventoryItemRecord::Get_ValidEntries(InHandle);
        ck::inventory::Display(TEXT("[RESTORE_DEBUG] DataOnly ReplicateOnRestore [{}]: relinking [{}] restored item(s)"),
            InHandle, Items.Num());
        algo::ForEachIsValid(Items, [&](const FCk_Handle_Item& InItemHandle)
        {
            auto Item = FCk_Handle{InItemHandle};
            if (UCk_Utils_EntityReplicationDriver_UE::Has(Item))
            {
                ck::inventory::Display(TEXT("[RESTORE_DEBUG] item [{}] already has a driver — skipping"), Item);
                return;
            }

            const auto* Definition = UCk_Utils_Item_UE::Get_Definition(InItemHandle);
            if (ck::Is_NOT_Valid(Definition))
            {
                // Definition asset gone (skip-if-missing restore) — the item can never reach
                // clients; leave it server-side-only rather than blocking the inventory.
                ck::inventory::Warning(TEXT("[RESTORE_DEBUG] item [{}] restored with NO definition — cannot re-replicate; left server-side-only"), Item);
                return;
            }

            // The item's direct lifetime owner is the INVENTORY entity (driverless); the replication
            // owner is the nearest ancestor in the ownership chain that carries a driver (the
            // actor-backed subject) — same entity the original Create targeted.
            auto ItemOwner = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(Item,
                [](const FCk_Handle& InAncestor)
                {
                    return UCk_Utils_EntityReplicationDriver_UE::Has(InAncestor);
                });
            if (ck::Is_NOT_Valid(ItemOwner))
            {
                ck::inventory::Display(TEXT("[RESTORE_DEBUG] item [{}] has no driver-carrying ancestor yet — retrying next tick"), Item);
                AllItemsRelinked = false;
                return;
            }

            ck::inventory::Display(TEXT("[RESTORE_DEBUG] re-replicating item [{}] (definition [{}]) under owner [{}]"),
                Item, Definition->GetName(), ItemOwner);

            auto ConstructionInfo = FCk_EntityReplicationDriver_ConstructionInfo{Definition->GetClass()};
            ConstructionInfo.Set_ConstructionScriptArchetype(const_cast<UCk_InventoryItem_Definition*>(Definition));

            UCk_Utils_EntityReplicationDriver_UE::Request_TryReplicateExisting(
                Item, ItemOwner, TArray<FCk_EntityReplicationDriver_ConstructionInfo>{ConstructionInfo});
        });

        if (NOT AllItemsRelinked)
        { return; }

        InHandle.AddOrGet<FTag_Inventory_MayRequireReplication>();
        InHandle.Add<FTag_Inventory_DataOnly_RestoreReplicated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_DataOnly_Replicate::
        ForEachEntity(
            TimeType,
            HandleType& InHandle,
            const FFragment_Inventory_Params&) -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto& Items = FInventoryItemRecord::Get_ValidEntries(InHandle);

        auto Entries = TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry>{};
        Entries.Reserve(Items.Num());

        algo::ForEachIsValid(Items, [&](const auto& ItemHandle)
        {
            Entries.Emplace(FCk_InventoryItem_DataOnly_ReplicatedEntry(ItemHandle));
        });

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Inventory_DataOnly_Items>(
            LifetimeOwner, [&](FCk_RepData_Inventory_DataOnly_Items& Data)
        {
            Data.Items = MoveTemp(Entries);
        });

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
