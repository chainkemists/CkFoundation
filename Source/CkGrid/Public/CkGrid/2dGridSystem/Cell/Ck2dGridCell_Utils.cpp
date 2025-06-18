#include "Ck2dGridCell_Utils.h"

#include "Ck2dGridCell_Fragment.h"
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
    Get_Index(
        const FCk_Handle_2dGridCell& InCell)
    -> int32
{
    return InCell.Get_Entity().Get_EntityNumber();
}

auto
    UCk_Utils_2dGridCell_UE::
    Get_Coordinate(
        const FCk_Handle_2dGridCell& InCell)
    -> FIntPoint
{
    const auto& ParentGrid = Get_ParentGrid(InCell);
    CK_ENSURE_IF_NOT(ck::IsValid(ParentGrid),
        TEXT("Cannot get coordinate for cell [{}] because parent grid is INVALID"), InCell)
    { return {}; }

    const auto Dimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(ParentGrid);
    const auto Index = Get_Index(InCell);

    return UCk_Utils_Grid2D_UE::Get_IndexAsCoordinate(Index, Dimensions);
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

// --------------------------------------------------------------------------------------------------------------------