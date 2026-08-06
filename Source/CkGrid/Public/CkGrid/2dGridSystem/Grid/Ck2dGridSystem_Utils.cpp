#include "Ck2dGridSystem_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Math/Geometry/CkGeometry_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGrid/CkGrid_Log.h"
#include "CkGrid/CkGrid_Stats.h"
#include "CkGrid/CkGrid_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Grid::Intersections"), STAT_Grid_Intersections, STATGROUP_CkGrid);
DECLARE_DWORD_COUNTER_STAT(TEXT("Grid Cell Pairs Tested"), STAT_Grid_CellPairsTested, STATGROUP_CkGrid);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    DoCompose_PivotCurrentAndCells(
        FCk_Handle_Transform& InHandle)
    -> FCk_Handle_2dGridSystem
{
    const auto InParams = InHandle.Get<ck::FFragment_2dGridSystem_Params>();

    auto PivotSceneNode = UCk_Utils_SceneNode_UE::Create(InHandle, InParams.Get_Pivot());
    UCk_Utils_Handle_UE::Set_DebugName(PivotSceneNode, TEXT("GridPivot"));
    InHandle.Add<ck::FFragment_2dGridSystem_Current>(PivotSceneNode);
    auto GridEntity = UCk_Utils_2dGridSystem_UE::Cast(InHandle);

    const auto& Dimensions = InParams.Get_Dimensions();

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Coordinate = FIntPoint(X, Y);

            const auto EnabledState = InParams.Get_IsCoordinateActive(Coordinate)
                ? ECk_EnableDisable::Enable
                : ECk_EnableDisable::Disable;

            auto Cell = UCk_Utils_2dGridCell_UE::Create(GridEntity, FCk_2dGridCell_Spec{}, EnabledState);
            UCk_Utils_Handle_UE::Set_DebugName(Cell, *ck::Format_UE(TEXT("Cell_[{},{}]"), X, Y));
        }
    }

    UCk_Utils_Handle_UE::Set_DebugName(GridEntity, *ck::Format_UE(TEXT("2dGridSystem_[{}x{}]"), Dimensions.X, Dimensions.Y));

    return GridEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_2dGridSystem_Spec& InParams)
    -> FCk_Handle_2dGridSystem
{
    for (const auto& ResolvedActiveCoords = InParams.Get_ResolvedActiveCoordinates();
        const auto& Coordinate : ResolvedActiveCoords)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(InParams.Get_Dimensions(), Coordinate),
            TEXT("Cannot Create 2dGridSystem because ActiveCoordinate [{}] is invalid for grid dimensions [{}]"),
            Coordinate, InParams.Get_Dimensions())
        { return {}; }
    }

    InHandle.Add<ck::FFragment_2dGridSystem_Params>(InParams);

    return DoCompose_PivotCurrentAndCells(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    Create(
        FCk_Handle& InOwner,
        const FTransform& InInitialTransform,
        const FCk_2dGridSystem_Spec& InParams)
    -> FCk_Handle_2dGridSystem
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    auto ChildTransform = UCk_Utils_Transform_UE::Add(NewEntity, InInitialTransform, ECk_Replication::DoesNotReplicate);
    return Add(ChildTransform, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    Request_RecomposeFromSnapshot(
        FCk_Handle_Transform& InHandle)
    -> FCk_Handle_2dGridSystem
{
    CK_ENSURE_IF_NOT(InHandle.Has<ck::FFragment_2dGridSystem_Params>(),
        TEXT("Request_RecomposeFromSnapshot on [{}]: no restored 2dGridSystem Params"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InHandle.Has<ck::FFragment_2dGridSystem_Current>(),
        TEXT("Request_RecomposeFromSnapshot on [{}]: grid already composed"), InHandle)
    { return {}; }

    return DoCompose_PivotCurrentAndCells(InHandle);
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
    Get_Pivot(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_LocalWorld InLocalWorld)
    -> FTransform
{
    const auto& Pivot = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_Pivot();
    switch(InLocalWorld)
    {
        case ECk_LocalWorld::Local:
        {
            return UCk_Utils_SceneNode_UE::Get_Offset(Pivot);
        }
        case ECk_LocalWorld::World:
        {
            return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Pivot);
        }
    }

    return {};
}

auto
    UCk_Utils_2dGridSystem_UE::
    Request_UpdatePivot(
        const FCk_Handle_2dGridSystem& InGrid,
        FVector InLocationOffset,
        FRotator InRotationOffset,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto IsGridValid = ck::IsValid(InGrid);
    CK_ENSURE_IF_NOT(IsGridValid, TEXT("Cannot update pivot for invalid grid"))
    {}
    if (NOT IsGridValid)
    {
        InDelegate.ExecuteIfBound(FCk_Handle{InGrid}, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    auto Pivot = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_Pivot();
    UCk_Utils_SceneNode_UE::Request_UpdateOffset(Pivot,
        FCk_Request_SceneNode_UpdateRelativeTransform{FTransform{InRotationOffset, InLocationOffset}}, {});

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(FCk_Handle{InGrid}, ECk_Request_OperationResult::Succeeded);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_PivotTransformForAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_PivotAnchor InAnchor)
    -> FTransform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot calculate pivot transform for invalid grid"))
    { return FTransform::Identity; }

    const auto Dimensions = Get_Dimensions(InGrid);
    const auto CellSize = Get_CellSize(InGrid);
    const auto GridSize = FVector2D(Dimensions.X * CellSize.X, Dimensions.Y * CellSize.Y);

    auto PivotOffset = FVector2D::ZeroVector;

    switch (InAnchor)
    {
        case ECk_2dGridSystem_PivotAnchor::Center:
        {
            PivotOffset = GridSize * 0.5f;
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::BottomLeft:
        {
            PivotOffset = FVector2D::ZeroVector;
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::BottomCenter:
        {
            PivotOffset = FVector2D(GridSize.X * 0.5f, 0.0f);
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::BottomRight:
        {
            PivotOffset = FVector2D(GridSize.X, 0.0f);
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::MiddleLeft:
        {
            PivotOffset = FVector2D(0.0f, GridSize.Y * 0.5f);
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::MiddleRight:
        {
            PivotOffset = FVector2D(GridSize.X, GridSize.Y * 0.5f);
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::TopLeft:
        {
            PivotOffset = FVector2D(0.0f, GridSize.Y);
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::TopCenter:
        {
            PivotOffset = FVector2D(GridSize.X * 0.5f, GridSize.Y);
            break;
        }
        case ECk_2dGridSystem_PivotAnchor::TopRight:
        {
            PivotOffset = GridSize;
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InAnchor);
            break;
        }
    }

    auto Pivot = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_Pivot();
    auto NewTransform = UCk_Utils_SceneNode_UE::Get_Offset(Pivot);

    NewTransform.SetLocation(FVector(-PivotOffset.X, -PivotOffset.Y, NewTransform.GetLocation().Z));

    return NewTransform;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Request_SetPivotToAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_PivotAnchor InAnchor,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FTransform
{
    const auto& NewPivotTransform = Get_PivotTransformForAnchor(InGrid, InAnchor);
    Request_UpdatePivot(InGrid, NewPivotTransform.GetLocation(), NewPivotTransform.GetRotation().Rotator(), {});

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(FCk_Handle{InGrid}, ECk_Request_OperationResult::Succeeded);

    return NewPivotTransform;
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
    Get_CellAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate)
    -> FCk_Handle_2dGridCell
{
    const auto& Dimensions = Get_Dimensions(InGrid);

    // Out-of-bounds is a valid query RESULT, not an error: callers deliberately probe coordinates
    // outside the grid (e.g. a footprint straddling an edge) and rely on an invalid handle back.
    if (NOT UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(Dimensions, InCoordinate))
    { return {}; }

    const auto Index = UCk_Utils_Grid2D_UE::Get_CoordinateAsIndex(InCoordinate, Dimensions);
    const auto& CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    constexpr auto TransientEntityOffset = 1;
    const auto Entity = FCk_Entity{static_cast<FCk_Entity::IdType>(Index + TransientEntityOffset)};
    auto CellHandle = ck::MakeHandle(Entity, CellRegistry);

    return UCk_Utils_2dGridCell_UE::Cast(CellHandle);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_AllCells(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter)
    -> TArray<FCk_Handle_2dGridCell>
{
    auto AllCells = TArray<FCk_Handle_2dGridCell>{};

    ForEach_Cell(InGrid, InCellFilter, [&](const FCk_Handle_2dGridCell& InGridCell)
    {
        AllCells.Add(InGridCell);
    });

    return AllCells;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_AllCells_AsCoordinate(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter)
    -> TArray<FIntPoint>
{
    auto AllCellCoordinates = TArray<FIntPoint>{};

    if (InCellFilter == ECk_2dGridSystem_CellFilter::NoFilter)
    {
        const auto& Dimensions = Get_Dimensions(InGrid);
        AllCellCoordinates.Reserve(Dimensions.X * Dimensions.Y);
    }

    ForEach_Cell(InGrid, InCellFilter, [&](const FCk_Handle_2dGridCell& InGridCell)
    {
        AllCellCoordinates.Add(UCk_Utils_2dGridCell_UE::Get_Coordinate(InGridCell, ECk_2dGridSystem_CoordinateType::Local));
    });

    return AllCellCoordinates;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_Footprint(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter)
    -> FIntPoint
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot get cell bounds for invalid grid"))
    { return {}; }

    auto MinCoord = FIntPoint{INT_MAX, INT_MAX};
    auto MaxCoord = FIntPoint{INT_MIN, INT_MIN};

    ForEach_Cell(InGrid, InCellFilter, [&](const FCk_Handle_2dGridCell& Cell)
    {
        const auto Coord = UCk_Utils_2dGridCell_UE::Get_Coordinate(Cell, ECk_2dGridSystem_CoordinateType::Local);
        MinCoord.X = FMath::Min(MinCoord.X, Coord.X);
        MinCoord.Y = FMath::Min(MinCoord.Y, Coord.Y);
        MaxCoord.X = FMath::Max(MaxCoord.X, Coord.X);
        MaxCoord.Y = FMath::Max(MaxCoord.Y, Coord.Y);
    });

    constexpr auto InclusiveSpan = 1;
    return FIntPoint(MaxCoord.X - MinCoord.X + InclusiveSpan, MaxCoord.Y - MinCoord.Y + InclusiveSpan);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_IntersectsWith(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGridA), TEXT("GridA is invalid"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InGridB), TEXT("GridB is invalid"))
    { return {}; }

    const auto& BoundsA_NoFilter = Get_Bounds(InGridA, ECk_LocalWorld::World, ECk_2dGridSystem_CellFilter::NoFilter);
    const auto& BoundsB_NoFilter = Get_Bounds(InGridB, ECk_LocalWorld::World, ECk_2dGridSystem_CellFilter::NoFilter);

    if (NOT BoundsA_NoFilter.Intersect(BoundsB_NoFilter))
    { return false; }

    const auto BoundsA = InFilterA == ECk_2dGridSystem_CellFilter::NoFilter ? BoundsA_NoFilter : Get_Bounds(InGridA, ECk_LocalWorld::World, InFilterA);
    const auto BoundsB = InFilterB == ECk_2dGridSystem_CellFilter::NoFilter ? BoundsB_NoFilter : Get_Bounds(InGridB, ECk_LocalWorld::World, InFilterB);

    return BoundsA.Intersect(BoundsB);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_IsAlignedWith(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        float InAlignmentTolerance)
    -> bool
{
    const auto& PivotA = Get_Pivot(InGridA, ECk_LocalWorld::World);
    const auto& PivotB = Get_Pivot(InGridB, ECk_LocalWorld::World);

    const auto& RotationA = PivotA.GetRotation().Rotator();
    const auto& RotationB = PivotB.GetRotation().Rotator();

    const auto RotationDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(RotationA.Yaw, RotationB.Yaw));

    const auto RemainderFrom90 = FMath::Fmod(RotationDiff, 90.0f);

    const auto IsRotationallyAligned =
        RemainderFrom90 <= InAlignmentTolerance ||
        RemainderFrom90 >= (90.0f - InAlignmentTolerance);

    return IsRotationallyAligned;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_Intersections(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB,
        float InCellOverlapThreshold0to1)
    -> FCk_GridIntersectionResult
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGridA), TEXT("GridA is invalid"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InGridB), TEXT("GridB is invalid"))
    { return {}; }

    CK_ENSURE_IF_NOT(InCellOverlapThreshold0to1 >= 0.0f && InCellOverlapThreshold0to1 <= 1.0f,
        TEXT("TolerancePercent [{}] must be between 0.0 and 1.0"), InCellOverlapThreshold0to1)
    { return {}; }

    if (NOT Get_IntersectsWith(InGridA, InGridB, InFilterA, InFilterB))
    { return {}; }

    if (Get_IsAlignedWith(InGridA, InGridB))
    {
        const auto& CellSizeA = Get_CellSize(InGridA);
        const auto& CellSizeB = Get_CellSize(InGridB);

        if (CellSizeA.Equals(CellSizeB))
        {
            return DoGet_Intersections_Aligned(InGridA, InGridB, InFilterA, InFilterB, InCellOverlapThreshold0to1);
        }
    }

    return DoGet_Intersections(InGridA, InGridB, InFilterA, InFilterB, InCellOverlapThreshold0to1);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_IntersectingCells(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        float InTolerancePercent)
    -> TArray<FCk_GridCellIntersection>
{
    const auto Result = Get_Intersections(
        InGridA,
        InGridB,
        ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        InTolerancePercent);

    return Result.Get_IntersectingCells();
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_PositionForAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InDesiredAnchorCoordinate)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot calculate anchor position for invalid grid"))
    { return FVector::ZeroVector; }

    const auto& Dimensions = Get_Dimensions(InGrid);
    CK_ENSURE_IF_NOT(UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(Dimensions, InDesiredAnchorCoordinate),
        TEXT("Anchor coordinate [{}] is invalid for grid dimensions [{}]"), InDesiredAnchorCoordinate, Dimensions)
    { return FVector::ZeroVector; }

    const auto CellSize = Get_CellSize(InGrid);

    const auto AnchorLocalPos = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(InDesiredAnchorCoordinate, CellSize);
    const auto AnchorCenterPos = AnchorLocalPos + (CellSize * 0.5f);

    // Negated: this offset positions the grid so the anchor becomes the rotation origin.
    return FVector(-AnchorCenterPos.X, -AnchorCenterPos.Y, 0.0f);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_Bounds(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_LocalWorld InLocalOrWorld,
        ECk_2dGridSystem_CellFilter InCellFilter)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot get bounds for invalid grid"))
    { return {}; }

    if (InCellFilter == ECk_2dGridSystem_CellFilter::NoFilter)
    {
        const auto Dimensions = Get_Dimensions(InGrid);
        const auto CellSize = Get_CellSize(InGrid);
        const auto GridSize = FVector2D(Dimensions.X * CellSize.X, Dimensions.Y * CellSize.Y);

        if (InLocalOrWorld == ECk_LocalWorld::Local)
        {
            return FBox2D(FVector2D::ZeroVector, GridSize);
        }

        const auto PivotWorldTransform = Get_Pivot(InGrid, ECk_LocalWorld::World);

        const auto Corners = TArray{
            FVector2D::ZeroVector,
            FVector2D(GridSize.X, 0.0f),
            GridSize,
            FVector2D(0.0f, GridSize.Y)
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

    auto TotalBounds = FBox2D{};
    auto FirstCell = true;

    ForEach_Cell(InGrid, InCellFilter, [&](const FCk_Handle_2dGridCell& Cell)
    {
        const auto CellBounds = UCk_Utils_2dGridCell_UE::Get_Bounds(Cell, InLocalOrWorld);

        if (FirstCell)
        {
            TotalBounds = CellBounds;
            FirstCell = false;
        }
        else
        {
            TotalBounds = TotalBounds + CellBounds;
        }
    });

    CK_ENSURE_IF_NOT(NOT FirstCell, TEXT("Grid has no cells matching filter [{}]"), InCellFilter)
    { return {}; }

    return TotalBounds;
}

auto
    UCk_Utils_2dGridSystem_UE::
    DebugDraw_Grid(
        const UObject* InWorldContextObject,
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_2dGridSystem_DebugDraw_Options& InOptions)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot debug draw invalid grid"))
    { return; }

    const auto& Options = InOptions;
    const auto CellVisualization = Options.Get_CellVisualization();

    if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::None &&
        NOT Options.Get_ShowCoordinates() &&
        NOT Options.Get_ShowPivot() &&
        NOT Options.Get_ShowCellSizeInfo())
    { return; }

    const auto PivotTransform = Get_Pivot(InGrid, ECk_LocalWorld::World);

    const auto CellSize = Get_CellSize(InGrid);
    const auto Dimensions = Get_Dimensions(InGrid);

    if (CellVisualization != ECk_2dGridSystem_DebugDraw_CellVisualization::None)
    {
        ForEach_Cell(InGrid, ECk_2dGridSystem_CellFilter::NoFilter, [&](const FCk_Handle_2dGridCell& InCell)
        {
            const auto IsDisabled = UCk_Utils_2dGridCell_UE::Get_IsDisabled(InCell);
            const auto CellColor = IsDisabled ? Options.Get_DisabledCellColor() : Options.Get_EnabledCellColor();

            if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::AABB)
            {
                const auto CellBounds = UCk_Utils_2dGridCell_UE::Get_Bounds(InCell, ECk_LocalWorld::World);
                const auto CellCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellBounds);
                const auto CellExtent = UCk_Utils_Geometry2D_UE::Get_Box_Size(CellBounds) * 0.5f;

                const auto CellCenter3D = FVector(CellCenter.X, CellCenter.Y, PivotTransform.GetLocation().Z);
                const auto CellExtent3D = FVector(CellExtent.X, CellExtent.Y, 0.0f);

                UCk_Utils_DebugDraw_UE::DrawDebugWireframeBox(
                    InWorldContextObject,
                    CellCenter3D,
                    CellExtent3D,
                    PivotTransform.GetRotation(),
                    CellColor,
                    Options.Get_Duration(),
                    Options.Get_CellThickness());
            }
            else if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::OBB)
            {
                const auto OrientedCellBounds = UCk_Utils_2dGridCell_UE::Get_OrientedBounds3D(InCell, ECk_LocalWorld::World);

                UCk_Utils_OrientedBox3D_UE::DebugDraw_OrientedBox3D(
                    InWorldContextObject,
                    OrientedCellBounds,
                    CellColor,
                    Options.Get_Duration(),
                    Options.Get_CellThickness());
            }

            if (Options.Get_ShowCoordinates())
            {
                const auto Coord = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);
                const auto CoordText = ck::Format_UE(TEXT("[{}]"), Coord);

                FVector TextLocation;
                if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::OBB)
                {
                    const auto OrientedBounds = UCk_Utils_2dGridCell_UE::Get_OrientedBounds3D(InCell, ECk_LocalWorld::World);
                    TextLocation = UCk_Utils_OrientedBox3D_UE::Get_Center(OrientedBounds) + FVector(0.0f, 0.0f, 10.0f);
                }
                else
                {
                    const auto CellBounds = UCk_Utils_2dGridCell_UE::Get_Bounds(InCell, ECk_LocalWorld::World);
                    const auto CellCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellBounds);
                    TextLocation = FVector(CellCenter.X, CellCenter.Y, PivotTransform.GetLocation().Z) + FVector(0.0f, 0.0f, 10.0f);
                }

                UCk_Utils_DebugDraw_UE::DrawDebugString(
                    InWorldContextObject,
                    TextLocation,
                    CoordText,
                    Options.Get_TextColor(),
                    Options.Get_Duration());
            }
        });
    }

    const auto& GridTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InGrid);

    constexpr auto AxisLength = 100.0f;
    constexpr auto AxisThickness = 10.0f;
    constexpr auto DrawAxisCones = true;
    constexpr auto ConeSize = 50.0f;
    UCk_Utils_DebugDraw_UE::DrawDebugTransformGizmo(
        InWorldContextObject,
        GridTransform,
        AxisLength,
        AxisThickness,
        DrawAxisCones,
        ConeSize,
        Options.Get_Duration());

    if (Options.Get_ShowPivot())
    {
        const auto PivotLocation = PivotTransform.GetLocation();

        UCk_Utils_DebugDraw_UE::DrawDebugCross(
            InWorldContextObject,
            PivotLocation,
            Options.Get_PivotSize(),
            Options.Get_PivotColor(),
            Options.Get_Duration(),
            Options.Get_CellThickness());
    }

    if (Options.Get_ShowCellSizeInfo())
    {
        const auto GridCenterLocal = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(
            FIntPoint(Dimensions.X / 2, Dimensions.Y / 2), CellSize);
        const auto GridCenter3D = PivotTransform.TransformPosition(
            FVector(GridCenterLocal.X, GridCenterLocal.Y, 0.0f));

        const auto SizeText = ck::Format_UE(TEXT("Grid: {}x{}\nCell: {:.1f}x{:.1f}"),
            Dimensions.X, Dimensions.Y, CellSize.X, CellSize.Y);
        const auto TextLocation = GridCenter3D + FVector(0.0f, 0.0f, 30.0f);

        UCk_Utils_DebugDraw_UE::DrawDebugString(
            InWorldContextObject,
            TextLocation,
            SizeText,
            Options.Get_TextColor(),
            Options.Get_Duration());
    }
}

auto
    UCk_Utils_2dGridSystem_UE::
    DebugDraw_Grid_Simple(
        const UObject* InWorldContextObject,
        const FCk_Handle_2dGridSystem& InGrid,
        float InDuration)
    -> void
{
    auto Options = FCk_2dGridSystem_DebugDraw_Options{};
    Options.Set_Duration(InDuration);

    DebugDraw_Grid(InWorldContextObject, InGrid, Options);
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter    InCellFilter)
    -> TArray<FCk_Handle_2dGridCell>
{
    auto Ret = TArray<FCk_Handle_2dGridCell>{};

    ForEach_Cell(InGrid, InCellFilter, [&](FCk_Handle_2dGridCell InCell)
    {
        Ret.Emplace(InCell);
    });

    return Ret;
}

auto
    UCk_Utils_2dGridSystem_UE::
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc)
    -> void
{
    const auto IsGridValid = ck::IsValid(InGrid);
    CK_ENSURE_IF_NOT(IsGridValid, TEXT("ForEach_Cell: grid handle is invalid"))
    { }

    if (NOT IsGridValid)
    { return; }

    auto CellRegistry = InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_CellRegistry();

    const auto IsCellRegistryValid = ck::IsValid(CellRegistry);
    CK_ENSURE_IF_NOT(IsCellRegistryValid, TEXT("ForEach_Cell: grid cell registry is invalid"))
    { }

    if (NOT IsCellRegistryValid)
    { return; }

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
            break;
        }
        case ECk_2dGridSystem_CellFilter::OnlyDisabledCells:
        {
            CellRegistry.View<ck::FFragment_2dGridCell_Params, ck::FTag_2dGridCell_Disabled>().ForEach(
            [&](FCk_Entity InEntity, const ck::FFragment_2dGridCell_Params&, const ck::FTag_2dGridCell_Disabled&)
            {
                auto EntityHandle = ck::MakeHandle(InEntity, CellRegistry);

                if (auto CellHandle = UCk_Utils_2dGridCell_UE::Cast(EntityHandle);
                    ck::IsValid(CellHandle))
                {
                    InFunc(CellHandle);
                }
            });
            break;
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
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InCellFilter);
            break;
        }
    }
}

