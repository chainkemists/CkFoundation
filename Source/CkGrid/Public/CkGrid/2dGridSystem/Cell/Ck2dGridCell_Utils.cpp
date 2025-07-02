#include "Ck2dGridCell_Utils.h"

#include "Ck2dGridCell_Fragment.h"

#include "CkCore/Math/Geometry/CkGeometry_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

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
        const FCk_Handle_2dGridCell& InCell)
    -> FCk_Handle_SceneNode
{
    const auto ParentHandle = Get_ParentGrid(InCell);
    return UCk_Utils_2dGridSystem_UE::Get_Pivot(ParentHandle);
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
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid")) { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);
    CK_ENSURE_IF_NOT(ck::IsValid(ParentGrid), TEXT("Cell's parent grid is invalid")) { return {}; }

    // Get the base local coordinate
    const auto& Dimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(ParentGrid);
    const auto EntityId = InCell.Get_Entity().Get_ID();
    const auto Index = static_cast<int32>(EntityId) - 1;
    const auto LocalCoord = UCk_Utils_Grid2D_UE::Get_IndexAsCoordinate(Index, Dimensions);

    // For now, always return local coordinates
    // The Entity transform handles all visual rotation
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
    Get_WorldBounds(
        const FCk_Handle_2dGridCell& InCell,
        ECk_2dGridSystem_CoordinateType InCoordinateType)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid"))
    { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);
    const auto ParentGridPivot = UCk_Utils_Transform_UE::Cast(Get_ParentGridPivot(InCell));

    const auto WorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(ParentGridPivot);
    const auto CellSize = UCk_Utils_2dGridSystem_UE::Get_CellSize(ParentGrid);

    // Always use local coordinates - Entity transform does the rotation
    const auto LocalCoord = Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

    // Calculate local cell bounds
    const auto LocalMin = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(LocalCoord, CellSize);
    const auto LocalMax = LocalMin + CellSize;

    // Transform all 4 corners to handle rotation properly
    const auto Corners = TArray<FVector2D>{
        LocalMin,
        FVector2D(LocalMax.X, LocalMin.Y),
        LocalMax,
        FVector2D(LocalMin.X, LocalMax.Y)
    };

    auto WorldMin = FVector2D{FLT_MAX, FLT_MAX};
    auto WorldMax = FVector2D{-FLT_MAX, -FLT_MAX};

    for (const auto& Corner : Corners)
    {
        const auto WorldCorner = FVector2D(WorldTransform.TransformPosition(FVector(Corner.X, Corner.Y, 0.0f)));
        WorldMin.X = FMath::Min(WorldMin.X, WorldCorner.X);
        WorldMin.Y = FMath::Min(WorldMin.Y, WorldCorner.Y);
        WorldMax.X = FMath::Max(WorldMax.X, WorldCorner.X);
        WorldMax.Y = FMath::Max(WorldMax.Y, WorldCorner.Y);
    }

    return FBox2D(WorldMin, WorldMax);
}

// --------------------------------------------------------------------------------------------------------------------