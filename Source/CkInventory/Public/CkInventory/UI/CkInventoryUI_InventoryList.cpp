#include "CkInventory/UI/CkInventoryUI_InventoryList.h"

#include "CkInventory/UI/CkInventoryUI_DragDropOperation.h"
#include "CkInventory/UI/CkInventoryUI_ListViewObject.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

auto
    UCk_InventoryUI_InventoryList::
    InjectInventory(
        FCk_Handle_Inventory InInventory)
    -> void
{
    if (_IsBound)
    {
        DoUnbindSignal();
    }

    _InventoryHandle = InInventory;

    if (ck::IsValid(_InventoryHandle))
    {
        DoBindSignal();
        RefreshList();
        OnInventoryBound(_InventoryHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryList::
    RefreshList()
    -> void
{
    if (ck::Is_NOT_Valid(_ItemListView))
    { return; }

    if (ck::Is_NOT_Valid(_InventoryHandle))
    {
        _ItemListView->ClearListItems();
        return;
    }

    const auto Items = UCk_Utils_Inventory_UE::Get_Items(_InventoryHandle);

    TArray<UObject*> ListItems;
    ListItems.Reserve(Items.Num());

    for (const auto& ItemHandle : Items)
    {
        auto* const ListObj = NewObject<UCk_InventoryUI_ListViewObject>(this);
        ListObj->Set_ItemHandle(ItemHandle);
        ListObj->Set_InventoryHandle(_InventoryHandle);
        ListItems.Add(ListObj);
    }

    _ItemListView->SetListItems(ListItems);

    OnInventoryRefreshed();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryList::
    ClearList()
    -> void
{
    DoUnbindSignal();

    _InventoryHandle = {};

    if (IsValid(_ItemListView))
    {
        _ItemListView->ClearListItems();
    }
}

auto
    UCk_InventoryUI_InventoryList::
    NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);
    return IsValid(InventoryOp);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryList::
    NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return false; }

    if (OnDropReceived(InventoryOp))
    { return true; }

    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return false; }

    const auto SourceItem = InventoryOp->Get_SourceItem();
    const auto SourceInventory = InventoryOp->Get_SourceInventory();

    if (ck::Is_NOT_Valid(SourceItem))
    { return false; }

    // Same-inventory moves (reorder, stack) belong in Blueprint at the slot level,
    // where the target item is known.
    if (SourceInventory == _InventoryHandle)
    { return false; }

    // Target shape decides the Transfer variant; source dispatch is internal to the base Utils.
    auto SourceMutable = const_cast<FCk_Handle_Inventory&>(SourceInventory);
    auto TargetMutable = _InventoryHandle;
    const auto Callback = FCk_Delegate_Inventory_OnOperationResult_Transfer{};

    if (UCk_Utils_Inventory_UE::Get_IsSpatial(TargetMutable))
    {
        auto TgtSpatial = UCk_Utils_Inventory_Spatial_UE::DoCastChecked(TargetMutable);
        UCk_Utils_Inventory_UE::Request_TransferItem_ToSpatial(
            SourceMutable,
            FCk_Request_Inventory_TransferItem_ToSpatial(SourceItem, TgtSpatial),
            Callback,
            {});
    }
    else
    {
        auto TgtDataOnly = UCk_Utils_Inventory_DataOnly_UE::DoCastChecked(TargetMutable);
        UCk_Utils_Inventory_UE::Request_TransferItem_ToDataOnly(
            SourceMutable,
            FCk_Request_Inventory_TransferItem_ToDataOnly(SourceItem, TgtDataOnly),
            Callback,
            {});
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryList::
    OnDropReceived_Implementation(
        UCk_InventoryUI_DragDropOperation* InOperation)
    -> bool
{
    // Default: do not suppress default handling
    return false;
}

auto
    UCk_InventoryUI_InventoryList::
    HandleOnItemsChanged(
        FCk_Handle_Inventory InInventory,
        const TArray<FCk_Handle_Item>& InItemsAdded,
        const TArray<FCk_Handle_Item>& InItemsRemoved)
    -> void
{
    RefreshList();
}

auto
    UCk_InventoryUI_InventoryList::
    NativeDestruct()
    -> void
{
    DoUnbindSignal();

    Super::NativeDestruct();
}

auto
    UCk_InventoryUI_InventoryList::
    DoBindSignal()
    -> void
{
    if (_IsBound)
    { return; }

    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return; }

    FCk_Delegate_Inventory_OnItemsChanged Delegate;
    Delegate.BindDynamic(this, &ThisClass::HandleOnItemsChanged);

    UCk_Utils_Inventory_UE::BindTo_OnItemsChanged(
        _InventoryHandle,
        Delegate);

    _IsBound = true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryList::
    DoUnbindSignal()
    -> void
{
    if (NOT _IsBound)
    { return; }

    if (ck::Is_NOT_Valid(_InventoryHandle))
    {
        _IsBound = false;
        return;
    }

    FCk_Delegate_Inventory_OnItemsChanged Delegate;
    Delegate.BindDynamic(this, &ThisClass::HandleOnItemsChanged);

    UCk_Utils_Inventory_UE::UnbindFrom_OnItemsChanged(
        _InventoryHandle,
        Delegate);

    _IsBound = false;
}

// --------------------------------------------------------------------------------------------------------------------
