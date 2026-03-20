#include "CkItemFragment_Stackable_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Definition.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.inl.h"
#include "CkInventory/Item/ItemFragments/CkItemFragment_Stackable.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

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
