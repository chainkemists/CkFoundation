#include "CkInventory_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkInventory/InventorySlot/CkInventorySlot_Fragment.h"

#include "CkInventory/Item/CkInventoryItem_Utils.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.inl.h"
#include "CkInventory/Item/ItemFragments/CkItemFragment_Stackable.h"
#include "CkInventory/Item/ItemFragments/CkItemFragment_Stackable_Utils.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

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

        const auto ByItemHandle = &FCk_InventoryItem_ReplicatedEntry::Get_ItemHandle;

        const auto EntriesAdded   = ck::algo::Except(CurrentEntries, PreviousEntries, ByItemHandle);
        const auto EntriesRemoved = ck::algo::Except(PreviousEntries, CurrentEntries, ByItemHandle);

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;
        const auto IsSpatial = UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle);

        auto ItemsAdded   = TArray<FCk_Handle_Item>{};
        auto ItemsRemoved = TArray<FCk_Handle_Item>{};

        // Process removals
        for (const auto& Entry : EntriesRemoved)
        {
            auto ItemHandle = Entry.Get_ItemHandle();

            if (IsSpatial)
            {
                UCk_Utils_Inventory_UE::DoRemoveItemFromGrid(InHandle, ItemHandle);
            }

            ItemRecordUtils::Request_Disconnect(InHandle, ItemHandle);

            ItemsRemoved.Add(ItemHandle);
        }

        // Process additions
        for (const auto& Entry : EntriesAdded)
        {
            auto ItemHandle = Entry.Get_ItemHandle();

            ItemRecordUtils::Request_Connect(InHandle, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);

            if (IsSpatial && Entry.Get_Coordinate().X >= 0 && Entry.Get_Coordinate().Y >= 0)
            {
                UCk_Utils_Inventory_UE::DoPlaceItemOnGrid(InHandle, ItemHandle, Entry.Get_Coordinate());
            }

            ItemsAdded.Add(ItemHandle);
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

    // ---- HandleRequests (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            FFragment_Inventory_Requests& InRequestsComp) const
        -> void
    {
        auto ItemsAdded   = TArray<FCk_Handle_Item>{};
        auto ItemsRemoved = TArray<FCk_Handle_Item>{};

        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Inventory_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InParams, InRequest, ItemsAdded, ItemsRemoved);
            }), ck::policy::DontResetContainer{});
        });

        if ([[maybe_unused]] const bool CollectionChanged = NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty())
        {
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InHandle);

            UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, ItemsAdded, ItemsRemoved));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::AddItemRequestType& InRequest,
            TArray<FCk_Handle_Item>& OutItemsAdded,
            TArray<FCk_Handle_Item>& /*OutItemsRemoved*/)
        -> void
    {
        auto ItemHandle = InRequest.Get_ItemToAdd();
        auto Result = ECk_Inventory_OperationResult_Add::Failed_InvalidItem;

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_Add::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, ItemHandle, Result));
            }
        };

        if (ck::Is_NOT_Valid(ItemHandle))
        {
            inventory::Warning(TEXT("AddItem: Invalid item handle"));
            return;
        }

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        // Check if already in record
        if (ItemRecordUtils::Get_ContainsEntry(InHandle, ItemHandle))
        {
            Result = ECk_Inventory_OperationResult_Add::Failed_ItemAlreadyInInventory;
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
                    Result = ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable;
                    inventory::Warning(TEXT("AddItem: No available placement for item [{}] in spatial inventory [{}]"), ItemHandle, InHandle);
                    return;
                }
            }

            // Validate placement
            if (NOT UCk_Utils_Inventory_UE::Get_CanPlaceItemAt(InHandle, ItemHandle, PlacementCoord))
            {
                Result = ECk_Inventory_OperationResult_Add::Failed_PlacementBlocked;
                inventory::Warning(TEXT("AddItem: Cannot place item [{}] at [{}] in inventory [{}]"),
                    ItemHandle, PlacementCoord, InHandle);
                return;
            }

            UCk_Utils_Inventory_UE::DoPlaceItemOnGrid(InHandle, ItemHandle, PlacementCoord);
        }

        // Add to record
        ItemRecordUtils::Request_Connect(InHandle, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);

        Result = ECk_Inventory_OperationResult_Add::Success;
        OutItemsAdded.Add(ItemHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::RemoveItemRequestType& InRequest,
            TArray<FCk_Handle_Item>& /*OutItemsAdded*/,
            TArray<FCk_Handle_Item>& OutItemsRemoved)
        -> void
    {
        auto ItemHandle = InRequest.Get_ItemToRemove();
        auto Result = ECk_Inventory_OperationResult_Remove::Failed_InvalidItem;

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_Remove::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, ItemHandle, Result));
            }
        };

        if (ck::Is_NOT_Valid(ItemHandle))
        {
            inventory::Warning(TEXT("RemoveItem: Invalid item handle"));
            return;
        }

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        // Check if actually in record
        if (NOT ItemRecordUtils::Get_ContainsEntry(InHandle, ItemHandle))
        {
            Result = ECk_Inventory_OperationResult_Remove::Failed_ItemNotInInventory;
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

        Result = ECk_Inventory_OperationResult_Remove::Success;
        OutItemsRemoved.Add(ItemHandle);
    }

    // ---- StackItems (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::StackItemsRequestType& InRequest,
            TArray<FCk_Handle_Item>& /*OutItemsAdded*/,
            TArray<FCk_Handle_Item>& OutItemsRemoved)
        -> void
    {
        auto SourceItem = InRequest.Get_SourceItem();
        auto TargetItem = InRequest.Get_TargetItem();
        auto Result = ECk_Inventory_OperationResult_Stack::Failed_InvalidSourceItem;

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_Stack::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, SourceItem, TargetItem, Result));
            }
        };

        // ---- Validation ----

        if (ck::Is_NOT_Valid(SourceItem))
        {
            inventory::Warning(TEXT("StackItems: Invalid source item handle"));
            return;
        }

        if (ck::Is_NOT_Valid(TargetItem))
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_InvalidTargetItem;
            inventory::Warning(TEXT("StackItems: Invalid target item handle"));
            return;
        }

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        if (NOT ItemRecordUtils::Get_ContainsEntry(InHandle, SourceItem))
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_SourceNotInInventory;
            inventory::Warning(TEXT("StackItems: Source [{}] not in inventory [{}]"), SourceItem, InHandle);
            return;
        }

        if (NOT ItemRecordUtils::Get_ContainsEntry(InHandle, TargetItem))
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_TargetNotInInventory;
            inventory::Warning(TEXT("StackItems: Target [{}] not in inventory [{}]"), TargetItem, InHandle);
            return;
        }

        if (NOT UCk_Utils_ItemFragment_Stackable_UE::Get_IsStackable(SourceItem) ||
            NOT UCk_Utils_ItemFragment_Stackable_UE::Get_IsStackable(TargetItem))
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_ItemsNotStackable;
            inventory::Warning(TEXT("StackItems: Source [{}] or target [{}] is not stackable"), SourceItem, TargetItem);
            return;
        }

        if (UCk_Utils_InventoryItem_UE::Get_Definition(SourceItem) != UCk_Utils_InventoryItem_UE::Get_Definition(TargetItem))
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_DefinitionMismatch;
            inventory::Warning(TEXT("StackItems: Source [{}] and target [{}] have different definitions"), SourceItem, TargetItem);
            return;
        }

        // ---- Compute transfer amount ----

        const auto SourceCount = UCk_Utils_ItemFragment_Stackable_UE::Get_StackCount(SourceItem);
        const auto TargetCount = UCk_Utils_ItemFragment_Stackable_UE::Get_StackCount(TargetItem);
        const auto MaxTarget   = UCk_Utils_ItemFragment_Stackable_UE::Get_HasMaxStackSize(TargetItem)
            ? UCk_Utils_ItemFragment_Stackable_UE::Get_MaxStackSize(TargetItem)
            : MAX_int32;

        const auto Available = MaxTarget - TargetCount;
        if (Available <= 0)
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_TargetStackFull;
            inventory::Warning(TEXT("StackItems: Target [{}] stack is full"), TargetItem);
            return;
        }

        const auto Requested     = (InRequest.Get_Count() == -1) ? SourceCount : FMath::Min(InRequest.Get_Count(), SourceCount);
        const auto TransferCount = FMath::Min(Requested, Available);

        if (TransferCount <= 0)
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_TargetStackFull;
            return;
        }

        // ---- Mutate stack counts ----

        auto TargetAttr = UCk_Utils_IntegerAttribute_UE::TryGet(TargetItem, TAG_IntegerAttribute_InventoryItem_StackCount);
        UCk_Utils_IntegerAttribute_UE::Request_Override(TargetAttr, TargetCount + TransferCount);

        const auto SourceRemaining = SourceCount - TransferCount;

        if (SourceRemaining <= 0)
        {
            // Source fully consumed — remove from inventory and destroy
            if (UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle))
            {
                UCk_Utils_Inventory_UE::DoRemoveItemFromGrid(InHandle, SourceItem);
            }

            ItemRecordUtils::Request_Disconnect(InHandle, SourceItem);
            OutItemsRemoved.Add(SourceItem);

            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(SourceItem);
        }
        else
        {
            auto SourceAttr = UCk_Utils_IntegerAttribute_UE::TryGet(SourceItem, TAG_IntegerAttribute_InventoryItem_StackCount);
            UCk_Utils_IntegerAttribute_UE::Request_Override(SourceAttr, SourceRemaining);
        }

        Result = ECk_Inventory_OperationResult_Stack::Success;
    }

    // ---- SplitStack (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::SplitStackRequestType& InRequest,
            TArray<FCk_Handle_Item>& OutItemsAdded,
            TArray<FCk_Handle_Item>& /*OutItemsRemoved*/)
        -> void
    {
        auto SourceItem = InRequest.Get_SourceItem();
        auto NewItem    = FCk_Handle_Item{};
        auto Result     = ECk_Inventory_OperationResult_Split::Failed_InvalidSourceItem;

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_Split::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, SourceItem, NewItem, Result));
            }
        };

        // ---- Validation ----

        if (ck::Is_NOT_Valid(SourceItem))
        {
            inventory::Warning(TEXT("SplitStack: Invalid source item handle"));
            return;
        }

        using ItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

        if (NOT ItemRecordUtils::Get_ContainsEntry(InHandle, SourceItem))
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_SourceNotInInventory;
            inventory::Warning(TEXT("SplitStack: Source [{}] not in inventory [{}]"), SourceItem, InHandle);
            return;
        }

        if (NOT UCk_Utils_ItemFragment_Stackable_UE::Get_IsStackable(SourceItem))
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_ItemNotStackable;
            inventory::Warning(TEXT("SplitStack: Source [{}] is not stackable"), SourceItem);
            return;
        }

        const auto CurrentCount = UCk_Utils_ItemFragment_Stackable_UE::Get_StackCount(SourceItem);
        const auto SplitCount   = InRequest.Get_SplitCount();

        if (SplitCount < 1 || SplitCount >= CurrentCount)
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_InsufficientCount;
            inventory::Warning(TEXT("SplitStack: Invalid split count [{}] for source [{}] with count [{}]"),
                SplitCount, SourceItem, CurrentCount);
            return;
        }

        // ---- Create new item from same definition ----

        auto* Definition = UCk_Utils_InventoryItem_UE::Get_Definition(SourceItem);
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        NewItem = UCk_Utils_InventoryItem_UE::Create(LifetimeOwner, Definition);

        if (ck::Is_NOT_Valid(NewItem))
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_InvalidSourceItem;
            inventory::Warning(TEXT("SplitStack: Failed to create new item from definition"));
            return;
        }

        // ---- Handle spatial placement for new item ----

        if (UCk_Utils_Inventory_UE::Get_IsSpatial(InHandle))
        {
            auto PlacementCoord = InRequest.Get_PlacementCoordinate();

            if (PlacementCoord.X < 0 || PlacementCoord.Y < 0)
            {
                PlacementCoord = UCk_Utils_Inventory_UE::Get_FindFirstAvailablePlacement(InHandle, NewItem);
            }

            if (PlacementCoord.X < 0 || PlacementCoord.Y < 0 ||
                NOT UCk_Utils_Inventory_UE::Get_CanPlaceItemAt(InHandle, NewItem, PlacementCoord))
            {
                Result = ECk_Inventory_OperationResult_Split::Failed_NoSpaceForNewItem;
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                NewItem = {};
                inventory::Warning(TEXT("SplitStack: No space for new item in spatial inventory [{}]"), InHandle);
                return;
            }

            UCk_Utils_Inventory_UE::DoPlaceItemOnGrid(InHandle, NewItem, PlacementCoord);
        }

        // ---- Override stack counts ----

        auto NewItemAttr = UCk_Utils_IntegerAttribute_UE::TryGet(NewItem, TAG_IntegerAttribute_InventoryItem_StackCount);
        UCk_Utils_IntegerAttribute_UE::Request_Override(NewItemAttr, SplitCount);

        auto SourceAttr = UCk_Utils_IntegerAttribute_UE::TryGet(SourceItem, TAG_IntegerAttribute_InventoryItem_StackCount);
        UCk_Utils_IntegerAttribute_UE::Request_Override(SourceAttr, CurrentCount - SplitCount);

        // ---- Add new item to inventory record ----

        ItemRecordUtils::Request_Connect(InHandle, NewItem, ECk_Record_LabelRequirementPolicy::Optional);
        OutItemsAdded.Add(NewItem);

        Result = ECk_Inventory_OperationResult_Split::Success;
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
