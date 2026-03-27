#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Blueprint/UserWidget.h>
#include <Components/ListView.h>

#include "CkInventoryUI_InventoryList.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryUI_DragDropOperation;
class UCk_InventoryUI_DragWidget;
class UCk_InventoryUI_ItemSlotEntry;
class UCk_InventoryUI_ListViewObject;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Abstract list container widget that displays inventory contents via UListView.
 *
 * Blueprint subclass places a UListView named "_ItemListView" and sets the
 * entry widget class. At runtime, call InjectInventory() to bind to an inventory —
 * the list auto-populates and auto-refreshes when items change.
 *
 * Supports receiving drops from UCk_InventoryUI_ItemSlotEntry for
 * transfer and stacking operations.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKINVENTORY_API UCk_InventoryUI_InventoryList : public UUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_InventoryList);

    // ---- Public API ----

public:
    /** Binds this list to an inventory. Populates items and subscribes to change signals. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|List",
              DisplayName = "[Ck][InventoryUI] Set Inventory")
    void InjectInventory(
        FCk_Handle_Inventory InInventory);

    /** Rebuilds the list view items from the current inventory contents. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|List",
              DisplayName = "[Ck][InventoryUI] Refresh List")
    void RefreshList();

    /** Clears the list and unbinds from the inventory signal. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|List",
              DisplayName = "[Ck][InventoryUI] Clear List")
    void ClearList();

    // ---- Blueprint Events ----

protected:
    /** Called after binding to a new inventory. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|List")
    void OnInventoryBound(
        FCk_Handle_Inventory InInventory);

    /** Called after the list has been refreshed. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|List")
    void OnInventoryRefreshed();

    /**
     * Called when a drag-drop operation is received on this list.
     * Return true to suppress the default drop handling (transfer/stack).
     */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|List")
    bool OnDropReceived(
        UCk_InventoryUI_DragDropOperation* InOperation);

    // ---- Drop Handling ----

protected:
    auto NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> bool override;

    auto NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) -> bool override;

    // ---- Lifecycle ----

protected:
    auto NativeDestruct() -> void override;

    // ---- Signal Callback ----

private:
    UFUNCTION()
    void HandleOnItemsChanged(
        FCk_Handle_Inventory InInventory,
        const TArray<FCk_Handle_Item>& InItemsAdded,
        const TArray<FCk_Handle_Item>& InItemsRemoved);

    // ---- Internal ----

private:
    auto DoUnbindSignal() -> void;
    auto DoBindSignal() -> void;

    // ---- Configuration ----

protected:
    /** The drag widget class used by item slots in this list. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|List")
    TSubclassOf<UCk_InventoryUI_DragWidget> _DragWidgetClass;

    // ---- Bound Widgets ----

protected:
    /** The ListView that displays item entries. Must be named "_ItemListView" in Blueprint. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidget))
    TObjectPtr<UListView> _ItemListView;

    // ---- Data ----

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _InventoryHandle;

    bool _IsBound = false;

public:
    CK_PROPERTY_GET(_InventoryHandle);
};

// --------------------------------------------------------------------------------------------------------------------
