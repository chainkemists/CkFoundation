#include "CkInventory_Spatial_Processor.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"
#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/Dimensions/CkItemTrait_Dimensions.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_ReplicateOnRestore);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_SyncReplication);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    auto
        FProcessor_Inventory_Spatial_SyncReplication::
        OnEntryAdded(
            FCk_Handle_Inventory_Spatial& InHandle,
            const FCk_InventoryItem_Spatial_ReplicatedEntry& InEntry,
            FCk_Handle_Item& InItem) -> void
    {
        if (InEntry.Get_Coordinate().X >= 0 && InEntry.Get_Coordinate().Y >= 0)
        {
            UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
                InHandle, InItem, InEntry.Get_Coordinate(), InEntry.Get_Rotation());
        }
    }

    auto
        FProcessor_Inventory_Spatial_SyncReplication::
        OnEntryRemoved(
            FCk_Handle_Inventory_Spatial& InHandle,
            FCk_Handle_Item& InItem) -> void
    {
        UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(InHandle, InItem);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_Spatial_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams) const
        -> void
    {
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        if (InHandle.Has<FTag_Inventory_Spatial_RestoreReplicated>())
        { return; }

        if (InParams.Get_InventoryType() != ECk_InventoryType::Spatial)
        { return; }

        // ---- Pre-driver re-derives (idempotent across retries) ----
        // Even a never-replicated restored inventory needs its shape tag, the PreviousItems diff
        // cache, and a WORKING GRID back for the request/query paths.

        InHandle.AddOrGet<FTag_Inventory_Spatial>();
        InHandle.AddOrGet<FFragment_Inventory_PreviousItems>();

        // The inventory's 2dGridSystem cannot survive a save (its cells live in a PRIVATE nested
        // registry) — re-compose it from the restored Params exactly like Setup_PerShape does on
        // first composition.
        if (NOT UCk_Utils_2dGridSystem_UE::Has(InHandle))
        {
            auto TransformHandle = UCk_Utils_Transform_UE::Has(InHandle)
                ? UCk_Utils_Transform_UE::CastChecked(InHandle)
                : UCk_Utils_Transform_UE::Add(InHandle, FTransform::Identity);

            const auto GridParams = FCk_Fragment_2dGridSystem_ParamsData(
                InParams.Get_Dimensions(),
                FVector2D(1.0, 1.0));

            auto Grid = UCk_Utils_2dGridSystem_UE::Add(TransformHandle, GridParams);

            UCk_Utils_2dGridSystem_UE::ForEach_Cell(Grid, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
                [&](FCk_Handle_2dGridCell InCell)
            {
                ck::TUtils_InventorySlot_ItemRef::Clear(InCell);
            });

            ck::inventory::Display(TEXT("[RESTORE_DEBUG] Spatial ReplicateOnRestore [{}]: re-composed [{}x{}] grid"),
                InHandle, InParams.Get_Dimensions().X, InParams.Get_Dimensions().Y);
        }

        auto SpatialInventory = UCk_Utils_Inventory_Spatial_UE::CastChecked(InHandle);

        // ---- Per-item re-compose + re-stamp (idempotent across retries) ----
        // Construct is abstained during reconstitution, so item traits never re-ran: re-build each
        // item's Dimensions-trait shape grid from its restored Definition, then re-stamp the item
        // onto the rebuilt inventory grid from its restored placement decision record
        // (FFragment_Item_SpatialPlacement). Items without the record were never placed — skip.

        const auto& Items = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Get_ValidEntries(InHandle);
        algo::ForEachIsValid(Items, [&](const FCk_Handle_Item& InItemHandle)
        {
            auto Item = InItemHandle;

            const auto* Definition = UCk_Utils_Item_UE::Get_Definition(InItemHandle);

            if (NOT UCk_Utils_2dGridSystem_UE::Has(Item))
            {
                if (ck::Is_NOT_Valid(Definition))
                {
                    ck::inventory::Warning(TEXT("[RESTORE_DEBUG] Spatial item [{}] restored with NO definition — "
                        "cannot re-compose its shape grid"), Item);
                    return;
                }

                if (const auto* DimensionsTrait = Definition->Get_ItemTrait<UCk_ItemTrait_Dimensions>();
                    DimensionsTrait != nullptr)
                {
                    auto ItemBase = FCk_Handle{Item};
                    DimensionsTrait->Construct(ItemBase);
                }
            }

            if (NOT Item.Has<FFragment_Item_SpatialPlacement>())
            { return; }

            // Copy first — Request_PlaceItemOnGrid rewrites the fragment it reads from.
            const auto Placement = Item.Get<FFragment_Item_SpatialPlacement>();
            UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
                SpatialInventory, InItemHandle, Placement.Get_Anchor(), Placement.Get_Rotation());
        });

        // ---- Owner-hosted container: the driver rides the LIFETIME OWNER. Not re-established yet -> retry. ----

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(LifetimeOwner))
        { return; }

        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Inventory_Spatial_Items>(LifetimeOwner);

        // ---- Re-replicate the restored ITEM entities (mirrors the DataOnly restore) ----

        auto AllItemsRelinked = true;
        ck::inventory::Display(TEXT("[RESTORE_DEBUG] Spatial ReplicateOnRestore [{}]: relinking [{}] restored item(s)"),
            InHandle, Items.Num());
        algo::ForEachIsValid(Items, [&](const FCk_Handle_Item& InItemHandle)
        {
            auto Item = FCk_Handle{InItemHandle};
            if (UCk_Utils_EntityReplicationDriver_UE::Has(Item))
            { return; }

            const auto* Definition = UCk_Utils_Item_UE::Get_Definition(InItemHandle);
            if (ck::Is_NOT_Valid(Definition))
            {
                ck::inventory::Warning(TEXT("[RESTORE_DEBUG] item [{}] restored with NO definition — cannot re-replicate; left server-side-only"), Item);
                return;
            }

            auto ItemOwner = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(Item,
                [](const FCk_Handle& InAncestor)
                {
                    return UCk_Utils_EntityReplicationDriver_UE::Has(InAncestor);
                });
            if (ck::Is_NOT_Valid(ItemOwner))
            {
                AllItemsRelinked = false;
                return;
            }

            auto ConstructionInfo = FCk_EntityReplicationDriver_ConstructionInfo{Definition->GetClass()};
            ConstructionInfo.Set_ConstructionScriptArchetype(const_cast<UCk_InventoryItem_Definition*>(Definition));

            UCk_Utils_EntityReplicationDriver_UE::Request_TryReplicateExisting(
                Item, ItemOwner, TArray<FCk_EntityReplicationDriver_ConstructionInfo>{ConstructionInfo});
        });

        if (NOT AllItemsRelinked)
        { return; }

        InHandle.AddOrGet<FTag_Inventory_MayRequireReplication>();
        InHandle.Add<FTag_Inventory_Spatial_RestoreReplicated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_Spatial_Replicate::
        ForEachEntity(
            TimeType,
            HandleType& InHandle,
            const FFragment_Inventory_Params&) -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto& Items = FInventoryItemRecord::Get_ValidEntries(InHandle);

        auto Entries = TArray<FCk_InventoryItem_Spatial_ReplicatedEntry>{};
        Entries.Reserve(Items.Num());

        algo::ForEachIsValid(Items, [&](const auto& ItemHandle)
        {
            const auto Coordinate = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementCoordinate(InHandle, ItemHandle);
            const auto Rotation   = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementRotation(ItemHandle);

            Entries.Emplace(FCk_InventoryItem_Spatial_ReplicatedEntry(ItemHandle)
                .Set_Coordinate(Coordinate)
                .Set_Rotation(Rotation));
        });

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Inventory_Spatial_Items>(
            LifetimeOwner, [&](FCk_RepData_Inventory_Spatial_Items& Data)
        {
            Data.Items = MoveTemp(Entries);
        });

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------