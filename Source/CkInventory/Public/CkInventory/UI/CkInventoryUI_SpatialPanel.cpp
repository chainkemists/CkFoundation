#include "CkInventory/UI/CkInventoryUI_SpatialPanel.h"

#include "CkInventory/UI/CkInventoryUI_ItemSlotEntry.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Components/SizeBox.h>

// ---- Public API ----

auto
    UCk_InventoryUI_SpatialPanel::
    InjectInventory(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> void
{
    DoInjectInventory(InInventory);
}

// ---- Subclass Interface ----

auto
    UCk_InventoryUI_SpatialPanel::
    DoConstruct()
    -> void
{
    // ---- Read dimensions ----

    const auto SpatialHandle = ck::StaticCast<FCk_Handle_Inventory_Spatial>(static_cast<const FCk_Handle&>(_InventoryHandle));
    _Dimensions = UCk_Utils_Inventory_Spatial_UE::Get_Dimensions(SpatialHandle);

    if (_Dimensions.X <= 0 || _Dimensions.Y <= 0)
    { return; }

    if (ck::Is_NOT_Valid(_GridPanel))
    { return; }

    if (ck::Is_NOT_Valid(_SlotClass))
    { return; }

    // ---- Create slot widgets ----

    const auto TotalSlots = _Dimensions.X * _Dimensions.Y;
    _Slots.Reserve(TotalSlots);

    for (auto Row = int32{0}; Row < _Dimensions.Y; ++Row)
    {
        for (auto Col = int32{0}; Col < _Dimensions.X; ++Col)
        {
            auto* const SlotWidget = CreateWidget<UCk_InventoryUI_ItemSlotEntry>(this, _SlotClass);

            if (ck::Is_NOT_Valid(SlotWidget))
            { continue; }

            // ---- Propagate drag widget class ----

            SlotWidget->Set_DragWidgetClass(_DragWidgetClass);

            // ---- Wrap in SizeBox for consistent sizing ----

            auto* const SizeBox = NewObject<USizeBox>(this);
            SizeBox->SetWidthOverride(_SlotSize.X);
            SizeBox->SetHeightOverride(_SlotSize.Y);
            SizeBox->AddChild(SlotWidget);

            // ---- Add to grid ----
            // UUniformGridPanel fills top-to-bottom, then left-to-right.
            // We use (Row, Col) so the grid fills left-to-right per row.

            _GridPanel->AddChildToUniformGrid(SizeBox, Row, Col);

            _Slots.Add(SlotWidget);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_SpatialPanel::
    DoRefresh()
    -> void
{
    if (_Slots.IsEmpty())
    { return; }

    // ---- Clear all slots ----

    const auto InvalidItem = FCk_Handle_Item{};

    for (const auto& SlotEntry : _Slots)
    {
        if (IsValid(SlotEntry))
        {
            SlotEntry->InjectItemData(InvalidItem, _InventoryHandle);
        }
    }

    // ---- Iterate grid cells and place items into their corresponding slots ----

    const auto SpatialHandle = ck::StaticCast<FCk_Handle_Inventory_Spatial>(static_cast<const FCk_Handle&>(_InventoryHandle));
    const auto GridHandle = UCk_Utils_Inventory_Spatial_UE::Get_Grid(SpatialHandle);

    if (ck::Is_NOT_Valid(GridHandle))
    { return; }

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&](FCk_Handle_2dGridCell InCell)
    {
        if (NOT ck::TUtils_InventorySlot_ItemRef::Has(InCell))
        { return; }

        const auto StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);

        if (ck::Is_NOT_Valid(StoredEntity))
        { return; }

        const auto ItemHandle = ck::StaticCast<FCk_Handle_Item>(StoredEntity);
        const auto Coordinate = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

        // ---- Map coordinate to slot index ----

        const auto SlotIndex = Coordinate.Y * _Dimensions.X + Coordinate.X;

        if (NOT _Slots.IsValidIndex(SlotIndex))
        { return; }

        if (auto* const TargetSlot = _Slots[SlotIndex].Get(); IsValid(TargetSlot))
        {
            TargetSlot->InjectItemData(ItemHandle, _InventoryHandle);
        }
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_SpatialPanel::
    DoClear()
    -> void
{
    _Slots.Empty();
    _Dimensions = FIntPoint::ZeroValue;

    if (IsValid(_GridPanel))
    {
        _GridPanel->ClearChildren();
    }
}

// --------------------------------------------------------------------------------------------------------------------
