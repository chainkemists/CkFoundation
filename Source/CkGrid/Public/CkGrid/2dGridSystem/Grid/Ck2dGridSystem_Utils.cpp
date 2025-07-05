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

#include "CkGrid/CkGrid_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridSystem_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_2dGridSystem_ParamsData& InParams)
    -> FCk_Handle_2dGridSystem
{
    // Validate all active coordinates are within grid dimensions
    const auto ResolvedActiveCoords = InParams.Get_ResolvedActiveCoordinates();
    for (const auto& Coordinate : ResolvedActiveCoords)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Grid2D_UE::Get_IsValidCoordinate(InParams.Get_Dimensions(), Coordinate),
            TEXT("Cannot Create 2dGridSystem because ActiveCoordinate [{}] is invalid for grid dimensions [{}]"),
            Coordinate, InParams.Get_Dimensions())
        { return {}; }
    }

    InHandle.Add<ck::FFragment_2dGridSystem_Params>(InParams);

    auto PivotSceneNode = UCk_Utils_SceneNode_UE::Create(InHandle, InParams.Get_Pivot());
    UCk_Utils_Handle_UE::Set_DebugName(PivotSceneNode, TEXT("GridPivot"));
    InHandle.Add<ck::FFragment_2dGridSystem_Current>(PivotSceneNode);
    auto GridEntity = Cast(InHandle);

    const auto& Dimensions = InParams.Get_Dimensions();

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Coordinate = FIntPoint(X, Y);

            // Determine if this cell should be disabled using new system
            const auto EnabledState = InParams.Get_IsCoordinateActive(Coordinate)
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
    Get_Pivot(
        const FCk_Handle_2dGridSystem& InGrid)
    -> FCk_Handle_SceneNode
{
    return InGrid.Get<ck::FFragment_2dGridSystem_Current>().Get_Pivot();
}

