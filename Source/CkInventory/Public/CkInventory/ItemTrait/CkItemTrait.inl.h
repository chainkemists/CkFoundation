#pragma once

#include "CkInventory/ItemTrait/CkItemTrait.h"
#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkInventory/Item/CkItem_Definition.h"

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
auto
    UCk_ItemTrait::
    Get(const FCk_Handle_Item& InItem)
    -> const T*
{
    const auto& ItemData = InItem.Get<ck::FFragment_InventoryItem>();
    const auto* Definition = ItemData.Get_Definition().Get();

    if (ck::Is_NOT_Valid(Definition))
    { return nullptr; }

    return Cast<T>(Definition->Get_ItemTrait<T>());
}

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
auto
    UCk_ItemTrait::
    Has(const FCk_Handle_Item& InItem)
    -> bool
{
    return ck::IsValid(Get<T>(InItem), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------
