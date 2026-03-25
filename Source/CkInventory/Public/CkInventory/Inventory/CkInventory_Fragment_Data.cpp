#include "CkInventory_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Category_Inventory, TEXT("Inventory"));
UE_DEFINE_GAMEPLAY_TAG(TAG_IntegerAttribute_InventoryBoundMax, TEXT("IntegerAttribute.Inventory.BoundMax"));

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(FGameplayTag InName, TOptional<int32> InBoundLimit)
    : _Name(InName)
    , _InventoryType(ECk_InventoryType::DataOnly)
    , _BoundMode(InBoundLimit.IsSet()
        ? ECk_Inventory_DataOnly_BoundMode::Bounded
        : ECk_Inventory_DataOnly_BoundMode::Unbounded)
    , _BoundLimit(InBoundLimit.IsSet() ? InBoundLimit.GetValue() : 10)
{}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(FGameplayTag InName, FIntPoint InDimensions)
    : _Name(InName), _InventoryType(ECk_InventoryType::Spatial), _Dimensions(InDimensions)
{}

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