auto
    UCk_Utils_2dGridSystem_UE::
    Request_UpdatePivot(
        const FCk_Handle_2dGridSystem& InGrid,
        const FTransform& InNewPivotTransform)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot update pivot for invalid grid")) { return; }

    auto PivotHandle = Get_Pivot(InGrid);
    UCk_Utils_SceneNode_UE::Request_UpdateOffset(PivotHandle, FCk_Request_SceneNode_UpdateRelativeTransform{InNewPivotTransform});
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
            PivotOffset = GridSize * 0.5f;
            break;
        case ECk_2dGridSystem_PivotAnchor::BottomLeft:
            PivotOffset = FVector2D::ZeroVector;
            break;
        case ECk_2dGridSystem_PivotAnchor::BottomCenter:
            PivotOffset = FVector2D(GridSize.X * 0.5f, 0.0f);
            break;
        case ECk_2dGridSystem_PivotAnchor::BottomRight:
            PivotOffset = FVector2D(GridSize.X, 0.0f);
            break;
        case ECk_2dGridSystem_PivotAnchor::MiddleLeft:
            PivotOffset = FVector2D(0.0f, GridSize.Y * 0.5f);
            break;
        case ECk_2dGridSystem_PivotAnchor::MiddleRight:
            PivotOffset = FVector2D(GridSize.X, GridSize.Y * 0.5f);
            break;
        case ECk_2dGridSystem_PivotAnchor::TopLeft:
            PivotOffset = FVector2D(0.0f, GridSize.Y);
            break;
        case ECk_2dGridSystem_PivotAnchor::TopCenter:
            PivotOffset = FVector2D(GridSize.X * 0.5f, GridSize.Y);
            break;
        case ECk_2dGridSystem_PivotAnchor::TopRight:
            PivotOffset = GridSize;
            break;
        default:
            CK_INVALID_ENUM(InAnchor);
            break;
    }

    // Get current pivot transform to preserve rotation and scale
    const auto PivotHandle = Get_Pivot(InGrid);
    const auto PivotTransformHandle = UCk_Utils_Transform_UE::Cast(PivotHandle);
    auto NewTransform = FTransform::Identity;

    if (ck::IsValid(PivotTransformHandle))
    {
        NewTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(PivotTransformHandle);
    }

    // Apply the calculated offset
    NewTransform.SetLocation(FVector(-PivotOffset.X, -PivotOffset.Y, NewTransform.GetLocation().Z));

    return NewTransform;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Request_SetPivotToAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_PivotAnchor InAnchor)
    -> void
{
    const auto NewPivotTransform = Get_PivotTransformForAnchor(InGrid, InAnchor);
    Request_UpdatePivot(InGrid, NewPivotTransform);
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
    Get_CellBoundsDimensions(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter)
    -> FIntPoint
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot get cell bounds for invalid grid"))
    { return {}; }

    const auto FilteredCells = Get_AllCells(InGrid, InCellFilter);
    CK_ENSURE_IF_NOT(NOT FilteredCells.IsEmpty(), TEXT("Grid has no cells matching filter [{}]"), InCellFilter)
    { return {}; }

    // Get coordinates of all filtered cells
    auto MinCoord = FIntPoint{INT_MAX, INT_MAX};
    auto MaxCoord = FIntPoint{INT_MIN, INT_MIN};

    for (const auto& Cell : FilteredCells)
    {
        const auto Coord = UCk_Utils_2dGridCell_UE::Get_Coordinate(Cell, ECk_2dGridSystem_CoordinateType::Local);

        MinCoord.X = FMath::Min(MinCoord.X, Coord.X);
        MinCoord.Y = FMath::Min(MinCoord.Y, Coord.Y);
        MaxCoord.X = FMath::Max(MaxCoord.X, Coord.X);
        MaxCoord.Y = FMath::Max(MaxCoord.Y, Coord.Y);
    }

    // Calculate dimensions (add 1 because coordinates are 0-based)
    return FIntPoint(MaxCoord.X - MinCoord.X + 1, MaxCoord.Y - MinCoord.Y + 1);
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

    auto Result = FCk_GridIntersectionResult{};
    auto IntersectingCells = TArray<FCk_GridCellIntersection>{};

    // Get all cells from both grids
    auto CellsA = Get_AllCells(InGridA, InFilterA);
    auto CellsB = Get_AllCells(InGridB, InFilterB);

    CK_ENSURE_IF_NOT(NOT CellsA.IsEmpty() && NOT CellsB.IsEmpty(),
        TEXT("Cannot calculate intersections - one or both grids have no cells matching the filter"))
    { return {}; }

    auto IntersectingCellsA = TSet<FCk_Handle_2dGridCell>{};
    auto IntersectingCellsB = TSet<FCk_Handle_2dGridCell>{};
    auto TotalBounds = FBox2D{};
    auto FirstIntersection = true;
    auto BestIntersection = FCk_GridCellIntersection{};
    auto HighestOverlap = 0.0f;

    // Check each cell in GridA against each cell in GridB
    for (const auto& CellA : CellsA)
    {
        const auto CoordA = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellA, ECk_2dGridSystem_CoordinateType::Local);
        const auto BoundsA = UCk_Utils_2dGridCell_UE::Get_WorldBounds(CellA, ECk_2dGridSystem_CoordinateType::Rotated);

        for (const auto& CellB : CellsB)
        {
            const auto CoordB = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellB, ECk_2dGridSystem_CoordinateType::Local);
            const auto BoundsB = UCk_Utils_2dGridCell_UE::Get_WorldBounds(CellB, ECk_2dGridSystem_CoordinateType::Rotated);

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

                // Track best intersection for snapping
                if (OverlapPercent > HighestOverlap)
                {
                    HighestOverlap = OverlapPercent;
                    BestIntersection = Intersection;
                }

                // Update total bounds
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
    }

    // Calculate summary statistics
    Result.Set_IntersectingCells(IntersectingCells);
    Result.Set_TotalIntersections(IntersectingCells.Num());
    Result.Set_TotalIntersectionBounds(TotalBounds);

    if (Result.Get_TotalIntersections() > 0)
    {
        const auto GridAOverlap = static_cast<float>(IntersectingCellsA.Num()) / static_cast<float>(CellsA.Num());
        const auto GridBOverlap = static_cast<float>(IntersectingCellsB.Num()) / static_cast<float>(CellsB.Num());

        Result.Set_GridAOverlapPercent(GridAOverlap);
        Result.Set_GridBOverlapPercent(GridBOverlap);

        Result.Set_GridAFullyContainedInGridB(GridAOverlap >= 1.0f);
        Result.Set_GridBFullyContainedInGridA(GridBOverlap >= 1.0f);

        // Calculate snap position using the best overlapping cells
        if (HighestOverlap > 0.0f)
        {
            const auto GridBTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
                UCk_Utils_Transform_UE::CastChecked(InGridB));

            // Get the actual world center positions of the intersecting cells
            const auto CellAWorldBounds = UCk_Utils_2dGridCell_UE::Get_WorldBounds(
                BestIntersection.Get_CellA(), ECk_2dGridSystem_CoordinateType::Local);
            const auto CellBWorldBounds = UCk_Utils_2dGridCell_UE::Get_WorldBounds(
                BestIntersection.Get_CellB(), ECk_2dGridSystem_CoordinateType::Local);

            const auto CellAWorldCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellAWorldBounds);
            const auto CellBWorldCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellBWorldBounds);

            // Calculate offset needed to align the cell centers
            const auto GridBWorldPos = FVector2D(GridBTransform.GetLocation());
            const auto SnapOffset = CellAWorldCenter - CellBWorldCenter;
            const auto SnapPos = GridBWorldPos + SnapOffset;

            Result.Set_SnapPosition(SnapPos);
            Result.Set_HasValidSnapPosition(true);
        }
    }

    return Result;
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_IntersectingCells(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        float InTolerancePercent)
    -> TArray<FCk_GridCellIntersection>
{
    const auto Result = Get_Intersections(InGridA, InGridB,
                                          ECk_2dGridSystem_CellFilter::OnlyActiveCells,
                                          ECk_2dGridSystem_CellFilter::OnlyActiveCells,
                                          InTolerancePercent);

    return Result.Get_IntersectingCells();
}
auto
    UCk_Utils_2dGridSystem_UE::
    Get_TransformRotationDegrees(
        const FCk_Handle_2dGridSystem& InGrid)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot get transform rotation from invalid grid"))
    { return 0; }

    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InGrid);
    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle),
        TEXT("Grid [{}] does not have a Transform component"), InGrid)
    { return 0; }

    const auto Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);
    auto RotationYaw = Transform.GetRotation().Rotator().Yaw;

    RotationYaw = FMath::Fmod(RotationYaw, 360.0f);
    if (RotationYaw < 0)
    {
        RotationYaw += 360;
    }

    return FMath::RoundToInt(RotationYaw);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_CoordinateRemappedForTransform(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InLocalCoordinate)
    -> FIntPoint
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot remap coordinate for invalid grid"))
    { return InLocalCoordinate; }

    const auto RotationDegrees = Get_TransformRotationDegrees(InGrid);
    if (RotationDegrees == 0)
    {
        return InLocalCoordinate;
    }

    // Use (0,0) as the default anchor for coordinate remapping
    return UCk_Utils_Grid2D_UE::RotateCoordinate(InLocalCoordinate, FIntPoint::ZeroValue, RotationDegrees);
}

