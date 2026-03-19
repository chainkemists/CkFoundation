#include "CkItemFragment_Stackable.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_IntegerAttribute_Inventory_StackCount, TEXT("IntegerAttribute.Inventory.StackCount"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ItemFragment_Stackable::
    OnApplied(
        FCk_Handle_Item& InItem) const
    -> void
{
    auto Params = FCk_Fragment_IntegerAttribute_ParamsData(TAG_IntegerAttribute_Inventory_StackCount, _InitialCount);
    Params.Set_MinValue(1);

    if (_HasMaxStackSize)
    {
        Params.Set_MinMax(ECk_MinMax::MinMax);
        Params.Set_MaxValue(_MaxStackSize);
    }
    else
    {
        Params.Set_MinMax(ECk_MinMax::Min);
    }

    UCk_Utils_IntegerAttribute_UE::Add(InItem, Params);
}

// --------------------------------------------------------------------------------------------------------------------
