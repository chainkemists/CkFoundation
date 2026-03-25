#pragma once

#include "CkInventory/UI/CkInventoryUI_InventoryPanel.h"

#include <Components/PanelWidget.h>

#include "CkInventoryUI_DataOnlyPanel.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Inventory panel for DataOnly inventories.
 *
 * Creates item slot widgets and adds them to a UPanelWidget. The Blueprint
 * designer chooses the concrete panel type (UVerticalBox, UHorizontalBox,
 * UWrapBox, UUniformGridPanel, etc.) — any UPanelWidget subclass works.
 *
 * For bounded inventories, pre-creates BoundLimit empty slots on construct.
 * For unbounded inventories, creates one slot per item on refresh.
 *
 * Blueprint setup:
 *  - Add any UPanelWidget subclass named "_ItemPanel" in the Designer
 *  - Set _SlotClass and _DragWidgetClass in defaults
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKINVENTORY_API UCk_InventoryUI_DataOnlyPanel : public UCk_InventoryUI_InventoryPanel
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_DataOnlyPanel);

    // ---- Public API ----

public:
    /** Binds to a DataOnly inventory, constructs the layout, and populates items. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Panel",
              DisplayName = "Inject DataOnly Inventory")
    void InjectInventory(
        const FCk_Handle_Inventory_DataOnly& InInventory);

    // ---- Subclass Interface ----

protected:
    auto DoConstruct() -> void override;
    auto DoRefresh() -> void override;
    auto DoClear() -> void override;

    // ---- Internal ----

private:
    auto DoCreateSlot() -> UCk_InventoryUI_ItemSlotEntry*;
    auto DoRefresh_Bounded(const TArray<FCk_Handle_Item>& InItems) -> void;
    auto DoRefresh_Unbounded(const TArray<FCk_Handle_Item>& InItems) -> void;

    // ---- Bound Widgets ----

protected:
    /** Panel widget for the item layout.
     *  Must be named "_ItemPanel" in Blueprint.
     *  Can be any UPanelWidget subclass: VerticalBox, HorizontalBox, WrapBox, etc. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidget))
    TObjectPtr<UPanelWidget> _ItemPanel;

    // ---- Data ----

private:
    /** Whether the bound inventory is bounded (fixed slot count). */
    bool _IsBounded = false;

    /** Cached bound limit for bounded inventories. */
    int32 _BoundLimit = 0;

    /** Number of columns when _ItemPanel is a UUniformGridPanel. Ignored for other panel types. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Panel",
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _Columns = 1;
};

// --------------------------------------------------------------------------------------------------------------------