auto
    UCk_Utils_2dGridSystem_UE::
    Get_EntityRotationDegrees(
        const FCk_Handle_2dGridSystem& InGrid)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot get entity rotation from invalid grid"))
    { return 0.0f; }

    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InGrid);
    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle),
        TEXT("Grid [{}] does not have a Transform component"), InGrid)
    { return 0.0f; }

    const auto Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);
    return Transform.GetRotation().Rotator().Yaw;
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

    // Calculate the local position of the desired anchor coordinate
    const auto AnchorLocalPos = UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation(InDesiredAnchorCoordinate, CellSize);

    // Add half cell size to get the center of the cell
    const auto AnchorCenterPos = AnchorLocalPos + (CellSize * 0.5f);

    // Return the negative offset - this positions the grid so the anchor becomes the rotation origin
    return FVector(-AnchorCenterPos.X, -AnchorCenterPos.Y, 0.0f);
}

auto
    UCk_Utils_2dGridSystem_UE::
    DebugDraw_Grid(
        const UObject* InWorldContextObject,
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_2dGridSystem_DebugDraw_Options& InOptions)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid), TEXT("Cannot debug draw invalid grid")) { return; }

    const auto& Options = InOptions;
    const auto CellVisualization = Options.Get_CellVisualization();

    // Early exit if nothing to draw
    if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::None &&
        NOT Options.Get_ShowCoordinates() &&
        NOT Options.Get_ShowPivot() &&
        NOT Options.Get_ShowCellSizeInfo())
    {
        return;
    }

    const auto PivotHandle = Get_Pivot(InGrid);
    const auto PivotTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(PivotHandle);

    const auto CellSize = Get_CellSize(InGrid);
    const auto Dimensions = Get_Dimensions(InGrid);

    // Draw cells
    if (CellVisualization != ECk_2dGridSystem_DebugDraw_CellVisualization::None)
    {
        ForEach_Cell(InGrid, ECk_2dGridSystem_CellFilter::NoFilter, [&](const FCk_Handle_2dGridCell& InCell)
        {
            const auto CellBounds = UCk_Utils_2dGridCell_UE::Get_WorldBounds(InCell, ECk_2dGridSystem_CoordinateType::Rotated);
            const auto CellCenter = UCk_Utils_Geometry2D_UE::Get_Box_Center(CellBounds);
            const auto CellExtent = UCk_Utils_Geometry2D_UE::Get_Box_Size(CellBounds) * 0.5f;

            const auto IsDisabled = UCk_Utils_2dGridCell_UE::Get_IsDisabled(InCell);
            const auto CellColor = IsDisabled ? Options.Get_DisabledCellColor() : Options.Get_EnabledCellColor();

            const auto CellCenter3D = FVector(CellCenter.X, CellCenter.Y, PivotTransform.GetLocation().Z);
            const auto CellExtent3D = FVector(CellExtent.X, CellExtent.Y, 0.0f);

            if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::Filled)
            {
                // Draw filled box
                UCk_Utils_DebugDraw_UE::DrawDebugBox(
                    InWorldContextObject,
                    CellCenter3D,
                    CellExtent3D,
                    CellColor,
                    FRotator::ZeroRotator,
                    Options.Get_Duration(),
                    Options.Get_CellThickness());
            }
            else if (CellVisualization == ECk_2dGridSystem_DebugDraw_CellVisualization::Outline)
            {
                // Draw wireframe box
                UCk_Utils_DebugDraw_UE::DrawDebugWireframeBox(
                    InWorldContextObject,
                    CellCenter3D,
                    CellExtent3D,
                    FQuat::Identity,
                    CellColor,
                    Options.Get_Duration(),
                    Options.Get_CellThickness());
            }

            // Draw coordinates if enabled
            if (Options.Get_ShowCoordinates())
            {
                const auto Coord = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);
                const auto CoordText = ck::Format_UE(TEXT("[{},{}]"), Coord.X, Coord.Y);
                const auto TextLocation = CellCenter3D + FVector(0.0f, 0.0f, 10.0f);

                UCk_Utils_DebugDraw_UE::DrawDebugString(
                    InWorldContextObject,
                    TextLocation,
                    CoordText,
                    static_cast<AActor*>(nullptr),
                    Options.Get_TextColor(),
                    Options.Get_Duration());
            }
        });
    }

    // Draw pivot marker
    if (Options.Get_ShowPivot())
    {
        const auto PivotLocation = PivotTransform.GetLocation();

        // Draw cross at pivot location
        UCk_Utils_DebugDraw_UE::DrawDebugCross(
            InWorldContextObject,
            PivotLocation,
            Options.Get_PivotSize(),
            Options.Get_PivotColor(),
            Options.Get_Duration(),
            Options.Get_CellThickness());

        // Draw transform gizmo at pivot
        UCk_Utils_DebugDraw_UE::DrawDebugTransformGizmo(
            InWorldContextObject,
            PivotTransform,
            Options.Get_PivotSize() * 0.8f,
            Options.Get_CellThickness(),
            true,
            Options.Get_PivotSize() * 0.2f,
            Options.Get_Duration());
    }

    // Draw cell size info
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
            static_cast<AActor*>(nullptr),
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
            break;
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

// --------------------------------------------------------------------------------------------------------------------