#include "CkInventory_Spatial_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Inventory_Spatial_Spec::
FCk_Inventory_Spatial_Spec(
    FGameplayTag InName, FIntPoint InDimensions)
    : _Name(InName)
    , _Dimensions(InDimensions)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InventoryItem_Spatial_ReplicatedEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _ItemHandle == InOther._ItemHandle
        && _Coordinate == InOther._Coordinate
        && _Rotation == InOther._Rotation;
}
