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

    // Get the base local coordinate using existing logic
    const auto& Dimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(ParentGrid);
    const auto CellRegistry = ParentGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    // Get entity ID and convert back to coordinate
    const auto EntityId = InCell.Get_Entity().Get_ID();
    const auto Index = static_cast<int32>(EntityId) - 1; // Subtract 1 for transient entity offset
    const auto LocalCoord = UCk_Utils_Grid2D_UE::Get_IndexAsCoordinate(Index, Dimensions);

    if (InCoordinateType == ECk_2dGridSystem_CoordinateType::Local)
    {
        return LocalCoord;
    }

    // Apply coordinate remapping based on Entity transform rotation
    return UCk_Utils_2dGridSystem_UE::Get_CoordinateRemappedForTransform(ParentGrid, LocalCoord);
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
    CK_ENSURE_IF_NOT(ck::IsValid(InCell), TEXT("Cell is invalid")) { return {}; }

    const auto ParentGrid = Get_ParentGrid(InCell);
    CK_ENSURE_IF_NOT(ck::IsValid(ParentGrid), TEXT("Cell's parent grid is invalid")) { return {}; }

    // Cast to transform handle to get world transform
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(ParentGrid);
    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle),
        TEXT("Grid [{}] does not have a Transform component"), ParentGrid) { return {}; }

    const auto WorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);
    const auto CellSize = UCk_Utils_2dGridSystem_UE::Get_CellSize(ParentGrid);
    const auto Coordinate = Get_Coordinate(InCell, InCoordinateType);

    // Calculate local cell bounds using geometry utils
    const auto LocalMin = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(Coordinate, CellSize);
    const auto LocalMax = LocalMin + CellSize;

    // Transform to world space (ignoring Z)
    const auto WorldMin = FVector2D(WorldTransform.TransformPosition(FVector(LocalMin.X, LocalMin.Y, 0.0f)));
    const auto WorldMax = FVector2D(WorldTransform.TransformPosition(FVector(LocalMax.X, LocalMax.Y, 0.0f)));

    return UCk_Utils_Geometry2D_UE::Make_Box_FromPoints({WorldMin, WorldMax});
}

// --------------------------------------------------------------------------------------------------------------------