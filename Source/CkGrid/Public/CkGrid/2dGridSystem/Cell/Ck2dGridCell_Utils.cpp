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

    // For now, always return local coordinates
    // The Parent Entity transform handles all visual rotation
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

    // Calculate local cell bounds (relative to grid origin)
    const auto Min = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto Max = Min + CellSize;

    if (InLocalWorld == ECk_LocalWorld::Local)
    {
        // For local space, just return the bounds relative to grid origin
        return FBox2D(Min, Max);
    }

    // For world space, use the pivot's world transform
    const auto PivotWorldTransform = UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentGrid, ECk_LocalWorld::World);

    // Transform all 4 corners to handle rotation properly
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

    // Calculate local cell center
    const auto LocalMin = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto LocalCenter = LocalMin + (CellSize * 0.5f);

    if (InLocalWorld == ECk_LocalWorld::Local)
    {
        return UCk_Utils_OrientedBox2D_UE::Request_Create(LocalCenter, CellSize * 0.5f);
    }

    // For world space, use the pivot's world transform
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

    // Calculate local cell center
    const auto LocalMin = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto LocalCenter = LocalMin + (CellSize * 0.5f);

    if (InLocalWorld == ECk_LocalWorld::Local)
    {
        const auto Center3D = FVector(LocalCenter.X, LocalCenter.Y, 0.0f);
        const auto Extents3D = FVector(CellSize.X * 0.5f, CellSize.Y * 0.5f, 1.0f); // Small Z extent for visualization
        return UCk_Utils_OrientedBox3D_UE::Request_Create(Center3D, Extents3D);
    }

    // For world space, use the pivot's world transform
    const auto PivotWorldTransform = UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentGrid, ECk_LocalWorld::World);
    const auto WorldCenter3D = PivotWorldTransform.TransformPosition(FVector(LocalCenter.X, LocalCenter.Y, 0.0f));

    // Create frame from pivot transform
    const auto Frame3D = UCk_Utils_Frame3D_UE::Request_CreateFromTransform(PivotWorldTransform);
    const auto Extents3D = FVector(CellSize.X * 0.5f, CellSize.Y * 0.5f, 1.0f); // Small Z extent for visualization

    auto OrientedBox = UCk_Utils_OrientedBox3D_UE::Request_CreateWithFrame(Frame3D, Extents3D);

    // Update the center to the actual cell world position
    auto BoxFrame = UCk_Utils_OrientedBox3D_UE::Get_Frame(OrientedBox);
    UCk_Utils_Frame3D_UE::Request_SetOrigin(BoxFrame, WorldCenter3D);
    UCk_Utils_OrientedBox3D_UE::Request_SetFrame(OrientedBox, BoxFrame);

    return OrientedBox;
}

// --------------------------------------------------------------------------------------------------------------------