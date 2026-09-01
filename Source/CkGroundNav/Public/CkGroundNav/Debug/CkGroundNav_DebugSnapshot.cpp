#include "CkGroundNav_DebugSnapshot.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_StatusName(
            EDebugSnapshotStatus InStatus)
        -> const TCHAR*
    {
        switch (InStatus)
        {
            case EDebugSnapshotStatus::NeverBuilt:         return TEXT("NeverBuilt");
            case EDebugSnapshotStatus::BackendUnavailable: return TEXT("BackendUnavailable");
            case EDebugSnapshotStatus::NoGeometryInRegion: return TEXT("NoGeometryInRegion");
            case EDebugSnapshotStatus::Failed:             return TEXT("Failed");
            case EDebugSnapshotStatus::Current:            return TEXT("Current");
            default:                                       return TEXT("Unknown");
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Do_RecordRejectedCells(
            const FCk_GroundNav_SpanField& InBeforeFilter,
            const FCk_GroundNav_SpanField& InAfterFilter,
            FCk_GroundNav_DebugSnapshot&   InOutSnapshot)
        -> void
    {
        if (InBeforeFilter._Columns.Num() != InAfterFilter._Columns.Num())
        { return; }

        const auto HalfCell = static_cast<double>(InAfterFilter._CellSizeUu) * 0.5;

        for (auto Y = 0; Y < InAfterFilter._SizeY; ++Y)
        {
            for (auto X = 0; X < InAfterFilter._SizeX; ++X)
            {
                const auto& Before = InBeforeFilter.Get_Column(X, Y);
                const auto& After = InAfterFilter.Get_Column(X, Y);

                if (Before.Num() != After.Num())
                { continue; }

                for (auto Index = 0; Index < After.Num(); ++Index)
                {
                    if (NOT Before[Index]._IsWalkable || After[Index]._IsWalkable)
                    { continue; }

                    ++InOutSnapshot._RejectedCellCount;

                    const auto Corner = InAfterFilter.Get_ColumnMinCorner(X, Y);

                    auto Cell = FCk_GroundNav_DebugCell{};
                    Cell._SurfaceCentre = FVector{
                        Corner.X + HalfCell, Corner.Y + HalfCell, static_cast<double>(After[Index]._MaxZ)};
                    Cell._LayerIndex = 0;
                    Cell._PlateIndex = FCk_GroundNav_Plate::kNoPlate;

                    InOutSnapshot._RejectedCells.Emplace(Cell);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Make_DebugSnapshot(
            const FCk_GroundNav_SpanField&      InSpans,
            const FCk_GroundNav_LayerField&     InLayers,
            const FCk_GroundNav_ClearanceField& InClearance,
            const FCk_GroundNav_PlateField&     InPlates,
            const FBox&                         InRegion,
            int32                               InMaxCells)
        -> FCk_GroundNav_DebugSnapshot
    {
        auto Snapshot = FCk_GroundNav_DebugSnapshot{};

        Snapshot._Region = InRegion;
        Snapshot._CellSizeUu = InSpans._CellSizeUu;
        Snapshot._LatticeSizeX = InLayers._SizeX;
        Snapshot._LatticeSizeY = InLayers._SizeY;
        Snapshot._LayerCount = InLayers._LayerCount;
        Snapshot._SpanCount = InSpans.Get_TotalSpanCount();
        Snapshot._MaxClearanceUu = InClearance.Get_MaxClearance();

        const auto HalfCell = static_cast<double>(InSpans._CellSizeUu) * 0.5;

        for (auto LayerIndex = 0; LayerIndex < InLayers._LayerCount; ++LayerIndex)
        {
            for (auto Y = 0; Y < InLayers._SizeY; ++Y)
            {
                for (auto X = 0; X < InLayers._SizeX; ++X)
                {
                    auto TopZ = 0.0f;
                    auto Normal = FVector::UpVector;

                    if (NOT Get_CellSurface(InSpans, InLayers, X, Y, LayerIndex, TopZ, Normal))
                    { continue; }

                    // Counted before the cap, so the reported total describes the bake rather than
                    // the draw budget.
                    ++Snapshot._WalkableCellCount;

                    if (Snapshot._Cells.Num() >= InMaxCells)
                    {
                        Snapshot._CellsWereTruncated = true;
                        continue;
                    }

                    const auto Corner = InSpans.Get_ColumnMinCorner(X, Y);

                    auto Cell = FCk_GroundNav_DebugCell{};
                    Cell._SurfaceCentre = FVector{Corner.X + HalfCell, Corner.Y + HalfCell, static_cast<double>(TopZ)};
                    Cell._ClearanceUu = InClearance.Get_ClearanceAt(X, Y, LayerIndex);
                    Cell._LayerIndex = LayerIndex;
                    Cell._PlateIndex = InPlates.Get_PlateIndexAt(X, Y, LayerIndex);

                    Snapshot._Cells.Emplace(Cell);
                }
            }
        }

        Snapshot._Plates.Reserve(InPlates._Plates.Num());

        for (const auto& Plate : InPlates._Plates)
        {
            const auto MinCorner = InSpans.Get_ColumnMinCorner(Plate._MinX, Plate._MinY);
            const auto MaxCorner = InSpans.Get_ColumnMinCorner(Plate._MaxX, Plate._MaxY);

            auto LowestZ = TNumericLimits<double>::Max();
            auto HighestZ = TNumericLimits<double>::Lowest();

            for (auto Y = Plate._MinY; Y <= Plate._MaxY; ++Y)
            {
                for (auto X = Plate._MinX; X <= Plate._MaxX; ++X)
                {
                    auto TopZ = 0.0f;
                    auto Normal = FVector::UpVector;

                    if (NOT Get_CellSurface(InSpans, InLayers, X, Y, Plate._LayerIndex, TopZ, Normal))
                    { continue; }

                    LowestZ = FMath::Min(LowestZ, static_cast<double>(TopZ));
                    HighestZ = FMath::Max(HighestZ, static_cast<double>(TopZ));
                }
            }

            if (LowestZ > HighestZ)
            { continue; }

            auto DebugPlate = FCk_GroundNav_DebugPlate{};
            DebugPlate._Bounds = FBox{
                FVector{MinCorner.X, MinCorner.Y, LowestZ},
                FVector{MaxCorner.X + (HalfCell * 2.0), MaxCorner.Y + (HalfCell * 2.0), HighestZ}};
            DebugPlate._LayerIndex = Plate._LayerIndex;
            DebugPlate._HeightRangeUu = Plate._HeightRangeUu;
            DebugPlate._MaxPlaneResidualUu = Plate._MaxPlaneResidualUu;

            Snapshot._Plates.Emplace(DebugPlate);
        }

        Snapshot._CollapseRatio = InPlates.Get_CollapseRatio();
        Snapshot._MaxPlaneResidualUu = InPlates.Get_MaxPlaneResidualUu();
        Snapshot._MaxPlateHeightRangeUu = InPlates.Get_MaxHeightRangeUu();

        Snapshot._Status = EDebugSnapshotStatus::Current;

        return Snapshot;
    }
}

// --------------------------------------------------------------------------------------------------------------------
