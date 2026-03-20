#include "CkInventory_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkInventory/InventorySlot/CkInventorySlot_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- SyncReplication (Client-side) ----

    auto
        FProcessor_Inventory_SyncReplication::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_SyncReplication& InSync)
        -> void
    {
        const auto& CurrentEntries  = InSync.Get_ItemsToReplicate();
        const auto& PreviousEntries = InSync.Get_ItemsToReplicate_Previous();

        // No change
        if (CurrentEntries == PreviousEntries)
        { return; }

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        auto ItemsAdded   = TArray<FCk_Handle>{};
        auto ItemsRemoved = TArray<FCk_Handle>{};

        // Compute delta: items to remove
        for (const auto& PrevEntry : PreviousEntries)
        {
            const auto StillPresent = CurrentEntries.ContainsByPredicate([&](const FCk_InventoryItem_ReplicatedEntry& InCurrent)
            {
                return InCurrent.Get_ItemHandle() == PrevEntry.Get_ItemHandle();
            });

            if (NOT StillPresent)
            {
                auto ItemHandle = ck::StaticCast<FCk_Handle_Item>(PrevEntry.Get_ItemHandle());

                // Remove from grid if spatial
                if (UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle))
                {
                    UCk_Utils_Inventory_UE::DoRemoveItemFromGrid(InHandle, ItemHandle);
                }

                ItemRecordUtils::Request_Disconnect(InHandle, ItemHandle);

                ItemsRemoved.Add(ItemHandle);
            }
        }

        // Compute delta: items to add
        for (const auto& CurrEntry : CurrentEntries)
        {
            const auto WasPresent = PreviousEntries.ContainsByPredicate([&](const FCk_InventoryItem_ReplicatedEntry& InPrev)
            {
                return InPrev.Get_ItemHandle() == CurrEntry.Get_ItemHandle();
            });

            if (NOT WasPresent)
            {
                auto ItemHandle = ck::StaticCast<FCk_Handle_Item>(CurrEntry.Get_ItemHandle());

                ItemRecordUtils::Request_Connect(InHandle, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);

                // Place on grid if spatial
                if (UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle) && CurrEntry.Get_Coordinate().X >= 0 && CurrEntry.Get_Coordinate().Y >= 0)
                {
                    UCk_Utils_Inventory_UE::DoPlaceItemOnGrid(InHandle, ItemHandle, CurrEntry.Get_Coordinate());
                }

                ItemsAdded.Add(ItemHandle);
            }
        }

        // Broadcast signal if any items changed
        if (NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty())
        {
            UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, ItemsAdded, ItemsRemoved));
        }

        InHandle.Remove<MarkedDirtyBy>();
    }

    // ---- ProcessSlots (Authority) ----

    auto
        FProcessor_Inventory_ProcessSlots::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            FFragment_Inventory_Requests& InRequestsComp) const
        -> void
    {
        auto ItemsAdded   = TArray<FCk_Handle>{};
        auto ItemsRemoved = TArray<FCk_Handle>{};

        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Inventory_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InParams, InRequest, ItemsAdded, ItemsRemoved);
            }), ck::policy::DontResetContainer{});
        });

        const bool CollectionChanged = NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty();

        if (CollectionChanged)
        {
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InHandle);

            UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, ItemsAdded, ItemsRemoved));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_ProcessSlots::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::AddItemRequestType& InRequest,
            TArray<FCk_Handle>& OutItemsAdded,
            TArray<FCk_Handle>& /*OutItemsRemoved*/)
        -> void
    {
        auto ItemToAdd = InRequest.Get_ItemToAdd();
        auto Result    = ECk_Inventory_ItemAddedOrNot::NotAdded;
        auto ItemHandle = FCk_Handle_Item{};

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnItemAddedOrNot::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, ItemHandle, Result));
            }
        };

        if (ck::Is_NOT_Valid(ItemToAdd))
        {
            inventory::Warning(TEXT("AddItem: Invalid item handle"));
            return;
        }

        ItemHandle = UCk_Utils_InventoryItem_UE::CastChecked(ItemToAdd);

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        // Check if already in record
        if (ItemRecordUtils::Get_ContainsEntry(InHandle, ItemHandle))
        {
            inventory::Warning(TEXT("AddItem: Item [{}] already in inventory [{}]"), ItemHandle, InHandle);
            return;
        }

        // Handle spatial placement
        if (UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle))
        {
            auto PlacementCoord = InRequest.Get_PlacementCoordinate();

            // Auto-place if coordinate is (-1,-1)
            if (PlacementCoord.X < 0 || PlacementCoord.Y < 0)
            {
                PlacementCoord = UCk_Utils_Inventory_UE::Get_FindFirstAvailablePlacement(InHandle, ItemHandle);

                if (PlacementCoord.X < 0 || PlacementCoord.Y < 0)
                {
                    inventory::Warning(TEXT("AddItem: No available placement for item [{}] in spatial inventory [{}]"), ItemHandle, InHandle);
                    return;
                }
            }

            // Validate placement
            if (NOT UCk_Utils_Inventory_UE::Get_CanPlaceItemAt(InHandle, ItemHandle, PlacementCoord))
            {
                inventory::Warning(TEXT("AddItem: Cannot place item [{}] at [{}] in inventory [{}]"),
                    ItemHandle, PlacementCoord, InHandle);
                return;
            }

            UCk_Utils_Inventory_UE::DoPlaceItemOnGrid(InHandle, ItemHandle, PlacementCoord);
        }

        // Add to record
        ItemRecordUtils::Request_Connect(InHandle, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);

        Result = ECk_Inventory_ItemAddedOrNot::Added;
        OutItemsAdded.Add(ItemHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_ProcessSlots::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::RemoveItemRequestType& InRequest,
            TArray<FCk_Handle>& /*OutItemsAdded*/,
            TArray<FCk_Handle>& OutItemsRemoved)
        -> void
    {
        auto ItemToRemove = InRequest.Get_ItemToRemove();
        auto Result       = ECk_Inventory_ItemRemovedOrNot::NotRemoved;
        auto ItemHandle   = FCk_Handle_Item{};

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnItemRemovedOrNot::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, ItemHandle, Result));
            }
        };

        if (ck::Is_NOT_Valid(ItemToRemove))
        {
            inventory::Warning(TEXT("RemoveItem: Invalid item handle"));
            return;
        }

        ItemHandle = UCk_Utils_InventoryItem_UE::CastChecked(ItemToRemove);

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        // Check if actually in record
        if (NOT ItemRecordUtils::Get_ContainsEntry(InHandle, ItemHandle))
        {
            inventory::Warning(TEXT("RemoveItem: Item [{}] not in inventory [{}]"), ItemHandle, InHandle);
            return;
        }

        // Remove from grid if spatial
        if (UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle))
        {
            UCk_Utils_Inventory_UE::DoRemoveItemFromGrid(InHandle, ItemHandle);
        }

        // Remove from record
        ItemRecordUtils::Request_Disconnect(InHandle, ItemHandle);

        Result = ECk_Inventory_ItemRemovedOrNot::Removed;
        OutItemsRemoved.Add(ItemHandle);
    }

    // ---- Replicate (Server-side) ----

    auto
        FProcessor_Inventory_Replicate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Inventory_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams)
        -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        const auto& Items = ItemRecordUtils::Get_ValidEntries(InHandle);
        const auto IsSpatial = UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle);

        // Build replicated entries with spatial coordinates
        TArray<FCk_InventoryItem_ReplicatedEntry> Entries;
        Entries.Reserve(Items.Num());

        for (const auto& ItemHandle : Items)
        {
            auto Coordinate = FIntPoint(-1, -1);

            if (IsSpatial)
            {
                // Find the coordinate of this item by scanning the grid for the first cell referencing it
                if (auto GridHandle = UCk_Utils_Inventory_UE::Get_Grid(InHandle);
                    ck::IsValid(GridHandle))
                {
                    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
                        [&](const FCk_Handle_2dGridCell& InCell)
                    {
                        if (Coordinate.X >= 0)
                        { return; }

                        if (NOT ck::TUtils_InventorySlot_ItemRef::Has(InCell))
                        { return; }

                        if (const auto& StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
                            StoredEntity == ItemHandle)
                        {
                            Coordinate = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);
                        }
                    });
                }
            }

            Entries.Emplace(FCk_InventoryItem_ReplicatedEntry(ItemHandle).Set_Coordinate(Coordinate));
        }

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_InventoryItems>(
            LifetimeOwner, [&](FCk_RepData_InventoryItems& Data)
        {
            Data.Items = MoveTemp(Entries);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
