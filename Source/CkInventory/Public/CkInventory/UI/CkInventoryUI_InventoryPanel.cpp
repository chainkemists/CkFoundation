#include "CkInventory/UI/CkInventoryUI_InventoryPanel.h"

#include "CkInventory/UI/CkInventoryUI_DragDropOperation.h"
#include "CkInventory/UI/CkInventoryUI_ItemSlotEntry.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

auto
    UCk_InventoryUI_InventoryPanel::
    RefreshPanel()
    -> void
{
    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return; }

    DoRefresh();
    DoApplyDragDropPolicyToSlots();
    OnPanelRefreshed();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryPanel::
    Set_DragDropPolicy(
        const FCk_InventoryUI_DragDropPolicy& InPolicy)
    -> void
{
    _DragDropPolicy = InPolicy;
    DoApplyDragDropPolicyToSlots();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryPanel::
    DoApplyDragDropPolicyToSlots()
    -> void
{
    for (const auto& ItemSlot : _Slots)
    {
        if (ck::Is_NOT_Valid(ItemSlot))
        { continue; }

        ItemSlot->Set_DragDropPolicy(_DragDropPolicy);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryPanel::
    ClearPanel()
    -> void
{
    DoUnbindSignal();
    DoClear();

    _InventoryHandle = {};
}

auto
    UCk_InventoryUI_InventoryPanel::
    DoInjectInventory(
        const FCk_Handle_Inventory& InInventory)
    -> void
{
    if (_InventoryHandle == InInventory)
    { return; }

    // ---- Unbind previous ----

    if (_IsBound)
    {
        DoUnbindSignal();
    }

    DoClear();

    // ---- Bind new ----

    _InventoryHandle = InInventory;

    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return; }

    // ---- Construct and refresh ----

    DoConstruct();
    DoBindSignal();
    RefreshPanel();
    OnPanelConstructed(_InventoryHandle);
}

auto
    UCk_InventoryUI_InventoryPanel::
    NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return false; }

    // Reject cross-inventory drag-over when drop-in is disallowed, so the cursor
    // reflects rejection. Same-inventory drag-over stays valid (intra-panel rearrange).
    if (NOT _DragDropPolicy.Get_AllowDropIn() &&
        InventoryOp->Get_SourceInventory() != _InventoryHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryPanel::
    NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation)
    -> bool
{
    auto* const InventoryOp = Cast<UCk_InventoryUI_DragDropOperation>(InOperation);

    if (ck::Is_NOT_Valid(InventoryOp))
    { return false; }

    // ---- Let Blueprint handle first ----

    if (OnDropReceived(InventoryOp))
    { return true; }

    // ---- Default handling: transfer between inventories ----

    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return false; }

    const auto SourceItem = InventoryOp->Get_SourceItem();
    const auto SourceInventory = InventoryOp->Get_SourceInventory();

    if (ck::Is_NOT_Valid(SourceItem))
    { return false; }

    // Only handle cross-inventory transfers in the default implementation.
    if (SourceInventory == _InventoryHandle)
    { return false; }

    // Drop-in gated by the panel's drag-drop policy. This is a safety net for drops on
    // non-slot panel area — slot drops are gated earlier by UCk_InventoryUI_ItemSlotEntry.
    if (NOT _DragDropPolicy.Get_AllowDropIn())
    { return false; }

    // Source / target are base-typed in the UI layer. Source dispatch is handled by the base Utils
    // internally; target type still drives which Transfer variant we call.
    auto SourceMutable = const_cast<FCk_Handle_Inventory&>(SourceInventory);
    auto TargetMutable = _InventoryHandle;
    const auto Callback = FCk_Delegate_Inventory_OnOperationResult_Transfer{};

    if (UCk_Utils_Inventory_UE::Get_IsSpatial(TargetMutable))
    {
        auto TgtSpatial = UCk_Utils_Inventory_Spatial_UE::DoCastChecked(TargetMutable);
        UCk_Utils_Inventory_UE::Request_TransferItem_ToSpatial(
            SourceMutable,
            FCk_Request_Inventory_TransferItem_ToSpatial(SourceItem, TgtSpatial),
            Callback);
    }
    else
    {
        auto TgtDataOnly = UCk_Utils_Inventory_DataOnly_UE::DoCastChecked(TargetMutable);
        UCk_Utils_Inventory_UE::Request_TransferItem_ToDataOnly(
            SourceMutable,
            FCk_Request_Inventory_TransferItem_ToDataOnly(SourceItem, TgtDataOnly),
            Callback);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryPanel::
    OnDropReceived_Implementation(
        UCk_InventoryUI_DragDropOperation* InOperation)
    -> bool
{
    return false;
}

auto
    UCk_InventoryUI_InventoryPanel::
    HandleOnItemsChanged(
        FCk_Handle_Inventory InInventory,
        const TArray<FCk_Handle_Item>& InItemsAdded,
        const TArray<FCk_Handle_Item>& InItemsRemoved)
    -> void
{
    RefreshPanel();
}

auto
    UCk_InventoryUI_InventoryPanel::
    NativeDestruct()
    -> void
{
    DoUnbindSignal();

    Super::NativeDestruct();
}

auto
    UCk_InventoryUI_InventoryPanel::
    DoBindSignal()
    -> void
{
    if (_IsBound)
    { return; }

    if (ck::Is_NOT_Valid(_InventoryHandle))
    { return; }

    FCk_Delegate_Inventory_OnItemsChanged Delegate;
    Delegate.BindDynamic(this, &ThisClass::HandleOnItemsChanged);

    UCk_Utils_Inventory_UE::BindTo_OnItemsChanged(
        _InventoryHandle,
        Delegate);

    _IsBound = true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryUI_InventoryPanel::
    DoUnbindSignal()
    -> void
{
    if (NOT _IsBound)
    { return; }

    if (ck::Is_NOT_Valid(_InventoryHandle))
    {
        _IsBound = false;
        return;
    }

    FCk_Delegate_Inventory_OnItemsChanged Delegate;
    Delegate.BindDynamic(this, &ThisClass::HandleOnItemsChanged);

    UCk_Utils_Inventory_UE::UnbindFrom_OnItemsChanged(
        _InventoryHandle,
        Delegate);

    _IsBound = false;
}

// --------------------------------------------------------------------------------------------------------------------
