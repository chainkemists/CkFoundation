#include "CkInventory_Spatial_Utils.h"

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"
#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkInventory/CkInventory_Stats.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/CkGrid_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Inventory::FirstAvailablePlacement"), STAT_Inventory_FirstAvailablePlacement, STATGROUP_CkInventory);
DECLARE_CYCLE_STAT(TEXT("Inventory::CanPlaceItemAt"), STAT_Inventory_CanPlaceItemAt, STATGROUP_CkInventory);
DECLARE_DWORD_COUNTER_STAT(TEXT("Inventory Cells Tested"), STAT_Inventory_CellsTested, STATGROUP_CkInventory);

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
    auto Base = ck::CreateInventory<ck::TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>>(
        InOwnerEntity, FCk_Fragment_Inventory_ParamsData{InParams}, InReplicates, InWorldContextObject);

    // Without this the ECS Debugger shows NONAME; a caller's later Set_DebugName still overrides it.
    UCk_Utils_Handle_UE::Set_DebugName(Base, InParams.Get_Name().GetTagName());

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
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> FCk_Handle_Inventory_Spatial
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InInventory);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_AddItem: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    {
        InCompletionDelegate.ExecuteIfBound(InInventory, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    if (InCompletionDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InCompletionDelegate); }

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
        const FCk_Delegate_Inventory_OnOperationResult_Split& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> FCk_Handle_Inventory_Spatial
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InInventory);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_SplitStack: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    {
        InCompletionDelegate.ExecuteIfBound(InInventory, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    if (InCompletionDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InCompletionDelegate); }

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
        const FCk_Delegate_Inventory_OnOperationResult_Relocate& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> FCk_Handle_Inventory_Spatial
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InInventory);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_RelocateItem: No authority over inventory [{}]"), InInventory)
    {
        InCompletionDelegate.ExecuteIfBound(InInventory, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInventory;
    }

    auto Request = InRequest;

    if (InCompletionDelegate.IsBound())
    { Request.Set_CompletionDelegate(InCompletionDelegate); }

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
        auto Yaw = FMath::Fmod(InYaw, 360.0f);
        if (Yaw < 0.0f)
        { Yaw += 360.0f; }

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
    // Transform-yaw derivation below is the fallback for items placed before this fragment existed.
    if (auto Item = InItem;
        Item.Has<ck::FFragment_Item_SpatialPlacement>())
    { return Item.Get<ck::FFragment_Item_SpatialPlacement>().Get_Rotation(); }

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
    // The cell scan below is the fallback for items placed before this fragment existed.
    if (auto Item = InItem;
        Item.Has<ck::FFragment_Item_SpatialPlacement>())
    { return Item.Get<ck::FFragment_Item_SpatialPlacement>().Get_Anchor(); }

    auto Coordinate = ck::Inventory::AutoPlaceCoordinate;

    // A Spatial inventory without a (re)composed grid — e.g. freshly snapshot-restored before its
    // restore processor runs — has nothing to scan; walking the tombstone handle would crash.
    const auto& GridHandle = Get_Grid(InInventory);
    if (ck::Is_NOT_Valid(GridHandle))
    { return Coordinate; }

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&](const FCk_Handle_2dGridCell& InCell)
    {
        if (const auto& StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
            StoredEntity != InItem)
        { return; }

        const auto Local = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

        // Multi-cell items mark every occupied cell and ForEach_Cell may not hit the anchor first,
        // so reduce to the lexicographic minimum (Y then X) — which IS the anchor, because
        // Get_RotatedShape normalizes rotated shapes to MinX = MinY = 0.
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
    SCOPE_CYCLE_COUNTER(STAT_Inventory_CanPlaceItemAt);

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
        INC_DWORD_STAT(STAT_Inventory_CellsTested);

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
    SCOPE_CYCLE_COUNTER(STAT_Inventory_FirstAvailablePlacement);

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

    // ---- Store the placement decision record on the item ----
    // The cell ItemRefs above are DERIVED state in the grid's private cell registry, invisible to
    // snapshots; this fragment is the persistent, O(1)-readable home for the placement.

    {
        auto Item = InItem;
        auto& Placement = Item.AddOrGet<ck::FFragment_Item_SpatialPlacement>();
        Placement._Anchor = InCoordinate;
        Placement._Rotation = InRotation;
    }

    // ---- Store rotation on item's Transform ----

    if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);
        ck::IsValid(TransformHandle))
    {
        const auto Yaw = ck_inventory::CardinalRotationToYaw(InRotation);
        UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, FCk_Request_Transform_SetRotation{FRotator{0.0, Yaw, 0.0}}, {});
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

    // ---- Drop the placement decision record ----

    {
        auto Item = InItem;
        Item.Try_Remove<ck::FFragment_Item_SpatialPlacement>();
    }

    // ---- Reset rotation on item's Transform ----

    if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);
        ck::IsValid(TransformHandle))
    {
        UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, FCk_Request_Transform_SetRotation{FRotator::ZeroRotator}, {});
    }
}
