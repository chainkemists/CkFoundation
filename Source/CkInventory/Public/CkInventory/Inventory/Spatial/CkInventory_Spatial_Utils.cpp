#include "CkInventory_Spatial_Utils.h"

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/CkGrid_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Make_Params(
        FGameplayTag InName,
        FIntPoint InDimensions,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems)
    -> FCk_Fragment_Inventory_Spatial_ParamsData
{
    auto Params = FCk_Fragment_Inventory_Spatial_ParamsData(InName, InDimensions);
    Params.Set_CustomCanAcceptItemDynamic(InCustomCanAcceptItem);
    Params.Set_CustomCanStackItemsDynamic(InCustomCanStackItems);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Add(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_Spatial_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> FCk_Handle_Inventory_Spatial
{
    const auto Base = ck::CreateInventory<ck::TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>>(
        InOwnerEntity, FCk_Fragment_Inventory_ParamsData{InParams}, InReplicates, InWorldContextObject);

    return Cast(Base);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    AddMultiple(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_MultipleInventory_Spatial_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_Inventory_Spatial>
{
    return ck::algo::Transform<TArray<FCk_Handle_Inventory_Spatial>>(
        InParams.Get_InventoryParams(),
        [&](const FCk_Fragment_Inventory_Spatial_ParamsData& InParam)
    {
        return Add(InOwnerEntity, InParam, InReplicates, InWorldContextObject);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_AddItem(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_SpatialPlacement& InPlacement,
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate)
    -> FCk_Handle_Inventory_Spatial
{
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_AddItem: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Add,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Spatial_Requests>()._Requests.Emplace(
        ck::FFragment_Inventory_Spatial_Requests::AddItemEntry{InRequest, InPlacement});
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_SplitStack(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Request_Inventory_SplitStack& InRequest,
        const FCk_SpatialPlacement& InPlacement,
        const FCk_Delegate_Inventory_OnOperationResult_Split& InDelegate)
    -> FCk_Handle_Inventory_Spatial
{
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_SplitStack: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Split,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Spatial_Requests>()._Requests.Emplace(
        ck::FFragment_Inventory_Spatial_Requests::SplitStackEntry{InRequest, InPlacement});
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_RelocateItem(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Request_Inventory_Spatial_RelocateItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Relocate& InDelegate)
    -> FCk_Handle_Inventory_Spatial
{
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_RelocateItem: No authority over inventory [{}]"), InInventory)
    { return InInventory; }

    auto Request = InRequest;
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Relocate,
        Request.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Spatial_Requests>()._Requests.Emplace(
        ck::FFragment_Inventory_Spatial_Requests::RelocateItemEntry{MoveTemp(Request)});
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_inventory
{
    auto Get_ItemActiveCells(const FCk_Handle_Item& InItem) -> TArray<FIntPoint>
    {
        if (auto GridHandle = UCk_Utils_2dGridSystem_UE::Cast(InItem);
            ck::IsValid(GridHandle))
        {
            return UCk_Utils_2dGridSystem_UE::Get_AllCells_AsCoordinate(
                GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells);
        }

        // Fallback: 1x1 item without a Dimensions fragment
        return { FIntPoint(0, 0) };
    }

    auto CardinalRotationToYaw(ECk_CardinalRotation InRotation) -> float
    {
        switch (InRotation)
        {
            case ECk_CardinalRotation::None:          return 0.0f;
            case ECk_CardinalRotation::Quarter:       return 90.0f;
            case ECk_CardinalRotation::Half:          return 180.0f;
            case ECk_CardinalRotation::ThreeQuarter:  return 270.0f;
            default:                                  return 0.0f;
        }
    }

    auto YawToCardinalRotation(float InYaw) -> ECk_CardinalRotation
    {
        // Normalize to [0, 360)
        auto Yaw = FMath::Fmod(InYaw, 360.0f);
        if (Yaw < 0.0f) { Yaw += 360.0f; }

        // Quantize to nearest 90°
        const auto Quantized = FMath::RoundToInt(Yaw / 90.0f) % 4;

        switch (Quantized)
        {
            case 0:  return ECk_CardinalRotation::None;
            case 1:  return ECk_CardinalRotation::Quarter;
            case 2:  return ECk_CardinalRotation::Half;
            case 3:  return ECk_CardinalRotation::ThreeQuarter;
            default: return ECk_CardinalRotation::None;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Inventory_Spatial_UE,
    FCk_Handle_Inventory_Spatial,
    ck::FTag_Inventory_Spatial);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_Dimensions(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> FIntPoint
{
    const auto GridHandle = Get_Grid(InInventory);
    return UCk_Utils_2dGridSystem_UE::Get_Dimensions(GridHandle);
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_NumFreeCells(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> int32
{
    const auto GridHandle = Get_Grid(InInventory);

    auto FreeCount = 0;

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&](const FCk_Handle_2dGridCell& InCell)
    {
        if (ck::Is_NOT_Valid(ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell)))
        { ++FreeCount; }
    });

    return FreeCount;
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemPlacementRotation(
        const FCk_Handle_Item& InItem)
    -> ECk_CardinalRotation
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);

    if (ck::Is_NOT_Valid(TransformHandle))
    { return ECk_CardinalRotation::None; }

    const auto Rotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(TransformHandle);
    return ck_inventory::YawToCardinalRotation(Rotation.Yaw);
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemActiveCells_Rotated(
        const FCk_Handle_Item& InItem,
        ECk_CardinalRotation InRotation)
    -> TArray<FIntPoint>
{
    auto BaseCells = ck_inventory::Get_ItemActiveCells(InItem);
    return UCk_Utils_Grid2D_UE::Get_RotatedShape(BaseCells, InRotation);
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemPlacementCoordinate(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem)
    -> FIntPoint
{
    auto Coordinate = ck::Inventory::AutoPlaceCoordinate;

    const auto& GridHandle = Get_Grid(InInventory);

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&](const FCk_Handle_2dGridCell& InCell)
    {
        if (const auto& StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
            StoredEntity != InItem)
        { return; }

        const auto Local = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

        // Multi-cell items mark every occupied cell with the same item ref. ForEach_Cell
        // iteration order is not guaranteed to hit the anchor first, so reduce to the
        // lexicographic minimum (Y then X) — which equals the placement anchor because
        // Get_RotatedShape normalizes rotated shapes so MinX=MinY=0.
        if (Coordinate.X < 0 ||
            Local.Y < Coordinate.Y ||
            (Local.Y == Coordinate.Y && Local.X < Coordinate.X))
        {
            Coordinate = Local;
        }
    });

    return Coordinate;
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemAtCoordinate(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FIntPoint& InCoordinate)
    -> FCk_Handle_Item
{
    const auto& GridHandle = Get_Grid(InInventory);
    const auto& CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, InCoordinate);
    return ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(CellHandle);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal spatial helpers (used by processors via friend access)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_Grid(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> FCk_Handle_2dGridSystem
{
    return UCk_Utils_2dGridSystem_UE::CastChecked(InInventory);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    DoCanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate,
        ECk_CardinalRotation InRotation)
    -> bool
{
    if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InInventory))
    { return false; }

    const auto& GridHandle = Get_Grid(InInventory);
    const auto RotatedCells = Get_ItemActiveCells_Rotated(InItem, InRotation);

    // ---- Check bounds and disabled via grid-level utility ----

    if (NOT UCk_Utils_2dGridSystem_UE::Get_CanFitShapeAt(GridHandle, RotatedCells, InCoordinate))
    { return false; }

    // ---- Check occupancy (inventory layer) ----

    return ck::algo::NoneOf(RotatedCells, [&](const FIntPoint& CellOffset)
    {
        const auto& Coord = InCoordinate + CellOffset;
        const auto& CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, Coord);
        return ck::IsValid(ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(CellHandle));
    });
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_inventory
{
    static constexpr ECk_CardinalRotation AllRotations[] =
    {
        ECk_CardinalRotation::None,
        ECk_CardinalRotation::Quarter,
        ECk_CardinalRotation::Half,
        ECk_CardinalRotation::ThreeQuarter
    };
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_CanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate)
    -> FCk_SpatialPlacementResult
{
    for (const auto Rotation : ck_inventory::AllRotations)
    {
        if (DoCanPlaceItemAt(InInventory, InItem, InCoordinate, Rotation))
        { return FCk_SpatialPlacementResult::Success(InCoordinate, Rotation); }
    }

    return FCk_SpatialPlacementResult::Failed();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_FirstAvailablePlacement(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem)
    -> FCk_SpatialPlacementResult
{
    if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InInventory))
    { return FCk_SpatialPlacementResult::Failed(); }

    const auto& GridHandle = Get_Grid(InInventory);
    const auto& GridDimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(GridHandle);

    for (const auto Rotation : ck_inventory::AllRotations)
    {
        for (auto Y = 0; Y < GridDimensions.Y; ++Y)
        {
            for (auto X = 0; X < GridDimensions.X; ++X)
            {
                if (DoCanPlaceItemAt(InInventory, InItem, FIntPoint{X, Y}, Rotation))
                { return FCk_SpatialPlacementResult::Success(FIntPoint{X, Y}, Rotation); }
            }
        }
    }

    return FCk_SpatialPlacementResult::Failed();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_PlaceItemOnGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate,
        ECk_CardinalRotation InRotation)
    -> void
{
    const auto& GridHandle = Get_Grid(InInventory);
    const auto& RotatedCells = Get_ItemActiveCells_Rotated(InItem, InRotation);

    ck::algo::ForEach(RotatedCells, [&](const FIntPoint& CellOffset)
    {
        const auto& Coord = InCoordinate + CellOffset;

        if (auto CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, Coord);
            ck::IsValid(CellHandle))
        {
            ck::TUtils_InventorySlot_ItemRef::AddOrReplace(CellHandle, InItem);
        }
    });

    // ---- Store rotation on item's Transform ----

    if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);
        ck::IsValid(TransformHandle))
    {
        const auto Yaw = ck_inventory::CardinalRotationToYaw(InRotation);
        UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, FCk_Request_Transform_SetRotation{FRotator{0.0, Yaw, 0.0}});
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_RemoveItemFromGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem)
    -> void
{
    const auto& GridHandle = Get_Grid(InInventory);

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&InItem](FCk_Handle_2dGridCell InCell)
    {
        if (const auto& StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
            StoredEntity == InItem)
        {
            ck::TUtils_InventorySlot_ItemRef::Clear(InCell);
        }
    });

    // ---- Reset rotation on item's Transform ----

    if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);
        ck::IsValid(TransformHandle))
    {
        UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, FCk_Request_Transform_SetRotation{FRotator::ZeroRotator});
    }
}
