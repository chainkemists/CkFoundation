#include "CkInventory_Spatial_RequestTraits.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Utils.h"
#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

namespace ck::inventory_handlers
{
    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    // ----------------------------------------------------------------------------------------------------

    // ---- TAddItem<Spatial, FCk_SpatialPlacement>: validate + placement (auto or fixed) + place + bind.
    template <>
    auto TAddItem<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>::Handle(
        FCk_Handle_Inventory_Spatial& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest   = InEntry.Common;
        const auto& InPlacement = InEntry.Addon;
        auto ItemHandle = InRequest.Get_ItemToAdd();
        auto R = Result::Failed_InvalidItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Add>(
            InRequest, [&]{ return MakePayload(Base, ItemHandle, R); });

        if (InRequest.Get_Acceptance() == ECk_AddAcceptance::Validate)
        {
            R = UCk_Utils_Inventory_UE::Get_CanAcceptItem(Base, ItemHandle);
            if (R != Result::Success)
            {
                // Caller-attributable rejection — Display, never Warning: the AutoTest harness
                // escalates Warnings to test failures.
                ck::inventory::Display(TEXT("AddItem_Spatial: Failed [{}] for item [{}] in inventory [{}]"),
                    R, ItemHandle, InHandle);
                return R;
            }
        }

        if (NOT UCk_Utils_2dGridSystem_UE::Has(ItemHandle))
        {
            R = Result::Failed_MissingDimensionsTrait;
            ck::inventory::Warning(TEXT("AddItem_Spatial: Item [{}] is missing a Dimensions trait and cannot be placed in spatial inventory [{}]"),
                ItemHandle, InHandle);
            return R;
        }

        const auto Coord = InPlacement.Get_Coordinate();
        const auto Placement = (Coord == ck::Inventory::AutoPlaceCoordinate)
            ? UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(InHandle, ItemHandle)
            : UCk_Utils_Inventory_Spatial_UE::Get_CanPlaceItemAt(InHandle, ItemHandle, Coord);

        if (NOT Placement.Get_Succeeded())
        {
            R = (Coord == ck::Inventory::AutoPlaceCoordinate)
                ? Result::Failed_NoSpaceAvailable
                : Result::Failed_PlacementBlocked;
            ck::inventory::Warning(TEXT("AddItem_Spatial: Cannot place item [{}] in spatial inventory [{}]"),
                ItemHandle, InHandle);
            return R;
        }

        UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
            InHandle, ItemHandle, Placement.Get_Coordinate(), Placement.Get_Rotation());

        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
            Base, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);
        TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, Base);
        UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(ItemHandle, Base);
        R = Result::Success;
        return R;
    }

    // ---- TRemoveItem<Spatial>: validate + remove from grid + unbind.
    template <>
    auto TRemoveItem<FCk_Handle_Inventory_Spatial>::Handle(
        FCk_Handle_Inventory_Spatial& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto ItemHandle = InRequest.Get_ItemToRemove();
        auto R = Result::Failed_InvalidItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Remove>(
            InRequest, [&]{ return MakePayload(Base, ItemHandle, R); });

        if (ck::Is_NOT_Valid(ItemHandle))
        {
            ck::inventory::Warning(TEXT("RemoveItem_Spatial: Invalid item handle"));
            return R;
        }

        if (NOT FInventoryItemRecord::Get_ContainsEntry(Base, ItemHandle))
        {
            R = Result::Failed_ItemNotInInventory;
            ck::inventory::Warning(TEXT("RemoveItem_Spatial: Item [{}] not in inventory [{}]"), ItemHandle, InHandle);
            return R;
        }

        UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(InHandle, ItemHandle);
        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Disconnect(Base, ItemHandle);
        TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, FCk_Handle_Inventory());
        detail::FinalizeItemRemoval(ItemHandle, Base, InRequest.Get_PostRemovePolicy());
        R = Result::Success;
        return R;
    }

    // ---- TStackItems<Spatial>: divergence helper specialization (remove from grid).
    template <>
    auto TStackItems<FCk_Handle_Inventory_Spatial>::OnSourceFullyConsumed(
        FCk_Handle_Inventory_Spatial& InInventory, FCk_Handle_Item& InItem) -> void
    {
        UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(InInventory, InItem);
    }

    // ---- TSplitStack<Spatial, FCk_SpatialPlacement>: full body with placement step.
    template <>
    auto TSplitStack<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>::Handle(
        FCk_Handle_Inventory_Spatial& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest   = InEntry.Common;
        const auto& InPlacement = InEntry.Addon;
        auto SourceItem = InRequest.Get_SourceItem();
        auto NewItem    = FCk_Handle_Item{};
        auto R          = Result::Failed_InvalidSourceItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Split>(
            InRequest, [&]{ return MakePayload(Base, SourceItem, NewItem, R); });

        if (ck::Is_NOT_Valid(SourceItem))
        {
            ck::inventory::Warning(TEXT("SplitStack: Invalid source item handle"));
            return R;
        }

        if (NOT FInventoryItemRecord::Get_ContainsEntry(Base, SourceItem))
        {
            R = Result::Failed_SourceNotInInventory;
            ck::inventory::Warning(TEXT("SplitStack: Source [{}] not in inventory [{}]"), SourceItem, InHandle);
            return R;
        }

        if (NOT UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(SourceItem))
        {
            R = Result::Failed_ItemNotStackable;
            ck::inventory::Warning(TEXT("SplitStack: Source [{}] is not stackable"), SourceItem);
            return R;
        }

        const auto CurrentCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(SourceItem);
        const auto SplitCount   = InRequest.Get_SplitCount();

        if (SplitCount < 1 || SplitCount >= CurrentCount)
        {
            R = Result::Failed_InsufficientCount;
            ck::inventory::Warning(TEXT("SplitStack: Invalid split count [{}] for source [{}] with count [{}]"),
                SplitCount, SourceItem, CurrentCount);
            return R;
        }

        // Placement gates the entry itself further below.
        if (SplitCount > UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize(Base, SourceItem))
        {
            R = Result::Failed_NoSpaceForNewItem;
            ck::inventory::Display(TEXT("SplitStack_Spatial: Split count [{}] exceeds the effective max stack size of inventory [{}]"),
                SplitCount, InHandle);
            return R;
        }

        auto* Definition  = UCk_Utils_Item_UE::Get_Definition(SourceItem);
        auto ContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(Base);
        NewItem = UCk_Utils_Item_UE::Create(ContextOwner, Definition);

        if (ck::Is_NOT_Valid(NewItem))
        {
            ck::inventory::Warning(TEXT("SplitStack: Failed to create new item from definition"));
            return R;
        }

        Definition->OnSplit(SourceItem, NewItem);

        const auto Coord = InPlacement.Get_Coordinate();
        const auto Placement = (Coord == ck::Inventory::AutoPlaceCoordinate)
            ? UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(InHandle, NewItem)
            : UCk_Utils_Inventory_Spatial_UE::Get_CanPlaceItemAt(InHandle, NewItem, Coord);

        if (NOT Placement.Get_Succeeded())
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
            NewItem = {};
            R = Result::Failed_NoSpaceForNewItem;
            ck::inventory::Warning(TEXT("SplitStack: No space for new item in inventory [{}]"), InHandle);
            return R;
        }

        UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
            InHandle, NewItem, Placement.Get_Coordinate(), Placement.Get_Rotation());

        UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, SplitCount);
        UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(SourceItem, -SplitCount);

        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
            Base, NewItem, ECk_Record_LabelRequirementPolicy::Optional);
        TUtils_Item_ParentInventory::AddOrReplace(NewItem, Base);
        UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(NewItem, Base);

        R = Result::Success;
        return R;
    }

    // ---- TAddByDefinition<Spatial>: divergence helper specialization (find placement + place).
    template <>
    auto TAddByDefinition<FCk_Handle_Inventory_Spatial>::TryPlace(
        FCk_Handle_Inventory_Spatial& InInventory, FCk_Handle_Item& InItem) -> bool
    {
        const auto Placement = UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(InInventory, InItem);
        if (NOT Placement.Get_Succeeded())
        { return false; }

        UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
            InInventory, InItem, Placement.Get_Coordinate(), Placement.Get_Rotation());
        return true;
    }

    // ---- TSort<Spatial>: full body — clear grid, sort, re-place.
    namespace ck_inventory_spatial_request_traits
    {
        auto BuildSpatialSortPredicate(const FCk_Request_Inventory_Sort& InRequest)
            -> TFunction<bool(const FCk_Handle_Item&, const FCk_Handle_Item&)>
        {
            const auto& NativePredicate  = InRequest.Get_SortPredicate();
            const auto& DynamicPredicate = InRequest.Get_SortPredicateDynamic();

            if (NOT NativePredicate.IsBound() && NOT DynamicPredicate.IsBound())
            { return {}; }

            return [NativePredicate, DynamicPredicate](const FCk_Handle_Item& A, const FCk_Handle_Item& B) -> bool
            {
                if (NativePredicate.IsBound())
                { return NativePredicate.Execute(A, B); }
                auto ABeforeB = false;
                DynamicPredicate.ExecuteIfBound(A, B, ABeforeB);
                return ABeforeB;
            };
        }
    }

    template <>
    auto TSort<FCk_Handle_Inventory_Spatial>::Handle(
        FCk_Handle_Inventory_Spatial& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto R = Result::Failed_InvalidInventory;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Sort>(
            InRequest, [&]{ return MakePayload(Base, R); });

        if (ck::Is_NOT_Valid(InHandle))
        { return R; }

        const auto SortPredicate = ck_inventory_spatial_request_traits::BuildSpatialSortPredicate(InRequest);
        if (NOT SortPredicate)
        { return R; }

        // Stash rotation BEFORE clearing the grid — the lookup is gone once items leave it.
        auto Items = FInventoryItemRecord::Get_ValidEntries(Base);
        struct FItemWithRotation { FCk_Handle_Item Item; ECk_CardinalRotation Rotation; };

        auto ItemsWithRotation = ck::algo::Transform<TArray<FItemWithRotation>>(
            Items,
            [](const FCk_Handle_Item& Item) -> FItemWithRotation
            { return {Item, UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementRotation(Item)}; });

        ItemsWithRotation.Sort([&](const FItemWithRotation& A, const FItemWithRotation& B)
        { return SortPredicate(A.Item, B.Item); });

        for (const auto& [Item, Rotation] : ItemsWithRotation)
        { UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(InHandle, Item); }

        for (const auto& [Item, Rotation] : ItemsWithRotation)
        {
            if (const auto Placement = UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(InHandle, Item);
                Placement.Get_Succeeded())
            {
                UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
                    InHandle, Item, Placement.Get_Coordinate(), Placement.Get_Rotation());
            }
            else
            {
                ck::inventory::Warning(TEXT("Sort: Failed to re-place item [{}] in spatial inventory [{}]"),
                    Item, InHandle);
            }
        }

        FInventoryItemRecord::Sort(Base, [&](const FCk_Handle_Item& A, const FCk_Handle_Item& B)
        { return SortPredicate(A, B); });

        R = Result::Success;
        return R;
    }

    // ---- TRelocate<Spatial>: full body.
    template <>
    auto TRelocate<FCk_Handle_Inventory_Spatial>::Handle(
        FCk_Handle_Inventory_Spatial& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto ItemHandle      = InRequest.Get_Item();
        const auto NewCoord  = InRequest.Get_NewPlacement().Get_Coordinate();
        const auto NewRot    = InRequest.Get_NewPlacement().Get_Rotation();
        auto R               = Result::Failed_InvalidItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Relocate>(
            InRequest, [&]{ return MakePayload(Base, ItemHandle, NewCoord, R); });

        CK_ENSURE_IF_NOT(ck::IsValid(ItemHandle), TEXT("RelocateItem: Invalid item handle"))
        { return R; }

        if (NOT UCk_Utils_Inventory_UE::Get_ContainsItem(Base, ItemHandle))
        {
            R = Result::Failed_ItemNotInInventory;
            ck::inventory::Warning(TEXT("RelocateItem: Item [{}] not in inventory [{}]"), ItemHandle, InHandle);
            return R;
        }

        const auto OldCoord = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementCoordinate(InHandle, ItemHandle);
        const auto OldRot   = UCk_Utils_Inventory_Spatial_UE::Get_ItemPlacementRotation(ItemHandle);

        UCk_Utils_Inventory_Spatial_UE::Request_RemoveItemFromGrid(InHandle, ItemHandle);

        const auto Placement = UCk_Utils_Inventory_Spatial_UE::DoCanPlaceItemAt(InHandle, ItemHandle, NewCoord, NewRot)
            ? FCk_SpatialPlacementResult::Success(NewCoord, NewRot)
            : UCk_Utils_Inventory_Spatial_UE::Get_CanPlaceItemAt(InHandle, ItemHandle, NewCoord);

        if (NOT Placement.Get_Succeeded())
        {
            if (OldCoord != ck::Inventory::AutoPlaceCoordinate)
            {
                UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(InHandle, ItemHandle, OldCoord, OldRot);
            }
            R = Result::Failed_PlacementBlocked;
            // Caller-attributable rejection — Display, never Warning: the AutoTest harness
            // escalates Warnings to test failures.
            ck::inventory::Display(TEXT("RelocateItem: Cannot place item [{}] at [{}] in inventory [{}]"),
                ItemHandle, NewCoord, InHandle);
            return R;
        }

        UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid(
            InHandle, ItemHandle, Placement.Get_Coordinate(), Placement.Get_Rotation());

        UCk_Utils_Inventory_UE::Request_TryReplicateInventory(Base);
        R = Result::Success;
        return R;
    }

    // Every Spatial-rooted TXxx<...> is explicitly instantiated in CkInventory_RequestHandlers.cpp,
    // where the default Handle bodies are visible; the header's forward decls route them here.
}

namespace ck
{
    auto TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>::Setup_PerShape(
        FCk_Handle_Inventory& InInventoryEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication /*InReplicates*/) -> void
    {
        InInventoryEntity.Add<FTag_Inventory_Spatial>();

        auto TransformHandle = UCk_Utils_Transform_UE::Add(InInventoryEntity, FTransform::Identity);

        const auto GridParams = FCk_Fragment_2dGridSystem_ParamsData(
            InParams.Get_Dimensions(),
            FVector2D(1.0, 1.0));

        auto TileMapHandle = UCk_Utils_2dGridSystem_UE::Add(TransformHandle, GridParams);

        UCk_Utils_2dGridSystem_UE::ForEach_Cell(TileMapHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
            [&](FCk_Handle_2dGridCell InCell)
        {
            ck::TUtils_InventorySlot_ItemRef::Clear(InCell);
        });
    }
}
