#include "CkInventory_Fragment_Data.h"

#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment_Data.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment_Data.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_Inventory_ParamsData);

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Category_Inventory, TEXT("Inventory"));
UE_DEFINE_GAMEPLAY_TAG(TAG_IntegerAttribute_InventoryBoundMax, TEXT("IntegerAttribute.Inventory.BoundMax"));

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_DataOnly_ParamsData& InDataOnlyParams)
    : _Name(InDataOnlyParams.Get_Name())
    , _InventoryType(ECk_InventoryType::DataOnly)
    , _BoundMode(InDataOnlyParams.Get_BoundMode())
    , _BoundLimit(InDataOnlyParams.Get_BoundLimit())
    , _CustomCanAcceptItem(InDataOnlyParams.Get_CustomCanAcceptItem())
    , _CustomCanAcceptItemDynamic(InDataOnlyParams.Get_CustomCanAcceptItemDynamic())
    , _CanAcceptItemRef(InDataOnlyParams.Get_CanAcceptItemRef())
    , _CustomCanStackItems(InDataOnlyParams.Get_CustomCanStackItems())
    , _CustomCanStackItemsDynamic(InDataOnlyParams.Get_CustomCanStackItemsDynamic())
    , _CanStackItemsRef(InDataOnlyParams.Get_CanStackItemsRef())
{}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_ParamsData::FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_Spatial_ParamsData& InSpatialParams)
    : _Name(InSpatialParams.Get_Name())
    , _InventoryType(ECk_InventoryType::Spatial)
    , _Dimensions(InSpatialParams.Get_Dimensions())
    , _CustomCanAcceptItem(InSpatialParams.Get_CustomCanAcceptItem())
    , _CustomCanAcceptItemDynamic(InSpatialParams.Get_CustomCanAcceptItemDynamic())
    , _CanAcceptItemRef(InSpatialParams.Get_CanAcceptItemRef())
    , _CustomCanStackItems(InSpatialParams.Get_CustomCanStackItems())
    , _CustomCanStackItemsDynamic(InSpatialParams.Get_CustomCanStackItemsDynamic())
    , _CanStackItemsRef(InSpatialParams.Get_CanStackItemsRef())
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