// ---- Shape placement queries ----

auto
    UCk_Utils_2dGridSystem_UE::
    Get_CanFitShapeAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const TArray<FIntPoint>& InShapeOffsets,
        const FIntPoint& InPosition)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid),
        TEXT("Cannot check shape fit for invalid grid"))
    { return false; }

    const auto Dimensions = Get_Dimensions(InGrid);

    return ck::algo::AllOf(InShapeOffsets, [&](const FIntPoint& Offset)
    {
        const auto Coord = InPosition + Offset;

        if (NOT UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(Dimensions, Coord))
        { return false; }

        auto CellHandle = Get_CellAt(InGrid, Coord);

        return ck::IsValid(CellHandle) && NOT UCk_Utils_2dGridCell_UE::Get_IsDisabled(CellHandle);
    });
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_FirstAvailablePositionForShape(
        const FCk_Handle_2dGridSystem& InGrid,
        const TArray<FIntPoint>& InShapeOffsets)
    -> FIntPoint
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid),
        TEXT("Cannot find available position for shape in invalid grid"))
    { return FIntPoint{-1, -1}; }

    const auto Dimensions = Get_Dimensions(InGrid);

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Position = FIntPoint{X, Y};

            if (Get_CanFitShapeAt(InGrid, InShapeOffsets, Position))
            { return Position; }
        }
    }

    return FIntPoint{-1, -1};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    DoGet_Intersections(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB,
        float InCellOverlapThreshold0to1)
    -> FCk_GridIntersectionResult
{
    SCOPE_CYCLE_COUNTER(STAT_Grid_Intersections);

    auto Result = FCk_GridIntersectionResult{};
    auto IntersectingCells = TArray<FCk_GridCellIntersection>{};
    auto IntersectingCellsA = TSet<FCk_Handle_2dGridCell>{};
    auto IntersectingCellsB = TSet<FCk_Handle_2dGridCell>{};
    auto TotalBounds = FBox2D{};
    auto FirstIntersection = true;
    auto BestIntersection = FCk_GridCellIntersection{};
    auto HighestOverlap = 0.0f;

    auto TotalCellsA = 0;
    auto TotalCellsB = 0;
    auto NumCellPairsTested = int32{0};

    TMap<FIntPoint, FCk_Handle_2dGridCell> GridBCellMap;
    TMap<FCk_Handle_2dGridCell, FBox2D> GridBBoundsCache;

    ForEach_Cell(InGridB, InFilterB, [&](const FCk_Handle_2dGridCell& CellB)
    {
        ++TotalCellsB;
        const auto& CoordB = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellB, ECk_2dGridSystem_CoordinateType::Local);
        const auto& BoundsB = UCk_Utils_2dGridCell_UE::Get_Bounds(CellB, ECk_LocalWorld::World);

        GridBCellMap.Add(CoordB, CellB);
        GridBBoundsCache.Add(CellB, BoundsB);
    });

    const auto& GridBWorldBounds = Get_Bounds(InGridB, ECk_LocalWorld::World, InFilterB);

    ForEach_Cell(InGridA, InFilterA, [&](const FCk_Handle_2dGridCell& CellA)
    {
        ++TotalCellsA;
        const auto& CoordA = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellA, ECk_2dGridSystem_CoordinateType::Local);
        const auto& BoundsA = UCk_Utils_2dGridCell_UE::Get_Bounds(CellA, ECk_LocalWorld::World);

        if (NOT BoundsA.Intersect(GridBWorldBounds))
        { return; }

        for (const auto& [CoordB, CellB] : GridBCellMap)
        {
            ++NumCellPairsTested;

            const auto& BoundsB = GridBBoundsCache[CellB];

            if (NOT BoundsA.Intersect(BoundsB))
            { continue; }

            const auto OverlapPercent = UCk_Utils_Geometry2D_UE::Calculate_OverlapPercent(BoundsA, BoundsB);
            if (OverlapPercent >= InCellOverlapThreshold0to1)
            {
                auto Intersection = FCk_GridCellIntersection{};
                Intersection.Set_CellA(CellA);
                Intersection.Set_CellB(CellB);
                Intersection.Set_CoordinateA(CoordA);
                Intersection.Set_CoordinateB(CoordB);
                Intersection.Set_OverlapPercent(OverlapPercent);

                const auto IntersectionBounds = UCk_Utils_Geometry2D_UE::Get_Box_Overlap(BoundsA, BoundsB);
                Intersection.Set_IntersectionBounds(IntersectionBounds);

                IntersectingCells.Add(Intersection);
                IntersectingCellsA.Add(CellA);
                IntersectingCellsB.Add(CellB);

                if (OverlapPercent > HighestOverlap)
                {
                    HighestOverlap = OverlapPercent;
                    BestIntersection = Intersection;
                }

                if (FirstIntersection)
                {
                    TotalBounds = IntersectionBounds;
                    FirstIntersection = false;
                }
                else
                {
                    TotalBounds = TotalBounds + IntersectionBounds;
                }
            }
        }
    });

    Result.Set_IntersectingCells(IntersectingCells);
    Result.Set_TotalIntersections(IntersectingCells.Num());
    Result.Set_TotalIntersectionBounds(TotalBounds);

    if (Result.Get_TotalIntersections() > 0)
    {
        const auto GridAOverlap = static_cast<float>(IntersectingCellsA.Num()) / static_cast<float>(TotalCellsA);
        const auto GridBOverlap = static_cast<float>(IntersectingCellsB.Num()) / static_cast<float>(TotalCellsB);

        Result.Set_GridAOverlapPercent(GridAOverlap);
        Result.Set_GridBOverlapPercent(GridBOverlap);
        Result.Set_GridAFullyContainedInGridB(GridAOverlap >= 1.0f);
        Result.Set_GridBFullyContainedInGridA(GridBOverlap >= 1.0f);

        if (HighestOverlap > 0.0f)
        {
            const auto GridBTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
                UCk_Utils_Transform_UE::CastChecked(InGridB));

            const auto CellAWorldBounds = UCk_Utils_2dGridCell_UE::Get_Bounds(
                BestIntersection.Get_CellA(), ECk_LocalWorld::World);
            const auto CellBWorldBounds = GridBBoundsCache[BestIntersection.Get_CellB()];

            const auto CellAWorldCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellAWorldBounds);
            const auto CellBWorldCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellBWorldBounds);

            const auto GridBWorldPos = FVector2D(GridBTransform.GetLocation());
            const auto SnapOffset = CellAWorldCenter - CellBWorldCenter;
            const auto SnapPos = GridBWorldPos + SnapOffset;

            Result.Set_SnapPosition(SnapPos);
            Result.Set_HasValidSnapPosition(true);
        }
    }

    INC_DWORD_STAT_BY(STAT_Grid_CellPairsTested, NumCellPairsTested);

    return Result;
}

