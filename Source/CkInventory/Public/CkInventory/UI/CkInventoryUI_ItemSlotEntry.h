#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"
#include "CkInventory/UI/CkInventoryUI_DragDropPolicy.h"

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

public:
    /** Populates this slot with the given inventory and optional item.
     *  Item may be invalid for empty slots. Calls OnItemDataSet. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Slot")
    void InjectItemData(
        FCk_Handle_Item InMaybeValidItem,
        FCk_Handle_Inventory InInventory);

protected:
    /** Implement in Blueprint to populate visuals.
     *  InMaybeValidItem may be invalid for empty slots — check with IsValid. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnItemDataSet(
        FCk_Handle_Item InMaybeValidItem,
        FCk_Handle_Inventory InInventory);

    /** Called when a drag starts from this slot. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnDragStarted();

    /** Called when a drag from this slot is cancelled. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnSlotDragCancelled();

    /** Called when a valid drag enters this slot's bounds. Use for highlight visuals. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnDragHoverStarted(
        UCk_InventoryUI_DragDropOperation* InOperation);

    /** Called when a drag leaves this slot's bounds. Use to clear highlight visuals. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnDragHoverEnded();

    /** Called when the mouse enters this slot and it holds a valid item. */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnItemHovered(
        FCk_Handle_Item InItem);

    /** Called when the mouse leaves this slot after an item hover. */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Slot")
    void OnItemUnhovered(
        FCk_Handle_Item InItem);

    /**
     * Called when a drag-drop operation is received on this slot.
     * Return true to mark the drop as handled.
     */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Slot")
    bool OnDropReceived(
        UCk_InventoryUI_DragDropOperation* InOperation);

    /** Override to control whether this slot can accept a drop.
     *  Default returns true when the operation carries a valid item. */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Slot")
    bool CanAcceptDrop(
        UCk_InventoryUI_DragDropOperation* InOperation) const;

protected:
    auto NativeOnListItemObjectSet(UObject* InListItemObject) -> void override;

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

protected:
    auto NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> bool override;

    auto NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> bool override;

protected:
    auto NativeOnMouseEnter(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) -> void override;

    auto NativeOnMouseLeave(
        const FPointerEvent& InMouseEvent) -> void override;

    auto NativeOnDragEnter(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> void override;

    auto NativeOnDragLeave(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> void override;

protected:
    /** The widget class to use for drag visuals. Set in Blueprint defaults
     *  or programmatically by the owning panel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Drag")
    TSubclassOf<UCk_InventoryUI_DragWidget> _DragWidgetClass;

    /** Drag-and-drop access policy. Normally propagated from the owning panel;
     *  drives the default CanDrag / CanAcceptDrop implementations. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Drag")
    FCk_InventoryUI_DragDropPolicy _DragDropPolicy;

public:
    CK_PROPERTY_SET(_DragWidgetClass);
    CK_PROPERTY(_DragDropPolicy);

    /** Override to control whether this slot can be dragged.
     *  Default returns _DragDropPolicy._AllowTakeOut. */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Drag")
    bool CanDrag() const;

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
