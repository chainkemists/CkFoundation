#include "Ck2dGridSystem_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Geometry/CkGeometry_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
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
    Get_ActiveCoordinates(
        const FCk_Handle_2dGridSystem& InGrid)
    -> TArray<FIntPoint>
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
    Get_Intersections(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB,
        float InTolerancePercent,
        float InSubstantialOverlapThreshold)
    -> FCk_GridIntersectionResult
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGridA), TEXT("GridA is invalid")) { return {}; }
    CK_ENSURE_IF_NOT(ck::IsValid(InGridB), TEXT("GridB is invalid")) { return {}; }
    CK_ENSURE_IF_NOT(InTolerancePercent >= 0.0f && InTolerancePercent <= 1.0f,
        TEXT("TolerancePercent [{}] must be between 0.0 and 1.0"), InTolerancePercent) { return {}; }

    auto Result = FCk_GridIntersectionResult{};
    auto IntersectingCells = TArray<FCk_GridCellIntersection>{};

    // Get all cells from both grids
    auto CellsA = Get_AllCells(InGridA, InFilterA);
    auto CellsB = Get_AllCells(InGridB, InFilterB);

    CK_ENSURE_IF_NOT(NOT CellsA.IsEmpty() && NOT CellsB.IsEmpty(),
        TEXT("Cannot calculate intersections - one or both grids have no cells matching the filter")) { return {}; }

    auto IntersectingCellsA = TSet<FCk_Handle_2dGridCell>{};
    auto IntersectingCellsB = TSet<FCk_Handle_2dGridCell>{};
    auto TotalBounds = FBox2D{};
    auto FirstIntersection = true;
    auto BestIntersection = FCk_GridCellIntersection{};
    auto HighestOverlap = 0.0f;

    // Check each cell in GridA against each cell in GridB
    for (const auto& CellA : CellsA)
    {
        const auto CoordA = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellA, ECk_2dGridSystem_CoordinateType::Rotated);
        const auto BoundsA = UCk_Utils_2dGridCell_UE::Get_WorldBounds(CellA, ECk_2dGridSystem_CoordinateType::Rotated);

        for (const auto& CellB : CellsB)
        {
            const auto CoordB = UCk_Utils_2dGridCell_UE::Get_Coordinate(CellB, ECk_2dGridSystem_CoordinateType::Rotated);
            const auto BoundsB = UCk_Utils_2dGridCell_UE::Get_WorldBounds(CellB, ECk_2dGridSystem_CoordinateType::Rotated);

            const auto OverlapPercent = UCk_Utils_Geometry2D_UE::Calculate_OverlapPercent(BoundsA, BoundsB);

            if (OverlapPercent >= InTolerancePercent)
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

        Result.Set_HasSubstantialOverlap((GridAOverlap >= InSubstantialOverlapThreshold) ||
                                        (GridBOverlap >= InSubstantialOverlapThreshold));

        // Calculate snap position using the best overlapping cells
        if (HighestOverlap > 0.0f)
        {
            const auto GridATransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
                UCk_Utils_Transform_UE::CastChecked(InGridA));
            const auto GridBTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
                UCk_Utils_Transform_UE::CastChecked(InGridB));

            const auto GridAWorldPos = FVector2D(GridATransform.GetLocation());
            const auto GridBWorldPos = FVector2D(GridBTransform.GetLocation());

            const auto CellSizeA = Get_CellSize(InGridA);
            const auto CellSizeB = Get_CellSize(InGridB);

            // Use LOCAL coordinates for snap calculation, not rotated ones
            const auto LocalCoordA = UCk_Utils_2dGridCell_UE::Get_Coordinate(
                BestIntersection.Get_CellA(), ECk_2dGridSystem_CoordinateType::Local);
            const auto LocalCoordB = UCk_Utils_2dGridCell_UE::Get_Coordinate(
                BestIntersection.Get_CellB(), ECk_2dGridSystem_CoordinateType::Local);

            const auto SnapPos = UCk_Utils_Grid2D_UE::Calculate_PerfectSnapPosition(
                GridAWorldPos,
                LocalCoordA,
                CellSizeA,
                GridBWorldPos,
                LocalCoordB,
                CellSizeB);

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
    const auto RotationYaw = Transform.GetRotation().Rotator().Yaw;

    // Normalize to 0, 90, 180, 270 degrees (assuming grid rotations are in 90° increments)
    auto NormalizedYaw = FMath::RoundToInt(RotationYaw / 90.0f) * 90;
    NormalizedYaw = NormalizedYaw % 360;
    if (NormalizedYaw < 0)
    {
        NormalizedYaw += 360;
    }

    return NormalizedYaw;
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

    // Add half cell size to get the center of the cell (more intuitive rotation point)
    const auto AnchorCenterPos = AnchorLocalPos + (CellSize * 0.5f);

    // Return the negative offset - this is where to position the grid entity
    // so that the anchor coordinate becomes the rotation origin
    return FVector(-AnchorCenterPos.X, -AnchorCenterPos.Y, 0.0f);
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