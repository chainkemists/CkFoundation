#include "Ck2dGridCell_Utils.h"

#include "Ck2dGridCell_Fragment.h"

#include "CkCore/Math/Geometry/CkGeometry_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkGrid/CkGrid_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_2dGridCell_UE::
    Create(
        FCk_Handle_2dGridSystem& InParentGrid,
        const FCk_Fragment_2dGridCell_ParamsData& InParams,
        ECk_EnableDisable InEnabledState)
    -> FCk_Handle_2dGridCell
{
    auto CellEntity = InParentGrid.Get<ck::FFragment_2dGridSystem_Current>().Request_CreateCellEntity();
    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(CellEntity, InParentGrid);

    CellEntity.Add<ck::FFragment_2dGridCell_Params>(InParams);

    if (InEnabledState == ECk_EnableDisable::Disable)
    {
        CellEntity.Add<ck::FTag_2dGridCell_Disabled>();
    }

    return Cast(CellEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_2dGridCell_UE, FCk_Handle_2dGridCell, ck::FFragment_2dGridCell_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridCell_UE::
    Get_ParentGrid(
        const FCk_Handle_2dGridCell& InCell)
    -> FCk_Handle_2dGridSystem
{
    const auto ParentHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InCell);
    return UCk_Utils_2dGridSystem_UE::CastChecked(ParentHandle);
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_ParentGridPivot(
        const FCk_Handle_2dGridCell& InCell,
        ECk_LocalWorld InLocalWorld)
    -> FTransform
{
    const auto ParentHandle = Get_ParentGrid(InCell);
    return UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentHandle, InLocalWorld);
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_Index(
        const FCk_Handle_2dGridCell& InCell)
    -> int32
{
    return InCell.Get_Entity().Get_EntityNumber() - 1;
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_Coordinate(
        const FCk_Handle_2dGridCell& InCell,
        ECk_2dGridSystem_CoordinateType InCoordinateType)
    -> FIntPoint
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid"))
    { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);

    const auto& Dimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(ParentGrid);
    const auto EntityId = InCell.Get_Entity().Get_ID();
    const auto Index = static_cast<int32>(EntityId) - 1;
    const auto LocalCoord = UCk_Utils_Grid2D_UE::Get_IndexAsCoordinate(Index, Dimensions);

    // InCoordinateType is deliberately ignored: the parent entity transform carries all rotation,
    // so cell coordinates are always local.
    return LocalCoord;
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_Tags(
        const FCk_Handle_2dGridCell& InCell)
    -> FGameplayTagContainer
{
    return InCell.Get<ck::FFragment_2dGridCell_Params>().Get_Tags();
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_IsDisabled(
        const FCk_Handle_2dGridCell& InCell)
    -> bool
{
    return InCell.Has<ck::FTag_2dGridCell_Disabled>();
}

auto
    UCk_Utils_2dGridCell_UE::
    Request_EnableDisable(
        FCk_Handle_2dGridCell& InCell,
        ECk_EnableDisable InEnableDisable,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto IsCellValid = ck::IsValid(InCell);
    CK_ENSURE_IF_NOT(IsCellValid, TEXT("Cell is invalid"))
    {
        InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    switch (InEnableDisable)
    {
        case ECk_EnableDisable::Disable:
        {
            // Disabled is a COUNTED tag: each disabler holds one vote, so overlapping
            // disablers compose (two blockers on one cell -> count 2). Add increments
            // and never ensures for counted tags.
            InCell.Add<ck::FTag_2dGridCell_Disabled>();
            break;
        }
        case ECk_EnableDisable::Enable:
        {
            InCell.Try_Remove<ck::FTag_2dGridCell_Disabled>();
            break;
        }
        default:
            break;
    }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Succeeded);
}

auto
    UCk_Utils_2dGridCell_UE::
    Request_AddTag(
        FCk_Handle_2dGridCell& InCell,
        FGameplayTag InTag,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_2dGridCell
{
    const auto IsCellValid = ck::IsValid(InCell);
    CK_ENSURE_IF_NOT(IsCellValid, TEXT("Cell is invalid"))
    {
        InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InCell;
    }

    const auto IsTagValid = InTag.IsValid();
    CK_ENSURE_IF_NOT(IsTagValid, TEXT("Request_AddTag: tag is invalid"))
    {
        InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InCell;
    }

    auto& Params = InCell.Get<ck::FFragment_2dGridCell_Params>();
    Params.Get_Tags().AddTag(InTag);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Succeeded);

    return InCell;
}

auto
    UCk_Utils_2dGridCell_UE::
    Request_RemoveTag(
        FCk_Handle_2dGridCell& InCell,
        FGameplayTag InTag,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_2dGridCell
{
    const auto IsCellValid = ck::IsValid(InCell);
    CK_ENSURE_IF_NOT(IsCellValid, TEXT("Cell is invalid"))
    {
        InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InCell;
    }

    auto& Params = InCell.Get<ck::FFragment_2dGridCell_Params>();
    Params.Get_Tags().RemoveTag(InTag);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InCell, ECk_Request_OperationResult::Succeeded);

    return InCell;
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_Bounds(
        const FCk_Handle_2dGridCell& InCell,
        ECk_LocalWorld InLocalWorld)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid"))
    { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);
    const auto CellSize = UCk_Utils_2dGridSystem_UE::Get_CellSize(ParentGrid);
    const auto LocalCoord = Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

    const auto Min = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto Max = Min + CellSize;

    if (InLocalWorld == ECk_LocalWorld::Local)
    {
        return FBox2D(Min, Max);
    }

    const auto PivotWorldTransform = UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentGrid, ECk_LocalWorld::World);

    const auto Corners = TArray
    {
        Min,
        FVector2D(Max.X, Min.Y),
        Max,
        FVector2D(Min.X, Max.Y)
    };

    auto BoundsMin = FVector2D{FLT_MAX, FLT_MAX};
    auto BoundsMax = FVector2D{-FLT_MAX, -FLT_MAX};

    for (const auto& Corner : Corners)
    {
        const auto TransformedCorner = FVector2D(PivotWorldTransform.TransformPosition(FVector(Corner.X, Corner.Y, 0.0f)));

        BoundsMin.X = FMath::Min(BoundsMin.X, TransformedCorner.X);
        BoundsMin.Y = FMath::Min(BoundsMin.Y, TransformedCorner.Y);
        BoundsMax.X = FMath::Max(BoundsMax.X, TransformedCorner.X);
        BoundsMax.Y = FMath::Max(BoundsMax.Y, TransformedCorner.Y);
    }

    return FBox2D(BoundsMin, BoundsMax);
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_OrientedBounds2D(
        const FCk_Handle_2dGridCell& InCell,
        ECk_LocalWorld InLocalWorld)
    -> FCk_OrientedBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid"))
    { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);
    const auto CellSize = UCk_Utils_2dGridSystem_UE::Get_CellSize(ParentGrid);
    const auto LocalCoord = Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

    const auto LocalMin = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto LocalCenter = LocalMin + (CellSize * 0.5f);

    if (InLocalWorld == ECk_LocalWorld::Local)
    {
        return UCk_Utils_OrientedBox2D_UE::Request_Create(LocalCenter, CellSize * 0.5f);
    }

    const auto PivotWorldTransform = UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentGrid, ECk_LocalWorld::World);
    const auto WorldCenter = FVector2D(PivotWorldTransform.TransformPosition(FVector(LocalCenter.X, LocalCenter.Y, 0.0f)));
    const auto WorldRotation = PivotWorldTransform.GetRotation().Rotator().Yaw;

    return UCk_Utils_OrientedBox2D_UE::Request_CreateWithRotation(WorldCenter, FMath::DegreesToRadians(WorldRotation), CellSize * 0.5f);
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_OrientedBounds3D(
        const FCk_Handle_2dGridCell& InCell,
        ECk_LocalWorld InLocalWorld)
    -> FCk_OrientedBox3D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid"))
    { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);
    const auto CellSize = UCk_Utils_2dGridSystem_UE::Get_CellSize(ParentGrid);
    const auto LocalCoord = Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

    const auto LocalMin = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto LocalCenter = LocalMin + (CellSize * 0.5f);

    constexpr auto VisualizationZExtent = 1.0f;

    if (InLocalWorld == ECk_LocalWorld::Local)
    {
        const auto Center3D = FVector(LocalCenter.X, LocalCenter.Y, 0.0f);
        const auto Extents3D = FVector(CellSize.X * 0.5f, CellSize.Y * 0.5f, VisualizationZExtent);
        return UCk_Utils_OrientedBox3D_UE::Request_Create(Center3D, Extents3D);
    }

    const auto PivotWorldTransform = UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentGrid, ECk_LocalWorld::World);
    const auto WorldCenter3D = PivotWorldTransform.TransformPosition(FVector(LocalCenter.X, LocalCenter.Y, 0.0f));

    const auto Frame3D = UCk_Utils_Frame3D_UE::Request_CreateFromTransform(PivotWorldTransform);
    const auto Extents3D = FVector(CellSize.X * 0.5f, CellSize.Y * 0.5f, VisualizationZExtent);

    auto OrientedBox = UCk_Utils_OrientedBox3D_UE::Request_CreateWithFrame(Frame3D, Extents3D);

    auto BoxFrame = UCk_Utils_OrientedBox3D_UE::Get_Frame(OrientedBox);
    UCk_Utils_Frame3D_UE::Request_SetOrigin(BoxFrame, WorldCenter3D);
    UCk_Utils_OrientedBox3D_UE::Request_SetFrame(OrientedBox, BoxFrame);

    return OrientedBox;
}

// --------------------------------------------------------------------------------------------------------------------