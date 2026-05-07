#include "CkInventory_DataOnly_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_DataOnly_ParamsData::
FCk_Fragment_Inventory_DataOnly_ParamsData(
    FGameplayTag InName, TOptional<int32> InBoundLimit)
    : _Name(InName)
{
    if (InBoundLimit.IsSet())
    {
        _BoundMode = ECk_Inventory_DataOnly_BoundMode::Bounded;
        _BoundLimit = InBoundLimit.GetValue();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto 
    FCk_InventoryItem_DataOnly_ReplicatedEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _ItemHandle == InOther._ItemHandle;
}
