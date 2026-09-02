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
        FCk_GroundNav_DebugSnapshot::
        Get_NarrowestPortalUu() const
        -> float
    {
        auto Narrowest = 0.0f;
        auto Found = false;

        for (const auto& Portal : _Portals)
        {
            Narrowest = Found ? FMath::Min(Narrowest, Portal._TraversalClearanceUu) : Portal._TraversalClearanceUu;
            Found = true;
        }

        return Narrowest;
    }

    auto
        FCk_GroundNav_DebugSnapshot::
        Get_BuiltTileCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& Tile : _Tiles)
        {
            if (Tile._IsBuilt)
            { ++Count; }
        }

        return Count;
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
            const FCk_GroundNav_PortalField&    InPortals,
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

        Snapshot._Portals.Reserve(InPortals._Portals.Num());

        for (const auto& Portal : InPortals._Portals)
        {
            const auto IsPlatePairValid =
                InPlates._Plates.IsValidIndex(Portal._PlateA) && InPlates._Plates.IsValidIndex(Portal._PlateB);

            if (NOT IsPlatePairValid)
            { continue; }

            auto DebugPortal = FCk_GroundNav_DebugPortal{};

            Portal.Get_Endpoints(InSpans, DebugPortal._MinEnd, DebugPortal._MaxEnd);

            DebugPortal._TraversalClearanceUu = Portal._TraversalClearanceUu;
            DebugPortal._IsCrossLayer =
                InPlates._Plates[Portal._PlateA]._LayerIndex != InPlates._Plates[Portal._PlateB]._LayerIndex;

            Snapshot._Portals.Emplace(DebugPortal);
        }

        Snapshot._CollapseRatio = InPlates.Get_CollapseRatio();
        Snapshot._MaxPlaneResidualUu = InPlates.Get_MaxPlaneResidualUu();
        Snapshot._MaxPlateHeightRangeUu = InPlates.Get_MaxHeightRangeUu();

        Snapshot._Status = EDebugSnapshotStatus::Current;

        return Snapshot;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Make_DebugSnapshotFromField(
            const FCk_GroundNav_Field& InField,
            int32                      InMaxCells)
        -> FCk_GroundNav_DebugSnapshot
    {
        auto Snapshot = FCk_GroundNav_DebugSnapshot{};

        Snapshot._Region = InField._Params.Get_Bounds();
        Snapshot._CellSizeUu = InField._Params._Config.Get_CellSizeUu();
        Snapshot._Status = EDebugSnapshotStatus::Current;

        // Every tile of a field covers the same whole number of cells, so the field's lattice is that
        // count times its divisions. Derived from the params rather than from the tiles, because an
        // unbuilt tile carries no size and would shrink the bound the per-cell counts are judged against.
        const auto CellSizeUu = static_cast<double>(InField._Params._Config.Get_CellSizeUu());
        const auto TileCellCount = CellSizeUu > 0.0
            ? FMath::RoundToInt32(InField._Params.Get_TileSpanUu() / CellSizeUu)
            : 0;

        Snapshot._LatticeSizeX = TileCellCount * InField._Params._Divisions.X;
        Snapshot._LatticeSizeY = TileCellCount * InField._Params._Divisions.Y;

        Snapshot._Tiles.Reserve(InField.Get_TileCount());

        for (const auto& Tile : InField._Tiles)
        {
            auto DebugTile = FCk_GroundNav_DebugTile{};

            const auto SpanUu = static_cast<double>(Tile._SizeX) * Tile._CellSizeUu;

            DebugTile._Bounds = FBox{
                FVector{Tile._Origin.X, Tile._Origin.Y, InField._Params._MinZUu},
                FVector{Tile._Origin.X + SpanUu, Tile._Origin.Y + SpanUu, InField._Params._MaxZUu}};

            DebugTile._IsBuilt = Tile.Get_IsBuilt();
            DebugTile._PlateCount = Tile._Plates._Plates.Num();
            DebugTile._WalkableCellCount = Tile.Get_WalkableCellCount();

            Snapshot._Tiles.Emplace(DebugTile);

            if (NOT Tile.Get_IsBuilt())
            { continue; }

            Snapshot._LayerCount = FMath::Max(Snapshot._LayerCount, Tile._LayerCount);
            Snapshot._MaxClearanceUu = FMath::Max(
                Snapshot._MaxClearanceUu, Tile._Clearance.Get_MaxClearance());

            const auto HalfCell = static_cast<double>(Tile._CellSizeUu) * 0.5;

            for (auto LayerIndex = 0; LayerIndex < Tile._LayerCount; ++LayerIndex)
            {
                for (auto Y = 0; Y < Tile._SizeY; ++Y)
                {
                    for (auto X = 0; X < Tile._SizeX; ++X)
                    {
                        if (NOT Tile.Get_HasSurfaceAt(X, Y, LayerIndex))
                        { continue; }

                        // Counted before the cap, so the reported total describes the field rather than
                        // the draw budget.
                        ++Snapshot._WalkableCellCount;

                        if (Snapshot._Cells.Num() >= InMaxCells)
                        {
                            Snapshot._CellsWereTruncated = true;
                            continue;
                        }

                        auto Cell = FCk_GroundNav_DebugCell{};

                        Cell._SurfaceCentre = Tile.Get_CellCentre(X, Y, LayerIndex);
                        Cell._ClearanceUu = Tile._Clearance.Get_ClearanceAt(X, Y, LayerIndex);
                        Cell._LayerIndex = LayerIndex;
                        Cell._PlateIndex = Tile._Plates.Get_PlateIndexAt(X, Y, LayerIndex);

                        Snapshot._Cells.Emplace(Cell);
                    }
                }
            }

            for (const auto& Plate : Tile._Plates._Plates)
            {
                auto LowestZ = TNumericLimits<double>::Max();
                auto HighestZ = TNumericLimits<double>::Lowest();

                for (auto Y = Plate._MinY; Y <= Plate._MaxY; ++Y)
                {
                    for (auto X = Plate._MinX; X <= Plate._MaxX; ++X)
                    {
                        if (NOT Tile.Get_HasSurfaceAt(X, Y, Plate._LayerIndex))
                        { continue; }

                        const auto SurfaceZ = static_cast<double>(
                            Tile.Get_SurfaceZAt(X, Y, Plate._LayerIndex));

                        LowestZ = FMath::Min(LowestZ, SurfaceZ);
                        HighestZ = FMath::Max(HighestZ, SurfaceZ);
                    }
                }

                if (LowestZ > HighestZ)
                { continue; }

                const auto Cell = static_cast<double>(Tile._CellSizeUu);

                auto DebugPlate = FCk_GroundNav_DebugPlate{};

                DebugPlate._Bounds = FBox{
                    FVector{
                        Tile._Origin.X + (static_cast<double>(Plate._MinX) * Cell),
                        Tile._Origin.Y + (static_cast<double>(Plate._MinY) * Cell),
                        LowestZ},
                    FVector{
                        Tile._Origin.X + (static_cast<double>(Plate._MaxX + 1) * Cell),
                        Tile._Origin.Y + (static_cast<double>(Plate._MaxY + 1) * Cell),
                        HighestZ}};

                DebugPlate._LayerIndex = Plate._LayerIndex;
                DebugPlate._HeightRangeUu = Plate._HeightRangeUu;
                DebugPlate._MaxPlaneResidualUu = Plate._MaxPlaneResidualUu;

                Snapshot._Plates.Emplace(DebugPlate);

                Snapshot._MaxPlaneResidualUu = FMath::Max(
                    Snapshot._MaxPlaneResidualUu, Plate._MaxPlaneResidualUu);
                Snapshot._MaxPlateHeightRangeUu = FMath::Max(
                    Snapshot._MaxPlateHeightRangeUu, Plate._HeightRangeUu);
            }

            for (const auto& Portal : Tile._Portals._Portals)
            {
                auto DebugPortal = FCk_GroundNav_DebugPortal{};

                Portal.Get_Endpoints(Tile._Origin, Tile._CellSizeUu, DebugPortal._MinEnd, DebugPortal._MaxEnd);

                DebugPortal._TraversalClearanceUu = Portal._TraversalClearanceUu;
                DebugPortal._IsCrossLayer =
                    Tile._Plates._Plates.IsValidIndex(Portal._PlateA) &&
                    Tile._Plates._Plates.IsValidIndex(Portal._PlateB) &&
                    Tile._Plates._Plates[Portal._PlateA]._LayerIndex !=
                        Tile._Plates._Plates[Portal._PlateB]._LayerIndex;

                Snapshot._Portals.Emplace(DebugPortal);
            }
        }

        Snapshot._Seams.Reserve(InField._SeamPortals.Num());

        for (const auto& Seam : InField._SeamPortals)
        {
            if (NOT InField._Tiles.IsValidIndex(Seam._TileIndexA))
            { continue; }

            auto DebugSeam = FCk_GroundNav_DebugSeam{};

            Seam.Get_Endpoints(InField._Tiles[Seam._TileIndexA], DebugSeam._MinEnd, DebugSeam._MaxEnd);
            DebugSeam._TraversalClearanceUu = Seam._TraversalClearanceUu;

            Snapshot._Seams.Emplace(DebugSeam);
        }

        Snapshot._CollapseRatio = Snapshot._Plates.IsEmpty()
            ? 0.0f
            : static_cast<float>(Snapshot._WalkableCellCount) / static_cast<float>(Snapshot._Plates.Num());

        return Snapshot;
    }
}

// --------------------------------------------------------------------------------------------------------------------
