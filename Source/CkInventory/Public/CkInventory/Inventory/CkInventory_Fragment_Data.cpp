#include "CkInventory_Fragment_Data.h"

#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment_Data.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Category_Inventory, TEXT("Inventory"));
UE_DEFINE_GAMEPLAY_TAG(TAG_IntegerAttribute_InventoryBoundMax, TEXT("IntegerAttribute.Inventory.BoundMax"));

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_DataOnly_ParamsData& InDataOnlyParams)
    : _Name(InDataOnlyParams.Get_Name())
    , _InventoryType(ECk_InventoryType::DataOnly)
    , _BoundMode(InDataOnlyParams.Get_BoundMode())
    , _BoundLimit(InDataOnlyParams.Get_BoundLimit())
    , _StackingPolicy(InDataOnlyParams.Get_StackingPolicy())
    , _MaxStackSizeClamp(InDataOnlyParams.Get_MaxStackSizeClamp())
    , _PersistContents(InDataOnlyParams.Get_PersistContents())
    , _CustomCanAcceptItem(InDataOnlyParams.Get_CustomCanAcceptItem())
    , _CustomCanAcceptItemDynamic(InDataOnlyParams.Get_CustomCanAcceptItemDynamic())
    , _CanAcceptItemRef(InDataOnlyParams.Get_CanAcceptItemRef())
    , _CustomCanStackItems(InDataOnlyParams.Get_CustomCanStackItems())
    , _CustomCanStackItemsDynamic(InDataOnlyParams.Get_CustomCanStackItemsDynamic())
    , _CanStackItemsRef(InDataOnlyParams.Get_CanStackItemsRef())
    , _CustomGetAbsorbableUnits(InDataOnlyParams.Get_CustomGetAbsorbableUnits())
    , _CustomGetAbsorbableUnitsDynamic(InDataOnlyParams.Get_CustomGetAbsorbableUnitsDynamic())
    , _GetAbsorbableUnitsRef(InDataOnlyParams.Get_GetAbsorbableUnitsRef())
{}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_Spatial_ParamsData& InSpatialParams)
    : _Name(InSpatialParams.Get_Name())
    , _InventoryType(ECk_InventoryType::Spatial)
    , _Dimensions(InSpatialParams.Get_Dimensions())
    , _StackingPolicy(InSpatialParams.Get_StackingPolicy())
    , _MaxStackSizeClamp(InSpatialParams.Get_MaxStackSizeClamp())
    , _CustomCanAcceptItem(InSpatialParams.Get_CustomCanAcceptItem())
    , _CustomCanAcceptItemDynamic(InSpatialParams.Get_CustomCanAcceptItemDynamic())
    , _CanAcceptItemRef(InSpatialParams.Get_CanAcceptItemRef())
    , _CustomCanStackItems(InSpatialParams.Get_CustomCanStackItems())
    , _CustomCanStackItemsDynamic(InSpatialParams.Get_CustomCanStackItemsDynamic())
    , _CanStackItemsRef(InSpatialParams.Get_CanStackItemsRef())
    , _CustomGetAbsorbableUnits(InSpatialParams.Get_CustomGetAbsorbableUnits())
    , _CustomGetAbsorbableUnitsDynamic(InSpatialParams.Get_CustomGetAbsorbableUnitsDynamic())
    , _GetAbsorbableUnitsRef(InSpatialParams.Get_GetAbsorbableUnitsRef())
{}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_SpatialPlacementResult::
    Success(
        FIntPoint InCoordinate,
        ECk_CardinalRotation InRotation)
    -> FCk_SpatialPlacementResult
{
    auto Result = FCk_SpatialPlacementResult{};
    Result._Succeeded = true;
    Result._Coordinate = InCoordinate;
    Result._Rotation = InRotation;
    return Result;
}

auto
    FCk_SpatialPlacementResult::
    Failed()
    -> FCk_SpatialPlacementResult
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------
