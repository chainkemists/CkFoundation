#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Blueprint/UserWidget.h>

#include "CkInventoryUI_DragWidget.generated.h"

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

    // ---- Data ----

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _ItemHandle;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _SourceInventory;

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY_GET(_SourceInventory);

    // ---- Setup ----

public:
    /** Called by the drag initiator to populate this widget with item data. */
    auto SetDragData(
        const FCk_Handle_Item& InItem,
        const FCk_Handle_Inventory& InInventory) -> void;

    // ---- Blueprint Events ----

protected:
    /** Implement in Blueprint to set up visuals (icon, name, count) from the item handles. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Drag")
    void OnDragDataSet(
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory);

    // ---- Factory ----

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
        FCk_Handle_Inventory InInventory);
};

// --------------------------------------------------------------------------------------------------------------------
