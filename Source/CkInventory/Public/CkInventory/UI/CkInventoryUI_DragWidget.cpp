#include "CkInventory/UI/CkInventoryUI_DragWidget.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/WidgetBlueprintLibrary.h>
#include <Components/SizeBox.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_DragWidget::
    SetDragData(
        const FCk_Handle_Item& InItem,
        const FCk_Handle_Inventory& InInventory,
        const FVector2D& InSlotSize)
    -> void
{
    _ItemHandle = InItem;
    _SourceInventory = InInventory;
    _SourceSlotSize = InSlotSize;

    // ---- Resize the drag visual to match the source slot ----
    // An axis <= 0 is left unconstrained so the visual keeps its authored size there.

    if (ck::IsValid(_RootSizeBox))
    {
        if (_SourceSlotSize.X > 0.0)
        { _RootSizeBox->SetWidthOverride(_SourceSlotSize.X); }

        if (_SourceSlotSize.Y > 0.0)
        { _RootSizeBox->SetHeightOverride(_SourceSlotSize.Y); }
    }

    OnDragDataSet(InItem, InInventory);
}

// --------------------------------------------------------------------------------------------------------------------

UCk_InventoryUI_DragWidget*
    UCk_InventoryUI_DragWidget::
    CreateDragWidget(
        UUserWidget* InOwner,
        TSubclassOf<UCk_InventoryUI_DragWidget> InClass,
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory,
        FVector2D InSlotSize)
{
    if (ck::Is_NOT_Valid(InOwner) || ck::Is_NOT_Valid(InClass))
    { return nullptr; }

    auto* const Widget = CreateWidget<UCk_InventoryUI_DragWidget>(InOwner, InClass);

    if (ck::Is_NOT_Valid(Widget))
    { return nullptr; }

    Widget->SetDragData(InItem, InInventory, InSlotSize);

    return Widget;
}

// --------------------------------------------------------------------------------------------------------------------
