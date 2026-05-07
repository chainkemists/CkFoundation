#pragma once

#include "CkInventory/UI/CkInventoryUI_InventoryPanel.h"

#include <Components/UniformGridPanel.h>

#include "CkInventoryUI_SpatialPanel.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Inventory panel for Spatial inventories.
 *
 * Creates a grid of item slot widgets in a UUniformGridPanel based on the
 * inventory's grid dimensions. Each cell maps 1:1 to a grid coordinate.
 *
 * Blueprint setup:
 *  - Add a UUniformGridPanel named "_GridPanel" in the Designer
 *  - Set _SlotClass and _DragWidgetClass in defaults
 *  - Set _SlotSize for per-cell dimensions
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKINVENTORY_API UCk_InventoryUI_SpatialPanel : public UCk_InventoryUI_InventoryPanel
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_SpatialPanel);

public:
    /** Binds to a spatial inventory, constructs the grid, and populates items. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Inventory|Panel",
              DisplayName = "Inject Spatial Inventory")
    void InjectInventory(
        const FCk_Handle_Inventory_Spatial& InInventory);

protected:
    auto DoConstruct() -> void override;
    auto DoRefresh() -> void override;
    auto DoClear() -> void override;

protected:
    /** Desired size for each slot widget in the grid. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|UI|Inventory|Panel")
    FVector2D _SlotSize = FVector2D(64.0, 64.0);

protected:
    /** Uniform grid panel for the spatial layout.
     *  Must be named "_GridPanel" in Blueprint. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> _GridPanel;

private:
    /** Cached dimensions for coordinate-to-index mapping. */
    FIntPoint _Dimensions = FIntPoint::ZeroValue;
};

// --------------------------------------------------------------------------------------------------------------------
