#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Blueprint/UserWidget.h>
#include <Blueprint/IUserObjectListEntry.h>

#include "CkInventoryUI_ItemSlotEntry.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryUI_DragWidget;
class UCk_InventoryUI_DragDropOperation;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Abstract widget representing a single inventory item slot.
 * Implements IUserObjectListEntry for use with UListView.
 *
 * Blueprint subclass designs the visual layout (icon, name, count)
 * and implements OnItemDataSet to populate widgets from the item handles.
 *
 * Supports drag initiation — when dragged, creates a UCk_InventoryUI_DragDropOperation
 * with the item and inventory handles, and a UCk_InventoryUI_DragWidget as the visual.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKINVENTORY_API UCk_InventoryUI_ItemSlotEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_ItemSlotEntry);

    // ---- Public API ----

public:
    /** Populates this slot with the given item and inventory handles.
     *  Calls OnItemDataSet. Works standalone or from a ListView. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Slot")
    void InjectItemData(
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory);

    // ---- Blueprint Events ----

protected:
    /** Implement in Blueprint to populate visuals from the item handles. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnItemDataSet(
        FCk_Handle_Item InItem,
        FCk_Handle_Inventory InInventory);

    /** Called when a drag starts from this slot. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnDragStarted();

    /** Called when a drag from this slot is cancelled. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnSlotDragCancelled();

    // ---- IUserObjectListEntry ----

protected:
    auto NativeOnListItemObjectSet(UObject* InListItemObject) -> void override;

    // ---- Drag Support ----

protected:
    auto NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) -> FReply override;

    auto NativeOnDragDetected(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation) -> void override;

    auto NativeOnDragCancelled(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> void override;

    // ---- Configuration ----

protected:
    /** The widget class to use for drag visuals. Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Drag")
    TSubclassOf<UCk_InventoryUI_DragWidget> _DragWidgetClass;

    /** Override to control whether this slot can be dragged.
     *  Default returns true. */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Drag")
    bool CanDrag() const;

    // ---- Data ----

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _ItemHandle;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _InventoryHandle;

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY_GET(_InventoryHandle);
};

// --------------------------------------------------------------------------------------------------------------------
