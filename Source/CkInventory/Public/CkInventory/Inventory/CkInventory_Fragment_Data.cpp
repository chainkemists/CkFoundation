#include "CkInventory_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Category_Inventory, TEXT("Inventory"));

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(FGameplayTag InName)
    : _Name(InName), _InventoryType(ECk_InventoryType::DataOnly)
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
