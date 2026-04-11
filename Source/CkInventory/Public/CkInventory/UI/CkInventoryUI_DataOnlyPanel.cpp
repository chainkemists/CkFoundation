#include "CkInventory/UI/CkInventoryUI_DataOnlyPanel.h"

#include "CkInventory/UI/CkInventoryUI_ItemSlotEntry.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Components/UniformGridPanel.h>

// ---- Public API ----

auto
    UCk_InventoryUI_DataOnlyPanel::
    InjectInventory(
        const FCk_Handle_Inventory_DataOnly& InInventory)
    -> void
{
    DoInjectInventory(InInventory);
}

// ---- Subclass Interface ----

auto
    UCk_InventoryUI_DataOnlyPanel::
    DoConstruct()
    -> void
{
    if (ck::Is_NOT_Valid(_ItemPanel))
    { return; }

    if (ck::Is_NOT_Valid(_SlotClass))
    { return; }

    // ---- Determine bounded vs unbounded ----

    const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::CastChecked(_InventoryHandle);
    const auto MaybeBound = UCk_Utils_Inventory_DataOnly_UE::Get_BoundMax(DataOnlyHandle);
    _IsBounded = MaybeBound.IsSet();

    if (_IsBounded)
    {
        _BoundLimit = FMath::Max(1, MaybeBound.GetValue());

        // ---- Pre-create fixed number of empty slots ----

        _Slots.Reserve(_BoundLimit);

        for (auto Index = int32{0}; Index < _BoundLimit; ++Index)
        {
            DoCreateSlot();
        }
    }

    // Unbounded: slots are created dynamically during DoRefresh.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_DataOnlyPanel::
    DoRefresh()
    -> void
{
    if (ck::Is_NOT_Valid(_ItemPanel))
    { return; }

    const auto Items = UCk_Utils_Inventory_UE::Get_Items(_InventoryHandle);

    if (_IsBounded)
    {
        DoRefresh_Bounded(Items);
    }
    else
    {
        DoRefresh_Unbounded(Items);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_DataOnlyPanel::
    DoClear()
    -> void
{
    _Slots.Empty();
    _IsBounded = false;
    _BoundLimit = 0;

    if (IsValid(_ItemPanel))
    {
        _ItemPanel->ClearChildren();
    }
}

// ---- Internal ----

auto
    UCk_InventoryUI_DataOnlyPanel::
    DoCreateSlot()
    -> UCk_InventoryUI_ItemSlotEntry*
{
    auto* const SlotWidget = CreateWidget<UCk_InventoryUI_ItemSlotEntry>(this, _SlotClass);

    if (ck::Is_NOT_Valid(SlotWidget))
    { return nullptr; }

    SlotWidget->Set_DragWidgetClass(_DragWidgetClass);

    if (auto* const GridPanel = Cast<UUniformGridPanel>(_ItemPanel))
    {
        const auto SlotIndex = _Slots.Num();
        const auto Columns = FMath::Max(1, _Columns);
        const auto Row    = SlotIndex / Columns;
        const auto Column = SlotIndex % Columns;

        GridPanel->AddChildToUniformGrid(SlotWidget, Row, Column);
    }
    else
    {
        _ItemPanel->AddChild(SlotWidget);
    }

    _Slots.Add(SlotWidget);

    return SlotWidget;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_DataOnlyPanel::
    DoRefresh_Bounded(
        const TArray<FCk_Handle_Item>& InItems)
    -> void
{
    // ---- Assign items to pre-created slots, clear any excess ----

    const auto InvalidItem = FCk_Handle_Item{};

    for (auto Index = int32{0}; Index < _Slots.Num(); ++Index)
    {
        auto* const ItemSlot = _Slots[Index].Get();

        if (ck::Is_NOT_Valid(ItemSlot))
        { continue; }

        if (InItems.IsValidIndex(Index))
        {
            ItemSlot->InjectItemData(InItems[Index], _InventoryHandle);
        }
        else
        {
            ItemSlot->InjectItemData(InvalidItem, _InventoryHandle);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_DataOnlyPanel::
    DoRefresh_Unbounded(
        const TArray<FCk_Handle_Item>& InItems)
    -> void
{
    const auto CurrentSlotCount = _Slots.Num();
    const auto ItemCount = InItems.Num();

    // ---- Remove excess slots ----

    if (CurrentSlotCount > ItemCount)
    {
        for (auto Index = CurrentSlotCount - 1; Index >= ItemCount; --Index)
        {
            if (auto* const ItemSlot = _Slots[Index].Get(); IsValid(ItemSlot))
            {
                ItemSlot->RemoveFromParent();
            }
        }

        _Slots.SetNum(ItemCount);
    }

    // ---- Add missing slots ----

    while (_Slots.Num() < ItemCount)
    {
        DoCreateSlot();
    }

    // ---- Inject item data ----

    for (auto Index = int32{0}; Index < ItemCount; ++Index)
    {
        if (auto* const ItemSlot = _Slots[Index].Get(); IsValid(ItemSlot))
        {
            ItemSlot->InjectItemData(InItems[Index], _InventoryHandle);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
