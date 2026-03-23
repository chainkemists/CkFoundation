#include "CkInventory/UI/CkInventoryUI_ItemSlotEntry.h"

#include "CkInventory/UI/CkInventoryUI_ListViewObject.h"
#include "CkInventory/UI/CkInventoryUI_DragDropOperation.h"
#include "CkInventory/UI/CkInventoryUI_DragWidget.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/WidgetBlueprintLibrary.h>

// ---- Public API ----

auto
    UCk_InventoryUI_ItemSlotEntry::
    InjectItemData(
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory)
    -> void
{
    _ItemHandle = InItem;
    _InventoryHandle = InInventory;

    OnItemDataSet(_ItemHandle, _InventoryHandle);
}

// ---- IUserObjectListEntry ----

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnListItemObjectSet(UObject* InListItemObject)
    -> void
{
    IUserObjectListEntry::NativeOnListItemObjectSet(InListItemObject);

    auto* const ListViewObject = Cast<UCk_InventoryUI_ListViewObject>(InListItemObject);

    if (ck::Is_NOT_Valid(ListViewObject))
    { return; }

    InjectItemData(ListViewObject->Get_ItemHandle(), ListViewObject->Get_InventoryHandle());
}

// ---- Configuration ----

auto
    UCk_InventoryUI_ItemSlotEntry::
    CanDrag_Implementation() const
    -> bool
{
    return true;
}

// ---- Drag Support ----

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent)
    -> FReply
{
    if (NOT CanDrag())
    { return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent); }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnDragDetected(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation)
    -> void
{
    if (ck::Is_NOT_Valid(_ItemHandle))
    {
        Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
        return;
    }

    // ---- Create drag operation ----

    auto* const Operation = NewObject<UCk_InventoryUI_DragDropOperation>();
    Operation->Set_SourceItem(_ItemHandle);
    Operation->Set_SourceInventory(_InventoryHandle);
    Operation->Set_DropOperation(ECk_InventoryUI_DropOperation::MoveItem);

    // ---- Create drag visual ----

    if (ck::IsValid(_DragWidgetClass))
    {
        auto* const DragVisual = UCk_InventoryUI_DragWidget::CreateDragWidget(
            this, _DragWidgetClass, _ItemHandle, _InventoryHandle);

        if (ck::IsValid(DragVisual))
        {
            Operation->DefaultDragVisual = DragVisual;
        }
    }

    OutOperation = Operation;

    OnDragStarted();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnDragCancelled(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> void
{
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

    OnSlotDragCancelled();
}

// --------------------------------------------------------------------------------------------------------------------
