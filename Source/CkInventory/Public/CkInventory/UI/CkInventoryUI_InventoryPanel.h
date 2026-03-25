#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Blueprint/UserWidget.h>

#include "CkInventoryUI_InventoryPanel.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryUI_DragDropOperation;
class UCk_InventoryUI_DragWidget;
class UCk_InventoryUI_ItemSlotEntry;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Abstract base class for self-constructing inventory panel widgets.
 *
 * Provides shared infrastructure: slot/drag class config, signal bind/unbind,
 * drop handling, and lifecycle management.
 *
 * Concrete subclasses:
 *  - UCk_InventoryUI_SpatialPanel   (grid-based, UUniformGridPanel)
 *  - UCk_InventoryUI_DataOnlyPanel  (list-based, UPanelWidget)
 */
UCLASS(Abstract, BlueprintType, NotBlueprintable, meta = (DisableNativeTick))
class CKINVENTORY_API UCk_InventoryUI_InventoryPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_InventoryPanel);

    // ---- Public API ----

public:
    /** Rebuilds the item layout from current inventory contents. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Panel")
    void RefreshPanel();

    /** Clears the panel and unbinds from the inventory signal. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Panel")
    void ClearPanel();

    // ---- Blueprint Events ----

protected:
    /** Called after the panel layout has been constructed for the new inventory. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Panel")
    void OnPanelConstructed(
        FCk_Handle_Inventory InInventory);

    /** Called after items have been refreshed. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Inventory|Panel")
    void OnPanelRefreshed();

    /**
     * Called when a drag-drop operation is received on this panel.
     * Return true to suppress the default drop handling (cross-inventory transfer).
     */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Inventory|Panel")
    bool OnDropReceived(
        UCk_InventoryUI_DragDropOperation* InOperation);

    // ---- Subclass Interface ----

protected:
    /** Subclass constructs its layout widgets (grid cells, list entries, etc.). */
    virtual auto DoConstruct() -> void PURE_VIRTUAL(UCk_InventoryUI_InventoryPanel::DoConstruct, );

    /** Subclass refreshes item placement into its layout. */
    virtual auto DoRefresh() -> void PURE_VIRTUAL(UCk_InventoryUI_InventoryPanel::DoRefresh, );

    /** Subclass clears its layout widgets. */
    virtual auto DoClear() -> void PURE_VIRTUAL(UCk_InventoryUI_InventoryPanel::DoClear, );

    // ---- Inject (called by subclass typed API) ----

protected:
    auto DoInjectInventory(const FCk_Handle_Inventory& InInventory) -> void;

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

    // ---- Internal Signal Management ----

private:
    auto DoBindSignal() -> void;
    auto DoUnbindSignal() -> void;

    // ---- Configuration ----

protected:
    /** The slot widget class to instantiate for each grid cell or list entry. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Panel")
    TSubclassOf<UCk_InventoryUI_ItemSlotEntry> _SlotClass;

    /** The drag widget class used by item slots in this panel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Panel")
    TSubclassOf<UCk_InventoryUI_DragWidget> _DragWidgetClass;

    // ---- Data ----

protected:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _InventoryHandle;

    /** Slot widgets created by the subclass. */
    UPROPERTY()
    TArray<TObjectPtr<UCk_InventoryUI_ItemSlotEntry>> _Slots;

    bool _IsBound = false;

public:
    CK_PROPERTY_GET(_InventoryHandle);
};

// --------------------------------------------------------------------------------------------------------------------
