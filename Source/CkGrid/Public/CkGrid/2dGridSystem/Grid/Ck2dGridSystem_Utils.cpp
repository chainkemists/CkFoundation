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
    Create(
        const FCk_Handle& InOwner,
        const FCk_Fragment_2dGridSystem_ParamsData& InParams)
    -> FCk_Handle_2dGridSystem
{
    CK_ENSURE_IF_NOT(InParams.Get_Dimensions().X > 0 && InParams.Get_Dimensions().Y > 0,
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

    auto GridEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner, [&](FCk_Handle InNewGrid)
    {
        // Add grid parameters
        InNewGrid.Add<ck::FFragment_2dGridSystem_Params>(InParams);

        // Add grid transform (default to identity)
        InNewGrid.Add<ck::FFragment_2dGridSystem_Transform>();

        // Initialize grid current data with a new registry
        auto CellRegistry = FCk_Registry{};

        const auto& Dimensions = InParams.Get_Dimensions();
        const auto& ActiveCoordinates = InParams.Get_ActiveCoordinates();

        // Create all cell entities in the cell registry
        for (int32 Y = 0; Y < Dimensions.Y; ++Y)
        {
            for (int32 X = 0; X < Dimensions.X; ++X)
            {
                const auto Coordinate = FIntPoint(X, Y);

                // Create cell entity as child of grid in the cell registry
                auto CellEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(CellRegistry, [&](FCk_Handle InNewCell)
                {
                    // Setup entity with grid as lifetime owner
                    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(InNewCell, InNewGrid);

                    // Add cell parameters (just tags)
                    const auto CellParams = FCk_Fragment_2dGridCell_ParamsData(FGameplayTagContainer{});

                    // Determine if this cell should be disabled
                    const auto EnabledState = (NOT ActiveCoordinates.IsEmpty() && NOT ActiveCoordinates.Contains(Coordinate))
                        ? ECk_EnableDisable::Disable
                        : ECk_EnableDisable::Enable;

                    auto Cell = UCk_Utils_2dGridCell_UE::Add(InNewCell, CellParams, EnabledState);

                    UCk_Utils_Handle_UE::Set_DebugName(InNewCell, *ck::Format_UE(TEXT("Cell_{}_{}"), X, Y));
                });
            }
        }

        // Add grid current data with the cell registry
        InNewGrid.Add<ck::FFragment_2dGridSystem_Current>(CellRegistry);

        UCk_Utils_Handle_UE::Set_DebugName(InNewGrid,
            *ck::Format_UE(TEXT("2dGridSystem_{}x{}"), Dimensions.X, Dimensions.Y));
    });

    return Cast(GridEntity);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Create_Transient(
        const FCk_Fragment_2dGridSystem_ParamsData& InParams,
        const UObject* InWorldContextObject)
    -> FCk_Handle_2dGridSystem
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject),
        TEXT("WorldContextObject [{}] is INVALID. Unable to Create a Transient 2dGridSystem Entity"), InWorldContextObject)
    { return {}; }

    auto TransientOwner = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity_FromContextObject(InWorldContextObject);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(TransientOwner, TEXT("(Transient) GridSystem Owner"));
#endif

    return Create(TransientOwner, InParams);
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
    Set_Transform(
        FCk_Handle_2dGridSystem& InGrid,
        const FTransform& InTransform)
    -> FCk_Handle_2dGridSystem
{
    InGrid.Get<ck::FFragment_2dGridSystem_Transform>().Set_Transform(InTransform);
    return InGrid;
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
        TEXT("Invalid coordinate [{}] for grid with dimensions [{}]"), InCoordinate, Dimensions)
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
        const FCk_Handle_2dGridSystem& InGrid)
    -> TArray<FCk_Handle_2dGridCell>
{
    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    auto AllCells = TArray<FCk_Handle_2dGridCell>{};
    CellRegistry.View<ck::FFragment_2dGridCell_Params>().ForEach(
    [&](FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&)
    {
        auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);
        AllCells.Add(UCk_Utils_2dGridCell_UE::Cast(EntityHandle));
    });

    return AllCells;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_ActiveCells(
        const FCk_Handle_2dGridSystem& InGrid)
    -> TArray<FCk_Handle_2dGridCell>
{
    const auto& AllCells = Get_AllCells(InGrid);

    return ck::algo::Filter(AllCells, [](const FCk_Handle_2dGridCell& InCell)
    {
        return NOT UCk_Utils_2dGridCell_UE::Get_IsDisabled(InCell);
    });
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_InactiveCells(
        const FCk_Handle_2dGridSystem& InGrid)
    -> TArray<FCk_Handle_2dGridCell>
{
    const auto& AllCells = Get_AllCells(InGrid);

    return ck::algo::Filter(AllCells, [](const FCk_Handle_2dGridCell& InCell)
    {
        return UCk_Utils_2dGridCell_UE::Get_IsDisabled(InCell);
    });
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_2dGridCell>
{
    auto Cells = TArray<FCk_Handle_2dGridCell>{};

    ForEach_Cell(InGrid, [&](FCk_Handle_2dGridCell InCell)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InCell, InOptionalPayload); }
        else
        { Cells.Emplace(InCell); }
    });

    return Cells;
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_ActiveCell(
        const FCk_Handle_2dGridSystem& InGrid,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_2dGridCell>
{
    auto Cells = TArray<FCk_Handle_2dGridCell>{};

    ForEach_ActiveCell(InGrid, [&](FCk_Handle_2dGridCell InCell)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InCell, InOptionalPayload); }
        else
        { Cells.Emplace(InCell); }
    });

    return Cells;
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc)
    -> void
{
    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    CellRegistry.View<ck::FFragment_2dGridCell_Params>().ForEach(
    [&](FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&)
    {
        auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);
        auto CellHandle = UCk_Utils_2dGridCell_UE::Cast(EntityHandle);

        if (ck::IsValid(CellHandle))
        {
            InFunc(CellHandle);
        }
    });
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_ActiveCell(
        const FCk_Handle_2dGridSystem& InGrid,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc)
    -> void
{
    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    CellRegistry.View<ck::FFragment_2dGridCell_Params>().ForEach(
    [&](const FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&)
    {
        auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);

        if (auto CellHandle = UCk_Utils_2dGridCell_UE::Cast(EntityHandle);
            ck::IsValid(CellHandle) && NOT UCk_Utils_2dGridCell_UE::Get_IsDisabled(CellHandle))
        {
            InFunc(CellHandle);
        }
    });
}

// --------------------------------------------------------------------------------------------------------------------