#include "CkInventory/UI/CkInventoryUI_InventoryList.h"

#include "CkInventory/UI/CkInventoryUI_DragDropOperation.h"
#include "CkInventory/UI/CkInventoryUI_ListViewObject.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// ---- Public API ----

auto
    UCk_InventoryUI_InventoryList::
    InjectInventory(
        FCk_Handle_Inventory InInventory)
    -> void
{
    // ---- Unbind previous ----

    if (_IsBound)
    {
        DoUnbindSignal();
    }

    // ---- Bind new ----

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

    // ---- Get current items ----

    const auto Items = UCk_Utils_Inventory_UE::Get_Items(_InventoryHandle);

    // ---- Build list view objects ----

    TArray<UObject*> ListItems;
    ListItems.Reserve(Items.Num());

    for (const auto& ItemHandle : Items)
    {
        auto* const ListObj = NewObject<UCk_InventoryUI_ListViewObject>(this);
        ListObj->Set_ItemHandle(ItemHandle);
        ListObj->Set_InventoryHandle(_InventoryHandle);
        ListItems.Add(ListObj);
    }

    // ---- Update list view ----

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

// ---- Drop Handling ----

auto
    UCk_InventoryUI_InventoryList::
    NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    // Accept drag-over if the operation is our inventory drag type
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

    // ---- Let Blueprint handle first ----

    if (OnDropReceived(InventoryOp))
    { return true; }

    // ---- Default handling: transfer between inventories ----

    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return false; }

    const auto SourceItem = InventoryOp->Get_SourceItem();
    const auto SourceInventory = InventoryOp->Get_SourceInventory();

    if (ck::Is_NOT_Valid(SourceItem))
    { return false; }

    // Only handle cross-inventory transfers in the default implementation.
    // Same-inventory moves (reorder, stack) are better handled in Blueprint
    // at the slot level where the target item is known.
    if (SourceInventory == _InventoryHandle)
    { return false; }

    const auto Request = FCk_Request_Inventory_TransferItem(SourceItem, _InventoryHandle);
    UCk_Utils_Inventory_UE::Request_TransferItem(
        const_cast<FCk_Handle_Inventory&>(SourceInventory),
        Request,
        FCk_Delegate_Inventory_OnOperationResult_Transfer{});

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

// ---- Signal Callback ----

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

// ---- Lifecycle ----

auto
    UCk_InventoryUI_InventoryList::
    NativeDestruct()
    -> void
{
    DoUnbindSignal();

    Super::NativeDestruct();
}

// ---- Internal ----

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
