#include "CkInventory_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InventoryItem_ReplicatedEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _ItemHandle == InOther._ItemHandle && _Coordinate == InOther._Coordinate;
}

// --------------------------------------------------------------------------------------------------------------------
