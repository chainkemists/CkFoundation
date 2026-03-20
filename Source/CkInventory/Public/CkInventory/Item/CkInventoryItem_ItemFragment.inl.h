#pragma once

#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"
#include "CkInventory/Item/CkInventoryItem_Fragment.h"
#include "CkInventory/Item/CkInventoryItem_Definition.h"

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
auto
    FCk_ItemFragment::
    Get(const FCk_Handle_Item& InItem)
    -> const T*
{
    const auto& Params = InItem.Get<ck::FFragment_InventoryItem_Params>();
    const auto* Definition = Params.Get_Definition().Get();

    if (ck::Is_NOT_Valid(Definition))
    { return nullptr; }

    return Definition->Get_ItemFragment<T>();
}

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
auto
    FCk_ItemFragment::
    Has(const FCk_Handle_Item& InItem)
    -> bool
{
    return ck::IsValid(Get<T>(InItem), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------
