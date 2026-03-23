#include "CkInventory/UI/CkInventoryUI_DragWidget.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/WidgetBlueprintLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_DragWidget::
    SetDragData(
        const FCk_Handle_Item& InItem,
        const FCk_Handle_Inventory& InInventory)
    -> void
{
    _ItemHandle = InItem;
    _SourceInventory = InInventory;

    OnDragDataSet(InItem, InInventory);
}

// --------------------------------------------------------------------------------------------------------------------

UCk_InventoryUI_DragWidget*
    UCk_InventoryUI_DragWidget::
    CreateDragWidget(
        UUserWidget* InOwner,
        TSubclassOf<UCk_InventoryUI_DragWidget> InClass,
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory)
{
    if (ck::Is_NOT_Valid(InOwner) || ck::Is_NOT_Valid(InClass))
    { return nullptr; }

    auto* const Widget = CreateWidget<UCk_InventoryUI_DragWidget>(InOwner, InClass);

    if (ck::Is_NOT_Valid(Widget))
    { return nullptr; }

    Widget->SetDragData(InItem, InInventory);

    return Widget;
}

// --------------------------------------------------------------------------------------------------------------------
