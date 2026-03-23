#include "CkItemFragment_Stackable_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Definition.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.inl.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"
#include "CkInventory/Item/ItemFragments/CkItemFragment_Stackable.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_IsStackable(
        const FCk_Handle_Item& InItem)
    -> bool
{
    return FCk_ItemFragment::Has<FCk_ItemFragment_Stackable>(InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_StackCount(
        const FCk_Handle_Item& InItem)
    -> int32
{
    const auto Attribute = UCk_Utils_IntegerAttribute_UE::TryGet(
        InItem, TAG_IntegerAttribute_InventoryItem_StackCount);

    if (ck::Is_NOT_Valid(Attribute))
    { return 0; }

    return UCk_Utils_IntegerAttribute_UE::Get_FinalValue(Attribute);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_MaxStackSize(
        const FCk_Handle_Item& InItem)
    -> int32
{
    const auto* Fragment = FCk_ItemFragment::Get<FCk_ItemFragment_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Fragment, ck::IsValid_Policy_NullptrOnly{}))
    { return 0; }

    if (NOT Fragment->Get_HasMaxStackSize())
    { return -1; }

    return Fragment->Get_MaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_HasMaxStackSize(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Fragment = FCk_ItemFragment::Get<FCk_ItemFragment_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Fragment, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return Fragment->Get_HasMaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_IsStackFull(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Fragment = FCk_ItemFragment::Get<FCk_ItemFragment_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Fragment, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    if (NOT Fragment->Get_HasMaxStackSize())
    { return false; }

    return Get_StackCount(InItem) >= Fragment->Get_MaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Request_OverrideStackCount(
        const FCk_Handle_Item& InItem,
        int32 InNewCount)
    -> void
{
    auto Attr = UCk_Utils_IntegerAttribute_UE::TryGet(InItem, TAG_IntegerAttribute_InventoryItem_StackCount);
    UCk_Utils_IntegerAttribute_UE::Request_Override(Attr, InNewCount);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    DoFillExistingStacks(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        int32 InCount,
        const FCk_Handle_Item& InSourceItem)
    -> int32
{
    const auto* StackableFragment = InDefinition->Get_ItemFragment<FCk_ItemFragment_Stackable>();

    if (ck::Is_NOT_Valid(StackableFragment, ck::IsValid_Policy_NullptrOnly{}))
    { return 0; }

    const auto HasMax = StackableFragment->Get_HasMaxStackSize();
    const auto CheckCompatibility = ck::IsValid(InSourceItem);
    auto Filled = int32{0};
    auto Remaining = InCount;

    for (const auto ExistingItems = UCk_Utils_Inventory_UE::Get_Items(InInventory);
        const auto& ExistingItem : ExistingItems)
    {
        if (Remaining <= 0)
        { break; }

        if (UCk_Utils_InventoryItem_UE::Get_Definition(ExistingItem) != InDefinition)
        { continue; }

        if (CheckCompatibility && NOT InDefinition->CanStackWith(InSourceItem, ExistingItem))
        { continue; }

        if (Get_IsStackFull(ExistingItem))
        { continue; }

        const auto CurrentCount = Get_StackCount(ExistingItem);
        const auto MaxStack = HasMax
            ? Get_MaxStackSize(ExistingItem)
            : MAX_int32;

        const auto Space    = MaxStack - CurrentCount;
        const auto Transfer = FMath::Min(Remaining, Space);

        if (Transfer <= 0)
        { continue; }

        Request_OverrideStackCount(ExistingItem, CurrentCount + Transfer);

        Remaining -= Transfer;
        Filled    += Transfer;
    }

    return Filled;
}

// --------------------------------------------------------------------------------------------------------------------
// Signals
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    BindTo_OnStackCountChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Item
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Stackable_OnStackCountChanged, InItem, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    UnbindFrom_OnStackCountChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate)
    -> FCk_Handle_Item
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Stackable_OnStackCountChanged, InItem, InDelegate);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------