auto
    UCk_Utils_2dGridSystem_UE::
    DoGet_Intersections_Aligned(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB,
        float InCellOverlapThreshold0to1)
    -> FCk_GridIntersectionResult
{
    SCOPE_CYCLE_COUNTER(STAT_Grid_Intersections);

    auto Result = FCk_GridIntersectionResult{};

    auto NumCellPairsTested = int32{0};

    const auto& PivotA = Get_Pivot(InGridA, ECk_LocalWorld::World);
    const auto& PivotB = Get_Pivot(InGridB, ECk_LocalWorld::World);
    const auto& CellSize = Get_CellSize(InGridA);
    const auto& DimensionsB = Get_Dimensions(InGridB);

    auto IntersectingCells = TArray<FCk_GridCellIntersection>{};
    auto IntersectingCellsA = TSet<FCk_Handle_2dGridCell>{};
    auto IntersectingCellsB = TSet<FCk_Handle_2dGridCell>{};
    auto TotalBounds = FBox2D{};
    auto BestIntersection = FCk_GridCellIntersection{};
    auto HighestOverlap = 0.0f;
    auto FirstIntersection = true;
    int32 TotalCellsA = 0;
    int32 TotalCellsB = 0;

    const auto TransformAToWorld = PivotA;
    const auto TransformWorldToB = PivotB.Inverse();
    const auto TransformAToB = TransformAToWorld * TransformWorldToB;

    TSet<FIntPoint> ActiveCellsB;
    ForEach_Cell(InGridB, InFilterB, [&](const FCk_Handle_2dGridCell& CellB)
    {
        ++TotalCellsB;
        const auto& CoordB = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellB, ECk_2dGridSystem_CoordinateType::Local);
        ActiveCellsB.Add(CoordB);
    });

    auto BoundsCache = TMap<FIntPoint, FBox2D>{};

    ForEach_Cell(InGridA, InFilterA, [&](const FCk_Handle_2dGridCell& CellA)
    {
        ++TotalCellsA;
        const auto& CoordA = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellA, ECk_2dGridSystem_CoordinateType::Local);

        const auto CellMinA = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(CoordA, CellSize);
        const auto CellMaxA = CellMinA + CellSize;

        const auto Corners = TArray{
            FVector(CellMinA.X, CellMinA.Y, 0.0f),
            FVector(CellMaxA.X, CellMinA.Y, 0.0f),
            FVector(CellMaxA.X, CellMaxA.Y, 0.0f),
            FVector(CellMinA.X, CellMaxA.Y, 0.0f)};

        auto MinCoordB = FIntPoint(INT_MAX, INT_MAX);
        auto MaxCoordB = FIntPoint(INT_MIN, INT_MIN);

        for (const auto& Corner : Corners)
        {
            const auto TransformedCorner = TransformAToB.TransformPosition(Corner);
            const auto CellCoordB = FIntPoint(
                FMath::FloorToInt(TransformedCorner.X / CellSize.X),
                FMath::FloorToInt(TransformedCorner.Y / CellSize.Y));

            MinCoordB.X = FMath::Min(MinCoordB.X, CellCoordB.X);
            MinCoordB.Y = FMath::Min(MinCoordB.Y, CellCoordB.Y);
            MaxCoordB.X = FMath::Max(MaxCoordB.X, CellCoordB.X);
            MaxCoordB.Y = FMath::Max(MaxCoordB.Y, CellCoordB.Y);
        }

        MinCoordB.X = FMath::Max(0, MinCoordB.X);
        MinCoordB.Y = FMath::Max(0, MinCoordB.Y);
        MaxCoordB.X = FMath::Min(DimensionsB.X - 1, MaxCoordB.X);
        MaxCoordB.Y = FMath::Min(DimensionsB.Y - 1, MaxCoordB.Y);

        if (MinCoordB.X > MaxCoordB.X || MinCoordB.Y > MaxCoordB.Y)
        { return; }

        const auto& BoundsA = UCk_Utils_2dGridCell_UE::Get_Bounds(CellA, ECk_LocalWorld::World);

        for (auto Y = MinCoordB.Y; Y <= MaxCoordB.Y; ++Y)
        {
            for (auto X = MinCoordB.X; X <= MaxCoordB.X; ++X)
            {
                const auto CoordB = FIntPoint(X, Y);

                if (NOT ActiveCellsB.Contains(CoordB))
                { continue; }

                FBox2D BoundsB;
                if (auto* CachedBounds = BoundsCache.Find(CoordB))
                {
                    BoundsB = *CachedBounds;
                }
                else
                {
                    const auto CellB = Get_CellAt(InGridB, CoordB);
                    BoundsB = UCk_Utils_2dGridCell_UE::Get_Bounds(CellB, ECk_LocalWorld::World);
                    BoundsCache.Add(CoordB, BoundsB);
                }

                ++NumCellPairsTested;
                const auto& OverlapPercent = UCk_Utils_Geometry2D_UE::Calculate_OverlapPercent(BoundsA, BoundsB);

                if (OverlapPercent < InCellOverlapThreshold0to1)
                { continue; }

                const auto CellB = Get_CellAt(InGridB, CoordB);

                const auto& IntersectionBounds = UCk_Utils_Geometry2D_UE::Get_Box_Overlap(BoundsA, BoundsB);

                const auto Intersection = FCk_GridCellIntersection{}
                    .Set_CellA(CellA)
                    .Set_CellB(CellB)
                    .Set_CoordinateA(CoordA)
                    .Set_CoordinateB(CoordB)
                    .Set_OverlapPercent(OverlapPercent)
                    .Set_IntersectionBounds(IntersectionBounds);

                IntersectingCells.Add(Intersection);
                IntersectingCellsA.Add(CellA);
                IntersectingCellsB.Add(CellB);

                if (OverlapPercent > HighestOverlap)
                {
                    HighestOverlap = OverlapPercent;
                    BestIntersection = Intersection;
                }

                TotalBounds = FirstIntersection ? IntersectionBounds : (TotalBounds + IntersectionBounds);
                FirstIntersection = false;
            }
        }
    });

    INC_DWORD_STAT_BY(STAT_Grid_CellPairsTested, NumCellPairsTested);

    Result.Set_IntersectingCells(IntersectingCells);
    Result.Set_TotalIntersections(IntersectingCells.Num());
    Result.Set_TotalIntersectionBounds(TotalBounds);

    if (Result.Get_TotalIntersections() == 0)
    { return Result; }

    const auto GridAOverlap = static_cast<float>(IntersectingCellsA.Num()) / static_cast<float>(TotalCellsA);
    const auto GridBOverlap = static_cast<float>(IntersectingCellsB.Num()) / static_cast<float>(TotalCellsB);

    Result.Set_GridAOverlapPercent(GridAOverlap);
    Result.Set_GridBOverlapPercent(GridBOverlap);
    Result.Set_GridAFullyContainedInGridB(GridAOverlap >= 1.0f);
    Result.Set_GridBFullyContainedInGridA(GridBOverlap >= 1.0f);

    if (HighestOverlap > 0.0f)
    {
        const auto GridBTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
            UCk_Utils_Transform_UE::CastChecked(InGridB));

        FBox2D CellBWorldBounds;
        if (auto* CachedBounds = BoundsCache.Find(BestIntersection.Get_CoordinateB()))
        {
            CellBWorldBounds = *CachedBounds;
        }
        else
        {
            CellBWorldBounds = UCk_Utils_2dGridCell_UE::Get_Bounds(
                BestIntersection.Get_CellB(), ECk_LocalWorld::World);
        }

        const auto& CellAWorldBounds = UCk_Utils_2dGridCell_UE::Get_Bounds(
            BestIntersection.Get_CellA(), ECk_LocalWorld::World);

        const auto& CellAWorldCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellAWorldBounds);
        const auto& CellBWorldCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellBWorldBounds);

        const auto GridBWorldPos = FVector2D(GridBTransform.GetLocation());
        const auto SnapOffset = CellAWorldCenter - CellBWorldCenter;
        const auto SnapPos = GridBWorldPos + SnapOffset;

        Result.Set_SnapPosition(SnapPos);
        Result.Set_HasValidSnapPosition(true);
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
