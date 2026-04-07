#include "CkInventory_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkInventory/InventorySlot/CkInventorySlot_Fragment.h"

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"

#include "CkCore/Technique/CkTechnique.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_SyncReplication);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Replicate);

// ============================================================================
// Processors
// ============================================================================

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

        if (CurrentEntries == PreviousEntries)
        { return; }

        const auto ByItemHandle = &FCk_InventoryItem_ReplicatedEntry::Get_ItemHandle;

        const auto EntriesAdded   = algo::Except(CurrentEntries, PreviousEntries, ByItemHandle);
        const auto EntriesRemoved = algo::Except(PreviousEntries, CurrentEntries, ByItemHandle);

        auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);

        auto ItemsAdded   = TArray<FCk_Handle_Item>{};
        auto ItemsRemoved = TArray<FCk_Handle_Item>{};

        for (const auto& Entry : EntriesRemoved)
        {
            auto ItemHandle = Entry.Get_ItemHandle();

            if (ck::IsValid(SpatialHandle))
            {
                UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(SpatialHandle, ItemHandle);
            }

            FInventoryItemRecordUtils::Request_Disconnect(InHandle, ItemHandle);

            // ---- Only clear parent if it still points to this inventory (order-safe for transfers) ----

            if (ck::IsValid(ItemHandle) && TUtils_Item_ParentInventory::Has(ItemHandle))
            {
                if (TUtils_Item_ParentInventory::Get_StoredEntity(ItemHandle) == InHandle)
                {
                    TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, FCk_Handle_Inventory());
                }
            }

            ItemsRemoved.Add(ItemHandle);
        }

        for (const auto& Entry : EntriesAdded)
        {
            auto ItemHandle = Entry.Get_ItemHandle();

            FInventoryItemRecordUtils::Request_Connect(InHandle, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);
            TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, InHandle);

            if (ck::IsValid(SpatialHandle) && Entry.Get_Coordinate().X >= 0 && Entry.Get_Coordinate().Y >= 0)
            {
                UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(SpatialHandle, ItemHandle, Entry.Get_Coordinate());
            }

            ItemsAdded.Add(ItemHandle);
        }

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
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Inventory_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InParams, InRequest);
            }), ck::policy::DontResetContainer{});
        });

        UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::AddItemRequestType& InRequest)
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

        Result = UCk_Utils_Inventory_UE::Get_CanAcceptItem(InHandle, ItemHandle);

        if (Result != ECk_Inventory_OperationResult_Add::Success)
        {
            inventory::Warning(TEXT("AddItem: Failed [{}] for item [{}] in inventory [{}]"),
                Result, ItemHandle, InHandle);
            return;
        }

        if (auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);
            ck::IsValid(SpatialHandle))
        {
            auto PlacementCoord = InRequest.Get_PlacementCoordinate();

            if (PlacementCoord == ck::Inventory::AutoPlaceCoordinate)
            {
                PlacementCoord = UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(SpatialHandle, ItemHandle);

                if (PlacementCoord == ck::Inventory::AutoPlaceCoordinate)
                {
                    Result = ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable;
                    inventory::Warning(TEXT("AddItem: No available placement for item [{}] in spatial inventory [{}]"), ItemHandle, InHandle);
                    return;
                }
            }

            if (NOT UCk_Utils_Inventory_Spatial_UE::Get_CanPlaceItemAt(SpatialHandle, ItemHandle, PlacementCoord))
            {
                Result = ECk_Inventory_OperationResult_Add::Failed_PlacementBlocked;
                inventory::Warning(TEXT("AddItem: Cannot place item [{}] at [{}] in inventory [{}]"),
                    ItemHandle, PlacementCoord, InHandle);
                return;
            }

            UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(SpatialHandle, ItemHandle, PlacementCoord);
        }

        DoBindItemToInventory(InHandle, ItemHandle);

        Result = ECk_Inventory_OperationResult_Add::Success;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::RemoveItemRequestType& InRequest)
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

        if (NOT FInventoryItemRecordUtils::Get_ContainsEntry(InHandle, ItemHandle))
        {
            Result = ECk_Inventory_OperationResult_Remove::Failed_ItemNotInInventory;
            inventory::Warning(TEXT("RemoveItem: Item [{}] not in inventory [{}]"), ItemHandle, InHandle);
            return;
        }

        if (auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);
            ck::IsValid(SpatialHandle))
        {
            UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(SpatialHandle, ItemHandle);
        }

        DoUnbindItemFromInventory(InHandle, ItemHandle);
        Result = ECk_Inventory_OperationResult_Remove::Success;
    }

    // ---- StackItems (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::StackItemsRequestType& InRequest)
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

        Result = UCk_Utils_ItemTrait_Stackable_UE::Get_CanStackItems(InHandle, SourceItem, TargetItem);

        if (Result != ECk_Inventory_OperationResult_Stack::Success)
        {
            inventory::Warning(TEXT("StackItems: Failed [{}] for source [{}] and target [{}] in inventory [{}]"),
                Result, SourceItem, TargetItem, InHandle);
            return;
        }

        const auto SourceCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(SourceItem);
        const auto TargetCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(TargetItem);
        const auto MaxTarget   = UCk_Utils_ItemTrait_Stackable_UE::Get_HasMaxStackSize(TargetItem)
            ? UCk_Utils_ItemTrait_Stackable_UE::Get_MaxStackSize(TargetItem)
            : MAX_int32;
        const auto Available   = MaxTarget - TargetCount;

        const auto Requested     = (InRequest.Get_Count() == -1) ? SourceCount : FMath::Min(InRequest.Get_Count(), SourceCount);
        const auto TransferCount = FMath::Min(Requested, Available);

        if (TransferCount <= 0)
        {
            Result = ECk_Inventory_OperationResult_Stack::Failed_TargetStackFull;
            return;
        }

        UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(TargetItem, TargetCount + TransferCount);

        if (const auto SourceRemaining = SourceCount - TransferCount;
            SourceRemaining <= 0)
        {
            if (auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);
                ck::IsValid(SpatialHandle))
            {
                UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(SpatialHandle, SourceItem);
            }

            DoUnbindItemFromInventory(InHandle, SourceItem);
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(SourceItem);
        }
        else
        {
            UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(SourceItem, SourceRemaining);
        }

        Result = ECk_Inventory_OperationResult_Stack::Success;
    }

    // ---- SplitStack (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::SplitStackRequestType& InRequest)
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

        if (ck::Is_NOT_Valid(SourceItem))
        {
            inventory::Warning(TEXT("SplitStack: Invalid source item handle"));
            return;
        }

        if (NOT FInventoryItemRecordUtils::Get_ContainsEntry(InHandle, SourceItem))
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_SourceNotInInventory;
            inventory::Warning(TEXT("SplitStack: Source [{}] not in inventory [{}]"), SourceItem, InHandle);
            return;
        }

        if (NOT UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(SourceItem))
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_ItemNotStackable;
            inventory::Warning(TEXT("SplitStack: Source [{}] is not stackable"), SourceItem);
            return;
        }

        const auto CurrentCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(SourceItem);
        const auto SplitCount   = InRequest.Get_SplitCount();

        if (SplitCount < 1 || SplitCount >= CurrentCount)
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_InsufficientCount;
            inventory::Warning(TEXT("SplitStack: Invalid split count [{}] for source [{}] with count [{}]"),
                SplitCount, SourceItem, CurrentCount);
            return;
        }

        auto* Definition = UCk_Utils_Item_UE::Get_Definition(SourceItem);

        // TODO: Revisit when transient entity replication channel actor is available.
        // Create requires a replication driver owner, so use the inventory's context owner.
        // Lifetime ownership is transferred to the inventory immediately after.
        auto ContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);
        NewItem = UCk_Utils_Item_UE::Create(ContextOwner, Definition);

        if (ck::Is_NOT_Valid(NewItem))
        {
            Result = ECk_Inventory_OperationResult_Split::Failed_InvalidSourceItem;
            inventory::Warning(TEXT("SplitStack: Failed to create new item from definition"));
            return;
        }

        Definition->OnSplit(SourceItem, NewItem);

        if (auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);
            ck::IsValid(SpatialHandle))
        {
            auto PlacementCoord = InRequest.Get_PlacementCoordinate();

            if (PlacementCoord == ck::Inventory::AutoPlaceCoordinate)
            {
                PlacementCoord = UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(SpatialHandle, NewItem);
            }

            if (PlacementCoord == ck::Inventory::AutoPlaceCoordinate ||
                NOT UCk_Utils_Inventory_Spatial_UE::Get_CanPlaceItemAt(SpatialHandle, NewItem, PlacementCoord))
            {
                Result = ECk_Inventory_OperationResult_Split::Failed_NoSpaceForNewItem;
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                NewItem = {};
                inventory::Warning(TEXT("SplitStack: No space for new item in spatial inventory [{}]"), InHandle);
                return;
            }

            UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(SpatialHandle, NewItem, PlacementCoord);
        }

        UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, SplitCount);
        UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(SourceItem, CurrentCount - SplitCount);

        DoBindItemToInventory(InHandle, NewItem);

        Result = ECk_Inventory_OperationResult_Split::Success;
    }

    // ---- AddItemByDefinition (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::AddItemByDefinitionRequestType& InRequest)
        -> void
    {
        const auto& Definition = InRequest.Get_Definition();
        const auto Amount   = InRequest.Get_Amount();
        const auto Policy   = InRequest.Get_Policy();

        auto Result       = ECk_Inventory_OperationResult_AddByDefinition::Failed_InvalidDefinition;
        auto AmountAdded  = int32{0};
        auto ItemsCreated = TArray<FCk_Handle_Item>{};
        auto Remaining     = int32{0};
        auto SpatialHandle = FCk_Handle_Inventory_Spatial{};
        auto IsStackable  = false;

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_AddByDefinition::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, Result, AmountAdded, ItemsCreated));
            }
        };

        const auto Validate = [&]() -> EStepResult
        {
            if (ck::Is_NOT_Valid(Definition))
            {
                inventory::Warning(TEXT("AddItemByDefinition: Invalid definition"));
                return EStepResult::Abort;
            }

            if (Amount < 1)
            {
                Result = ECk_Inventory_OperationResult_AddByDefinition::Failed_ZeroAmount;
                inventory::Warning(TEXT("AddItemByDefinition: Amount must be >= 1, got [{}]"), Amount);
                return EStepResult::Abort;
            }

            // TODO: Revisit when transient entity replication channel actor is available.
            // Create requires a replication driver owner, so use the inventory's context owner.
            // Lifetime ownership is transferred to the inventory immediately after.
            SpatialHandle  = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);
            Remaining = Amount;
            IsStackable = Definition->Has_ItemTrait<UCk_ItemTrait_Stackable>();
            return EStepResult::Continue;
        };

        const auto FillExistingStacks = [&]() -> EStepResult
        {
            if (NOT IsStackable || Policy != ECk_Inventory_AddPolicy::PreferStacking)
            { return EStepResult::Continue; }

            const auto Filled = UCk_Utils_ItemTrait_Stackable_UE::Request_FillExistingStacks(InHandle, Definition, Remaining);
            Remaining   -= Filled;
            AmountAdded += Filled;

            return EStepResult::Continue;
        };

        const auto CreateNewItems = [&]() -> EStepResult
        {
            auto ContextOwner  =  UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);
            while (Remaining > 0)
            {
                auto NewItem = UCk_Utils_Item_UE::Create(ContextOwner, Definition);

                if (ck::Is_NOT_Valid(NewItem))
                {
                    inventory::Warning(TEXT("AddItemByDefinition: Failed to create item from definition"));
                    break;
                }

                if (UCk_Utils_Inventory_UE::Get_CanAcceptItem(InHandle, NewItem) != ECk_Inventory_OperationResult_Add::Success)
                {
                    Result = ECk_Inventory_OperationResult_AddByDefinition::Failed_RejectedByCustomAcceptanceLogic;
                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                    break;
                }

                auto CountForThisItem = int32{1};

                if (IsStackable)
                {
                    const auto* StackableTrait = Definition->Get_ItemTrait<UCk_ItemTrait_Stackable>();
                    const auto HasMax = StackableTrait->Get_HasMaxStackSize();
                    const auto MaxStack = HasMax ? StackableTrait->Get_MaxStackSize() : MAX_int32;

                    CountForThisItem = FMath::Min(Remaining, MaxStack);

                    UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, CountForThisItem);
                }

                if (ck::IsValid(SpatialHandle))
                {
                    const auto PlacementCoord = UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(SpatialHandle, NewItem);

                    if (PlacementCoord == Inventory::AutoPlaceCoordinate)
                    {
                        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                        break;
                    }

                    UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(SpatialHandle, NewItem, PlacementCoord);
                }

                DoBindItemToInventory(InHandle, NewItem);
                ItemsCreated.Add(NewItem);

                Remaining   -= CountForThisItem;
                AmountAdded += CountForThisItem;
            }

            return EStepResult::Continue;
        };

        const auto DetermineResult = [&]() -> EStepResult
        {
            if (Remaining <= 0)
            { Result = ECk_Inventory_OperationResult_AddByDefinition::Success_AllAdded; }
            else if (AmountAdded > 0)
            { Result = ECk_Inventory_OperationResult_AddByDefinition::Success_PartiallyAdded; }
            else
            { Result = ECk_Inventory_OperationResult_AddByDefinition::Failed_NoSpaceAvailable; }

            return EStepResult::Continue;
        };

        auto Steps = Technique{}
            .AddStep(Validate)
            .AddStep(FillExistingStacks)
            .AddStep(CreateNewItems)
            .AddStep(DetermineResult);
        Steps.ProcessAllSteps();
    }

    // ---- TransferItem (Authority) ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::TransferItemRequestType& InRequest)
        -> void
    {
        auto SourceItem         = InRequest.Get_SourceItem();
        auto TargetInventory    = InRequest.Get_TargetInventory();
        const auto Count        = InRequest.Get_Count();
        const auto Policy       = InRequest.Get_Policy();
        const auto PlacementCoordinate = InRequest.Get_PlacementCoordinate();

        auto Result             = ECk_Inventory_OperationResult_Transfer::Failed_InvalidSourceItem;
        auto CountTransferred   = int32{0};
        auto IsStackable        = false;
        auto SourceCount        = int32{0};
        auto TransferCount      = int32{0};

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_Transfer::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, SourceItem, TargetInventory,
                                CountTransferred, Result));
            }
        };

        const auto Validate = [&]() -> EStepResult
        {
            if (ck::Is_NOT_Valid(SourceItem))
            {
                inventory::Warning(TEXT("TransferItem: Invalid source item handle"));
                return EStepResult::Abort;
            }

            if (NOT FInventoryItemRecordUtils::Get_ContainsEntry(InHandle, SourceItem))
            {
                Result = ECk_Inventory_OperationResult_Transfer::Failed_SourceNotInInventory;
                inventory::Warning(TEXT("TransferItem: Source [{}] not in inventory [{}]"), SourceItem, InHandle);
                return EStepResult::Abort;
            }

            if (ck::Is_NOT_Valid(TargetInventory))
            {
                Result = ECk_Inventory_OperationResult_Transfer::Failed_InvalidTargetInventory;
                inventory::Warning(TEXT("TransferItem: Invalid target inventory"));
                return EStepResult::Abort;
            }

            if (TargetInventory == InHandle)
            {
                Result = ECk_Inventory_OperationResult_Transfer::Failed_SameInventory;
                inventory::Warning(TEXT("TransferItem: Source and target inventory are the same [{}]"), InHandle);
                return EStepResult::Abort;
            }

            IsStackable = UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(SourceItem);

            if (IsStackable)
            {
                SourceCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(SourceItem);
                TransferCount = (Count == -1) ? SourceCount : FMath::Min(Count, SourceCount);
            }
            else
            {
                SourceCount = 1;
                TransferCount = 1;
            }

            if (TransferCount <= 0)
            {
                Result = ECk_Inventory_OperationResult_Transfer::Failed_ZeroCount;
                return EStepResult::Abort;
            }

            return EStepResult::Continue;
        };

        const auto ResolveTransfer = [&]() -> EStepResult
        {
            if (NOT IsStackable || Policy != ECk_Inventory_AddPolicy::PreferStacking)
            { return EStepResult::Continue; }

            const auto* Definition = UCk_Utils_Item_UE::Get_Definition(SourceItem);
            const auto Filled = UCk_Utils_ItemTrait_Stackable_UE::Request_FillExistingStacks(
                TargetInventory, Definition, TransferCount);

            TransferCount    -= Filled;
            CountTransferred += Filled;
            SourceCount      -= Filled;

            // ---- Deduct from source and destroy if depleted ----

            if (Filled > 0)
            {
                UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(SourceItem, SourceCount);

                if (SourceCount <= 0)
                {
                    const auto RemoveRequest = FFragment_Inventory_Requests::RemoveItemRequestType(SourceItem);
                    DoHandleRequest(InHandle, InParams, RemoveRequest);
                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(SourceItem);
                }
            }

            return EStepResult::Continue;
        };

        const auto ExecuteTransfer = [&]() -> EStepResult
        {
            if (TransferCount <= 0)
            { return EStepResult::Continue; }

            if (TransferCount >= SourceCount)
            {
                // ---- Moving entire item: Remove from source, Add to target ----

                auto RemoveRequest = FFragment_Inventory_Requests::RemoveItemRequestType(SourceItem);
                DoHandleRequest(InHandle, InParams, RemoveRequest);

                auto AddRequest = FFragment_Inventory_Requests::AddItemRequestType(SourceItem);
                AddRequest.Set_PlacementCoordinate(PlacementCoordinate);

                const auto& TargetParams = TargetInventory.Get<FFragment_Inventory_Params>();
                DoHandleRequest(TargetInventory, TargetParams, AddRequest);

                CountTransferred += TransferCount;
                UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(TargetInventory);
            }
            else
            {
                // ---- Partial transfer: split source, create new item, add to target ----

                UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(SourceItem, SourceCount - TransferCount);

                auto* Definition = UCk_Utils_Item_UE::Get_Definition(SourceItem);

                // TODO: Revisit when transient entity replication channel actor is available.
                auto TargetContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(TargetInventory);
                auto NewItem = UCk_Utils_Item_UE::Create(TargetContextOwner, Definition);

                if (ck::Is_NOT_Valid(NewItem))
                {
                    UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(SourceItem, SourceCount);
                    inventory::Warning(TEXT("TransferItem: Failed to create new item for partial transfer"));
                    return EStepResult::Abort;
                }

                Definition->OnSplit(SourceItem, NewItem);
                UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, TransferCount);

                auto AddRequest = FFragment_Inventory_Requests::AddItemRequestType(NewItem);
                AddRequest.Set_PlacementCoordinate(PlacementCoordinate);

                const auto& TargetParams = TargetInventory.Get<FFragment_Inventory_Params>();
                DoHandleRequest(TargetInventory, TargetParams, AddRequest);

                // TODO: Handle AddRequest failure — rollback stack count and destroy NewItem

                CountTransferred += TransferCount;
                UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(TargetInventory);
            }

            return EStepResult::Continue;
        };

        const auto DetermineResult = [&]() -> EStepResult
        {
            if (CountTransferred <= 0)
            {
                if (Result == ECk_Inventory_OperationResult_Transfer::Failed_InvalidSourceItem)
                { Result = ECk_Inventory_OperationResult_Transfer::Failed_NoSpaceInTarget; }
                return EStepResult::Continue;
            }

            const auto RequestedCount = (Count == -1) ? SourceCount : Count;
            Result = (CountTransferred >= RequestedCount)
                ? ECk_Inventory_OperationResult_Transfer::Success
                : ECk_Inventory_OperationResult_Transfer::Success_Partial;

            return EStepResult::Continue;
        };

        auto Steps = Technique{}
            .AddStep(Validate)
            .AddStep(ResolveTransfer)
            .AddStep(ExecuteTransfer)
            .AddStep(DetermineResult);
        Steps.ProcessAllSteps();
    }

    // ---- Sort ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::SortRequestType& InRequest)
        -> void
    {
        auto Result = ECk_Inventory_OperationResult_Sort::Failed_InvalidInventory;

        ON_SCOPE_EXIT
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_Inventory_OnOperationResult_Sort::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(InHandle, Result));
            }
        };

        if (ck::Is_NOT_Valid(InHandle))
        { return; }

        // ---- Build sort predicate from native or dynamic delegate ----

        const auto& NativePredicate = InRequest.Get_SortPredicate();
        const auto& DynamicPredicate = InRequest.Get_SortPredicateDynamic();

        if (const auto HasPredicate = NativePredicate.IsBound() || DynamicPredicate.IsBound(); 
            NOT HasPredicate)
        { return; }

        const auto SortPredicate = [&](const FCk_Handle_Item& A, const FCk_Handle_Item& B) -> bool
        {
            if (NativePredicate.IsBound())
            { return NativePredicate.Execute(A, B); }

            auto ABeforeB = false;
            DynamicPredicate.ExecuteIfBound(A, B, ABeforeB);
            return ABeforeB;
        };

        // ---- DataOnly: sort the record in-place ----

        if (UCk_Utils_Inventory_UE::Get_IsDataOnly(InHandle))
        {
            FInventoryItemRecordUtils::Sort(InHandle, SortPredicate);
            Result = ECk_Inventory_OperationResult_Sort::Success;
            return;
        }

        // ---- Spatial: TODO: tell the grid to sort (clear tilemap + re-place items in sorted order) ----

        if (ck::IsValid(UCk_Utils_Inventory_Spatial_UE::Cast(InHandle)))
        {
            Result = ECk_Inventory_OperationResult_Sort::Success;
            return;
        }
    }

    // ---- Bind / Unbind helpers ----

    auto
        FProcessor_Inventory_HandleRequests::
        DoBindItemToInventory(
            HandleType& InInventory,
            FCk_Handle_Item& InItem)
        -> void
    {
        FInventoryItemRecordUtils::Request_Connect(InInventory, InItem, ECk_Record_LabelRequirementPolicy::Optional);
        TUtils_Item_ParentInventory::AddOrReplace(InItem, InInventory);
        UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(InItem, InInventory);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_HandleRequests::
        DoUnbindItemFromInventory(
            HandleType& InInventory,
            FCk_Handle_Item& InItem)
        -> void
    {
        FInventoryItemRecordUtils::Request_Disconnect(InInventory, InItem);
        TUtils_Item_ParentInventory::AddOrReplace(InItem, FCk_Handle_Inventory());
    }

    // ---- FireSignals (Authority) ----

    auto
        FProcessor_Inventory_FireSignals::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            FFragment_Inventory_PreviousItems& InPreviousItems)
        -> void
    {
        const auto CurrentItems = FInventoryItemRecordUtils::Get_ValidEntries(InHandle);
        const auto& PreviousItems = InPreviousItems.Get_Items();

        const auto ItemsAdded   = ck::algo::Except(CurrentItems, PreviousItems);
        const auto ItemsRemoved = ck::algo::Except(PreviousItems, CurrentItems);

        if (NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty())
        {
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InHandle);

            UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, ItemsAdded, ItemsRemoved));
        }

        InPreviousItems._Items = CurrentItems;
        InHandle.Remove<FTag_Inventory_MayHaveChanged>();
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

        const auto& Items = FInventoryItemRecordUtils::Get_ValidEntries(InHandle);
        auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InHandle);

        TArray<FCk_InventoryItem_ReplicatedEntry> Entries;
        Entries.Reserve(Items.Num());

        for (const auto& ItemHandle : Items)
        {
            auto Coordinate = ck::Inventory::AutoPlaceCoordinate;

            if (ck::IsValid(SpatialHandle))
            {
                if (auto GridHandle = UCk_Utils_Inventory_Spatial_UE::Get_Grid(SpatialHandle);
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
