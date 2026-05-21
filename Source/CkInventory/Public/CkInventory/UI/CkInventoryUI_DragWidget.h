#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Blueprint/UserWidget.h>

#include "CkInventoryUI_DragWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class USizeBox;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Abstract widget displayed under the cursor during an inventory drag operation.
 * Blueprint subclass designs the visual (icon, name, quantity) and implements
 * OnDragDataSet to populate it from item data.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKINVENTORY_API UCk_InventoryUI_DragWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_DragWidget);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _ItemHandle;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _SourceInventory;

    /** Size of the slot the drag originated from, in local (DPI-scaled) pixels. */
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector2D _SourceSlotSize = FVector2D::ZeroVector;

    /** Optional root SizeBox. When present, the drag visual is resized to match
     *  the source slot. Name a USizeBox "_RootSizeBox" in the drag Blueprint to
     *  opt in; without it the widget keeps its authored layout size. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<USizeBox> _RootSizeBox;

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY_GET(_SourceInventory);
    CK_PROPERTY_GET(_SourceSlotSize);

public:
    /** Called by the drag initiator to populate this widget with item data.
     *  InSlotSize is the originating slot's size; an axis <= 0 is left unconstrained. */
    auto SetDragData(
        const FCk_Handle_Item& InItem,
        const FCk_Handle_Inventory& InInventory,
        const FVector2D& InSlotSize) -> void;

protected:
    /** Implement in Blueprint to set up visuals (icon, name, count) from the item handles. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Drag")
    void OnDragDataSet(
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory);

public:
    /** Creates and initializes a drag widget instance. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Drag",
              DisplayName = "[Ck][InventoryUI] Create Drag Widget")
    static UCk_InventoryUI_DragWidget*
    CreateDragWidget(
        UUserWidget* InOwner,
        TSubclassOf<UCk_InventoryUI_DragWidget> InClass,
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory,
        FVector2D InSlotSize);
};

// --------------------------------------------------------------------------------------------------------------------
