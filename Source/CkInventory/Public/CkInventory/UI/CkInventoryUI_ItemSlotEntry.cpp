#include "CkInventory/UI/CkInventoryUI_ItemSlotEntry.h"

#include "CkInventory/UI/CkInventoryUI_ListViewObject.h"
#include "CkInventory/UI/CkInventoryUI_DragDropOperation.h"
#include "CkInventory/UI/CkInventoryUI_DragWidget.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/WidgetBlueprintLibrary.h>

auto
    UCk_InventoryUI_ItemSlotEntry::
    InjectItemData(
        FCk_Handle_Item InMaybeValidItem,
        FCk_Handle_Inventory InInventory)
    -> void
{
    if (_ItemHandle == InMaybeValidItem && _InventoryHandle == InInventory)
    { return; }

    _ItemHandle = InMaybeValidItem;
    _InventoryHandle = InInventory;

    OnItemDataSet(_ItemHandle, _InventoryHandle);
}

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

auto
    UCk_InventoryUI_ItemSlotEntry::
    CanDrag_Implementation() const
    -> bool
{
    return _DragDropPolicy.Get_AllowTakeOut();
}

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnMouseEnter(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent)
    -> void
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (ck::IsValid(_ItemHandle))
    {
        OnItemHovered(_ItemHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnMouseLeave(
        const FPointerEvent& InMouseEvent)
    -> void
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (ck::IsValid(_ItemHandle))
    {
        OnItemUnhovered(_ItemHandle);
    }
}

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
        // The dragged slot's geometry gives the drag visual its size — so a
        // rectangular slot produces a rectangular drag visual, not a square.
        const auto SlotSize = InGeometry.GetLocalSize();

        auto* const DragVisual = UCk_InventoryUI_DragWidget::CreateDragWidget(
            this, _DragWidgetClass, _ItemHandle, _InventoryHandle, SlotSize);

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

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return false; }

    return CanAcceptDrop(InventoryOp);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return false; }

    if (NOT CanAcceptDrop(InventoryOp))
    { return false; }

    // ---- Don't drop onto self ----

    if (InventoryOp->Get_SourceItem() == _ItemHandle)
    { return false; }

    // ---- Delegate to Blueprint ----
    // Slot-level drop semantics (swap, stack, merge) depend on game logic.
    // Blueprint handles the operation via OnDropReceived.

    return OnDropReceived(InventoryOp);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnDragEnter(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> void
{
    Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return; }

    // Fire regardless of acceptance — the hover highlight is coloured from InCanAccept.
    OnDragHoverStarted(InventoryOp, CanAcceptDrop(InventoryOp));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    NativeOnDragLeave(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> void
{
    Super::NativeOnDragLeave(InDragDropEvent, InOperation);

    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return; }

    OnDragHoverEnded();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    OnDropReceived_Implementation(
        UCk_InventoryUI_DragDropOperation* InOperation)
    -> bool
{
    // Default: do not suppress default handling
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    CanAcceptDrop_Implementation(
        UCk_InventoryUI_DragDropOperation* InOperation) const
    -> bool
{
    if (ck::Is_NOT_Valid(InOperation))
    { return false; }

    if (ck::Is_NOT_Valid(InOperation->Get_SourceItem()))
    { return false; }

    // Same-inventory drops (rearrange / stack within this container) are always allowed.
    if (InOperation->Get_SourceInventory() == _InventoryHandle)
    { return true; }

    // Cross-inventory drop == "drop in" — gated by the panel-propagated policy.
    return _DragDropPolicy.Get_AllowDropIn();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    OnItemHovered_Implementation(
        FCk_Handle_Item InItem)
    -> void
{
    // Default: no-op. Override in Blueprint or C++ subclass.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_ItemSlotEntry::
    OnItemUnhovered_Implementation(
        FCk_Handle_Item InItem)
    -> void
{
    // Default: no-op. Override in Blueprint or C++ subclass.
}

// --------------------------------------------------------------------------------------------------------------------
