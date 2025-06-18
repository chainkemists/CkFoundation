#include "Ck2dGridSystem_Utils.h"

#include "Ck2dGridSystem_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkGrid/CkGrid_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_2dGridSystem_ParamsData& InParams)
    -> FCk_Handle_2dGridSystem
{
    CK_ENSURE_IF_NOT(UCk_Utils_Grid2D_UE::Get_IsValidGridDimensions(InParams.Get_Dimensions()),
        TEXT("Cannot Create 2dGridSystem because Dimensions [{}] are invalid"), InParams.Get_Dimensions())
    { return {}; }

    CK_ENSURE_IF_NOT(InParams.Get_CellSize().X > 0.0f && InParams.Get_CellSize().Y > 0.0f,
        TEXT("Cannot Create 2dGridSystem because CellSize [{}] is invalid"), InParams.Get_CellSize())
    { return {}; }

    // Validate all active coordinates are within grid dimensions
    for (const auto& ActiveCoordinates = InParams.Get_ActiveCoordinates();
        const auto& Coordinate : ActiveCoordinates)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(InParams.Get_Dimensions(), Coordinate),
            TEXT("Cannot Create 2dGridSystem because ActiveCoordinate [{}] is invalid for grid dimensions [{}]"),
            Coordinate, InParams.Get_Dimensions())
        { return {}; }
    }

    InHandle.Add<ck::FFragment_2dGridSystem_Params>(InParams);
    InHandle.Add<ck::FFragment_2dGridSystem_Transform>();
    InHandle.Add<ck::FFragment_2dGridSystem_Current>();
    auto GridEntity = Cast(InHandle);

    const auto& Dimensions = InParams.Get_Dimensions();
    const auto& ActiveCoordinates = InParams.Get_ActiveCoordinates();

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Coordinate = FIntPoint(X, Y);

            // Determine if this cell should be disabled
            const auto EnabledState = ActiveCoordinates.Contains(Coordinate)
                ? ECk_EnableDisable::Enable
                : ECk_EnableDisable::Disable;

            auto Cell = UCk_Utils_2dGridCell_UE::Create(GridEntity, FCk_Fragment_2dGridCell_ParamsData{}, EnabledState);

            UCk_Utils_Handle_UE::Set_DebugName(Cell, *ck::Format_UE(TEXT("Cell_[{},{}]"), X, Y));
        }
    }

    UCk_Utils_Handle_UE::Set_DebugName(GridEntity, *ck::Format_UE(TEXT("2dGridSystem_[{}x{}]"), Dimensions.X, Dimensions.Y));

    return GridEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_2dGridSystem_UE, FCk_Handle_2dGridSystem,
    ck::FFragment_2dGridSystem_Params, ck::FFragment_2dGridSystem_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    Get_Dimensions(
        const FCk_Handle_2dGridSystem& InGrid)
    -> FIntPoint
{
    return InGrid.Get<ck::FFragment_2dGridSystem_Params>().Get_Dimensions();
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_CellSize(
        const FCk_Handle_2dGridSystem& InGrid)
    -> FVector2D
{
    return InGrid.Get<ck::FFragment_2dGridSystem_Params>().Get_CellSize();
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_Transform(
        const FCk_Handle_2dGridSystem& InGrid)
    -> FTransform
{
    return InGrid.Get<ck::FFragment_2dGridSystem_Transform>().Get_Transform();
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_ActiveCoordinates(
        const FCk_Handle_2dGridSystem& InGrid)
    -> TSet<FIntPoint>
{
    return InGrid.Get<ck::FFragment_2dGridSystem_Params>().Get_ActiveCoordinates();
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_CellAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate)
    -> FCk_Handle_2dGridCell
{
    const auto& Dimensions = Get_Dimensions(InGrid);

    CK_ENSURE_IF_NOT(UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(Dimensions, InCoordinate),
        TEXT("Invalid coordinate [{}] for grid [{}] with dimensions [{}]"), InCoordinate, InGrid, Dimensions)
    { return {}; }

    const auto Index = UCk_Utils_Grid2D_UE::Get_CoordinateAsIndex(InCoordinate, Dimensions);
    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    // Index + 1 because of our transient entity
    const auto Entity = FCk_Entity{static_cast<FCk_Entity::IdType>(Index + 1)};
    auto CellHandle = ck::StaticCast<FCk_Handle_2dGridCell>(ck::MakeHandle(Entity, InGrid));

    return CellHandle;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_AllCells(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter)
    -> TArray<FCk_Handle_2dGridCell>
{
    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();
    auto AllCells = TArray<FCk_Handle_2dGridCell>{};

    ForEach_Cell(InGrid, InCellFilter, [&](const FCk_Handle_2dGridCell& InGridCell)
    {
        AllCells.Add(InGridCell);
    });

    return AllCells;
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc)
    -> void
{
    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    switch (InCellFilter)
    {
        case ECk_2dGridSystem_CellFilter::OnlyActiveCells:
        {
            CellRegistry.View<ck::FFragment_2dGridCell_Params, ck::TExclude<ck::FTag_2dGridCell_Disabled>>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&)
            {
                auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);

                if (auto CellHandle = UCk_Utils_2dGridCell_UE::Cast(EntityHandle);
                    ck::IsValid(CellHandle))
                {
                    InFunc(CellHandle);
                }
            });
        }
        case ECk_2dGridSystem_CellFilter::OnlyDisabledCells:
        {
            CellRegistry.View<ck::FFragment_2dGridCell_Params, ck::FTag_2dGridCell_Disabled>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&)
            {
                auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);

                if (auto CellHandle = UCk_Utils_2dGridCell_UE::Cast(EntityHandle);
                    ck::IsValid(CellHandle))
                {
                    InFunc(CellHandle);
                }
            });
        }
        case ECk_2dGridSystem_CellFilter::NoFilter:
        {
            CellRegistry.View<ck::FFragment_2dGridCell_Params>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&)
            {
                auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);

                if (auto CellHandle = UCk_Utils_2dGridCell_UE::Cast(EntityHandle);
                    ck::IsValid(CellHandle))
                {
                    InFunc(CellHandle);
                }
            });
        }
        default:
        {
            CK_INVALID_ENUM(InCellFilter);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------