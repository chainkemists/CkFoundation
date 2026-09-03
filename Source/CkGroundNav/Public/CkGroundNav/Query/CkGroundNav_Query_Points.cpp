#include "CkGroundNav_Query_Points.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"

#include <Algo/BinarySearch.h>
#include <Math/RandomStream.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace pointsquery_private
    {
        // Enough that a plate whose drawable cells are a modest fraction of its window is answered by
        // rejection alone; past it the exact scan is cheaper than continuing to miss.
        constexpr auto kMaxRejectionTries = 64;

        // Draws a path-distance generator may spend per point asked for. A range only a sliver of the
        // flooded region satisfies must terminate rather than search for it forever.
        constexpr auto kDrawsPerRequestedPoint = 32;

        // ------------------------------------------------------------------------------------------------------------

        /** An inclusive rectangle of tile-local cells: a plate's own bounds, or those narrowed by a query. */
        struct FCellWindow
        {
            int32 _MinX = 0;
            int32 _MinY = 0;
            int32 _MaxX = 0;
            int32 _MaxY = 0;

            auto Get_IsEmpty() const -> bool { return _MinX > _MaxX || _MinY > _MaxY; }
        };

        // ------------------------------------------------------------------------------------------------------------

        /**
         * The horizontal disc a draw must land in, or an unbounded one.
         *
         * A negative radius admits every cell. The path-distance generator's region is decided by
         * connectivity and not by a reach, so it shares the weighting and the drawing with the radius
         * generator instead of growing a second copy of both.
         */
        struct FDiscFilter
        {
            FVector2D _CentreXY = FVector2D::ZeroVector;

            double _RadiusSquaredUu = -1.0;

            auto Get_Holds(const FVector2D& InPointXY) const -> bool
            {
                return _RadiusSquaredUu < 0.0 || FVector2D::DistSquared(InPointXY, _CentreXY) <= _RadiusSquaredUu;
            }
        };

        // ------------------------------------------------------------------------------------------------------------

        /** One plate a generator may draw from, and how many of its cells inside the window can be drawn. */
        struct FPlateCandidate
        {
            int32 _TileIndex = INDEX_NONE;
            int32 _PlateIndex = FCk_GroundNav_Plate::kNoPlate;
            int32 _LayerIndex = 0;

            FCellWindow _Window;

            int32 _Weight = 0;
        };

        // ------------------------------------------------------------------------------------------------------------

        auto Make_Window(
            const FCk_GroundNav_Plate& InPlate) -> FCellWindow
        {
            auto Window = FCellWindow{};
            Window._MinX = InPlate._MinX;
            Window._MinY = InPlate._MinY;
            Window._MaxX = InPlate._MaxX;
            Window._MaxY = InPlate._MaxY;

            return Window;
        }

        auto Make_Window(
            const FCk_GroundNav_Plate& InPlate,
            const FIntPoint&           InMinCell,
            const FIntPoint&           InMaxCell) -> FCellWindow
        {
            auto Window = FCellWindow{};
            Window._MinX = FMath::Max(InPlate._MinX, InMinCell.X);
            Window._MinY = FMath::Max(InPlate._MinY, InMinCell.Y);
            Window._MaxX = FMath::Min(InPlate._MaxX, InMaxCell.X);
            Window._MaxY = FMath::Min(InPlate._MaxY, InMaxCell.Y);

            return Window;
        }

        // ------------------------------------------------------------------------------------------------------------

        /**
         * Whether one cell can carry a drawn point: it belongs to the plate, it has a surface, its
         * clearance admits the body, and its CENTRE lies in the disc.
         *
         * The centre rather than any overlap is what makes the weighting exact: a cell counts once or
         * not at all, so a plate's weight is a cell count and the expectation a chi-square test forms
         * from it is a whole number of cells.
         */
        auto Get_IsCellDrawable(
            const FCk_GroundNav_Tile&       InTile,
            const FPlateCandidate&          InCandidate,
            int32                           InCellX,
            int32                           InCellY,
            const FCk_GroundNav_QueryAgent& InAgent,
            const FDiscFilter&              InDisc) -> bool
        {
            if (InTile._Plates.Get_PlateIndexAt(InCellX, InCellY, InCandidate._LayerIndex) != InCandidate._PlateIndex)
            { return false; }

            if (NOT InTile.Get_HasSurfaceAt(InCellX, InCellY, InCandidate._LayerIndex))
            { return false; }

            const auto ClearanceUu = InTile._Clearance.Get_ClearanceAt(InCellX, InCellY, InCandidate._LayerIndex);

            if (NOT Get_IsAdmitted(ClearanceUu, InAgent))
            { return false; }

            const auto CellSize = static_cast<double>(InTile._CellSizeUu);
            const auto HalfCell = CellSize * 0.5;

            const auto CentreXY = FVector2D{
                InTile._Origin.X + (static_cast<double>(InCellX) * CellSize) + HalfCell,
                InTile._Origin.Y + (static_cast<double>(InCellY) * CellSize) + HalfCell};

            return InDisc.Get_Holds(CentreXY);
        }

        /** How many of the window's cells a draw could land on. Bills one read per cell examined. */
        auto Get_DrawableCellCount(
            const FCk_GroundNav_Tile&       InTile,
            const FPlateCandidate&          InCandidate,
            const FCk_GroundNav_QueryAgent& InAgent,
            const FDiscFilter&              InDisc,
            FCk_GroundNav_QueryCost&        InOutCost) -> int32
        {
            auto Count = int32{0};

            for (auto CellY = InCandidate._Window._MinY; CellY <= InCandidate._Window._MaxY; ++CellY)
            {
                for (auto CellX = InCandidate._Window._MinX; CellX <= InCandidate._Window._MaxX; ++CellX)
                {
                    ++InOutCost._CellsRead;

                    if (Get_IsCellDrawable(InTile, InCandidate, CellX, CellY, InAgent, InDisc))
                    { ++Count; }
                }
            }

            return Count;
        }

        // ------------------------------------------------------------------------------------------------------------

        /**
         * A cell drawn uniformly among the candidate's drawable cells.
         *
         * Rejection first, and an exact scan once the tries run out. The scan draws afresh over the
         * weight already counted, so exhausting the tries costs a pass over the window and cannot skew
         * the answer toward whatever the rejection loop was missing.
         */
        auto Get_DrawnCell(
            const FCk_GroundNav_Tile&       InTile,
            const FPlateCandidate&          InCandidate,
            const FCk_GroundNav_QueryAgent& InAgent,
            const FDiscFilter&              InDisc,
            FRandomStream&                  InOutStream,
            FCk_GroundNav_QueryCost&        InOutCost,
            int32&                          OutCellX,
            int32&                          OutCellY) -> bool
        {
            if (InCandidate._Weight <= 0 || InCandidate._Window.Get_IsEmpty())
            { return false; }

            for (auto Try = 0; Try < kMaxRejectionTries; ++Try)
            {
                const auto CellX = InOutStream.RandRange(InCandidate._Window._MinX, InCandidate._Window._MaxX);
                const auto CellY = InOutStream.RandRange(InCandidate._Window._MinY, InCandidate._Window._MaxY);

                ++InOutCost._CellsRead;

                if (NOT Get_IsCellDrawable(InTile, InCandidate, CellX, CellY, InAgent, InDisc))
                { continue; }

                OutCellX = CellX;
                OutCellY = CellY;

                return true;
            }

            auto Skipped = InOutStream.RandRange(0, InCandidate._Weight - 1);

            for (auto CellY = InCandidate._Window._MinY; CellY <= InCandidate._Window._MaxY; ++CellY)
            {
                for (auto CellX = InCandidate._Window._MinX; CellX <= InCandidate._Window._MaxX; ++CellX)
                {
                    ++InOutCost._CellsRead;

                    if (NOT Get_IsCellDrawable(InTile, InCandidate, CellX, CellY, InAgent, InDisc))
                    { continue; }

                    if (Skipped > 0)
                    {
                        --Skipped;
                        continue;
                    }

                    OutCellX = CellX;
                    OutCellY = CellY;

                    return true;
                }
            }

            return false;
        }

        // ------------------------------------------------------------------------------------------------------------

        /** The candidate a draw lands on, chosen with probability proportional to its weight. */
        auto Get_DrawnCandidateIndex(
            TConstArrayView<int32> InPrefixSums,
            FRandomStream&         InOutStream) -> int32
        {
            const auto TotalWeight = InPrefixSums.IsEmpty() ? 0 : InPrefixSums.Last();

            if (TotalWeight <= 0)
            { return INDEX_NONE; }

            return Algo::UpperBound(InPrefixSums, InOutStream.RandRange(0, TotalWeight - 1));
        }

        /**
         * One area-weighted draw: a plate by weight, a cell uniformly within it, and a point uniformly
         * inside that cell's square at that cell's surface height.
         */
        auto Get_DrawnPoint(
            const FCk_GroundNav_Field&       InField,
            TConstArrayView<FPlateCandidate> InCandidates,
            TConstArrayView<int32>           InPrefixSums,
            const FCk_GroundNav_QueryAgent&  InAgent,
            const FDiscFilter&               InDisc,
            FRandomStream&                   InOutStream,
            FCk_GroundNav_QueryCost&         InOutCost,
            FCk_GroundNav_GeneratedPoint&    OutPoint) -> bool
        {
            const auto CandidateIndex = Get_DrawnCandidateIndex(InPrefixSums, InOutStream);

            if (NOT InCandidates.IsValidIndex(CandidateIndex))
            { return false; }

            const auto& Candidate = InCandidates[CandidateIndex];

            if (NOT InField._Tiles.IsValidIndex(Candidate._TileIndex))
            { return false; }

            const auto& Tile = InField._Tiles[Candidate._TileIndex];

            auto CellX = int32{INDEX_NONE};
            auto CellY = int32{INDEX_NONE};

            if (NOT Get_DrawnCell(Tile, Candidate, InAgent, InDisc, InOutStream, InOutCost, CellX, CellY))
            { return false; }

            auto Address = FCk_GroundNav_CellAddress{};
            Address._TileIndex = Candidate._TileIndex;
            Address._CellX = CellX;
            Address._CellY = CellY;

            auto Surface = FCk_GroundNav_SurfaceRef{};
            auto SurfaceZUu = 0.0f;
            auto ClearanceUu = 0.0f;

            ++InOutCost._CellsRead;

            if (NOT Get_SurfaceAt(InField, Address, Candidate._LayerIndex, Surface, SurfaceZUu, ClearanceUu))
            { return false; }

            // Hoisted so the statements order the two draws, rather than the initializer they feed.
            const auto FractionX = static_cast<double>(InOutStream.GetFraction());
            const auto FractionY = static_cast<double>(InOutStream.GetFraction());

            const auto CellSize = static_cast<double>(Tile._CellSizeUu);

            OutPoint._Location = FVector{
                Tile._Origin.X + ((static_cast<double>(CellX) + FractionX) * CellSize),
                Tile._Origin.Y + ((static_cast<double>(CellY) + FractionY) * CellSize),
                static_cast<double>(SurfaceZUu)};
            OutPoint._Surface = Surface;

            return true;
        }

        // ------------------------------------------------------------------------------------------------------------

        auto DoAdd_Candidate(
            const FCk_GroundNav_Tile&       InTile,
            const FPlateCandidate&          InCandidate,
            const FCk_GroundNav_QueryAgent& InAgent,
            const FDiscFilter&              InDisc,
            TArray<FPlateCandidate>&        OutCandidates,
            TArray<int32>&                  OutPrefixSums,
            FCk_GroundNav_QueryCost&        InOutCost) -> void
        {
            if (InCandidate._Window.Get_IsEmpty())
            { return; }

            auto Candidate = InCandidate;
            Candidate._Weight = Get_DrawableCellCount(InTile, Candidate, InAgent, InDisc, InOutCost);

            if (Candidate._Weight <= 0)
            { return; }

            OutCandidates.Add(Candidate);
            OutPrefixSums.Add((OutPrefixSums.IsEmpty() ? 0 : OutPrefixSums.Last()) + Candidate._Weight);
        }

        /** Nothing qualified: Unbuilt when the footprint reached a tile that may still hold the answer. */
        auto Get_EmptyStatus(
            const FCk_GroundNav_QueryCost& InCost) -> ECk_NavSurface_QueryStatus
        {
            return InCost._TouchedUnbuiltTile
                ? ECk_NavSurface_QueryStatus::Unbuilt
                : ECk_NavSurface_QueryStatus::NoSurface;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_RandomPointsInRadius(
            const FCk_GroundNav_Field&             InField,
            const FCk_GroundNav_RandomPointsQuery& InQuery)
        -> FCk_GroundNav_PointsResult
    {
        using namespace pointsquery_private;

        auto Result = FCk_GroundNav_PointsResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        if (InQuery._Count <= 0)
        {
            Result._Status = ECk_NavSurface_QueryStatus::Success;

            return Result;
        }

        const auto& Params = InField._Params;

        const auto RadiusUu = static_cast<double>(InQuery._RadiusUu);
        const auto OriginXY = FVector2D{InQuery._Origin.X, InQuery._Origin.Y};

        auto Disc = FDiscFilter{};
        Disc._CentreXY = OriginXY;
        Disc._RadiusSquaredUu = RadiusUu * RadiusUu;

        const auto CellsPerTile = Get_CellsPerTile(Params);

        const auto MinCorner = Get_FieldCellAt(Params, OriginXY - FVector2D{RadiusUu, RadiusUu});
        const auto MaxCorner = Get_FieldCellAt(Params, OriginXY + FVector2D{RadiusUu, RadiusUu});

        // Clamped into the field's own cell range BEFORE any division, so the tile range below is a
        // floor division of non-negative numbers and a disc wholly off the field is empty here rather
        // than truncating onto tile zero.
        const auto MinCell = FIntPoint{FMath::Max(MinCorner.X, 0), FMath::Max(MinCorner.Y, 0)};
        const auto MaxCell = FIntPoint{
            FMath::Min(MaxCorner.X, (Params._Divisions.X * CellsPerTile) - 1),
            FMath::Min(MaxCorner.Y, (Params._Divisions.Y * CellsPerTile) - 1)};

        const auto RangeIsUsable = CellsPerTile > 0 && RadiusUu >= 0.0 &&
            MinCell.X <= MaxCell.X && MinCell.Y <= MaxCell.Y;

        auto Candidates = TArray<FPlateCandidate>{};
        auto PrefixSums = TArray<int32>{};

        if (RangeIsUsable)
        {
            const auto MinTile = FIntPoint{MinCell.X / CellsPerTile, MinCell.Y / CellsPerTile};
            const auto MaxTile = FIntPoint{MaxCell.X / CellsPerTile, MaxCell.Y / CellsPerTile};

            for (auto TileY = MinTile.Y; TileY <= MaxTile.Y; ++TileY)
            {
                for (auto TileX = MinTile.X; TileX <= MaxTile.X; ++TileX)
                {
                    const auto TileIndex = Get_TileIndex(Params._Divisions, FCk_GroundNav_TileCoord{TileX, TileY});

                    if (NOT InField._Tiles.IsValidIndex(TileIndex))
                    { continue; }

                    ++Result._Cost._TilesTouched;

                    const auto& Tile = InField._Tiles[TileIndex];

                    if (NOT Tile.Get_IsBuilt())
                    {
                        Result._Cost._TouchedUnbuiltTile = true;
                        continue;
                    }

                    const auto TileCornerX = TileX * CellsPerTile;
                    const auto TileCornerY = TileY * CellsPerTile;

                    const auto LocalMin = FIntPoint{
                        FMath::Clamp(MinCell.X - TileCornerX, 0, Tile._SizeX - 1),
                        FMath::Clamp(MinCell.Y - TileCornerY, 0, Tile._SizeY - 1)};
                    const auto LocalMax = FIntPoint{
                        FMath::Clamp(MaxCell.X - TileCornerX, 0, Tile._SizeX - 1),
                        FMath::Clamp(MaxCell.Y - TileCornerY, 0, Tile._SizeY - 1)};

                    for (auto PlateIndex = 0; PlateIndex < Tile._Plates._Plates.Num(); ++PlateIndex)
                    {
                        const auto& Plate = Tile._Plates._Plates[PlateIndex];

                        auto Candidate = FPlateCandidate{};
                        Candidate._TileIndex = TileIndex;
                        Candidate._PlateIndex = PlateIndex;
                        Candidate._LayerIndex = Plate._LayerIndex;
                        Candidate._Window = Make_Window(Plate, LocalMin, LocalMax);

                        DoAdd_Candidate(
                            Tile, Candidate, InQuery._Agent, Disc, Candidates, PrefixSums, Result._Cost);
                    }
                }
            }
        }

        if (Candidates.IsEmpty())
        {
            Result._Status = Get_EmptyStatus(Result._Cost);

            return Result;
        }

        auto Stream = FRandomStream{InQuery._Seed};

        Result._Points.Reserve(InQuery._Count);

        for (auto Index = 0; Index < InQuery._Count; ++Index)
        {
            ++Result._Attempts;

            auto Point = FCk_GroundNav_GeneratedPoint{};

            if (Get_DrawnPoint(InField, Candidates, PrefixSums, InQuery._Agent, Disc, Stream, Result._Cost, Point))
            { Result._Points.Add(Point); }
        }

        Result._Status = ECk_NavSurface_QueryStatus::Success;

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_RandomPointsByPathDistance(
            const FCk_GroundNav_Field&                   InField,
            const FCk_GroundNav_PathDistancePointsQuery& InQuery)
        -> FCk_GroundNav_PointsResult
    {
        using namespace pointsquery_private;

        auto Result = FCk_GroundNav_PointsResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        if (InQuery._Count <= 0)
        {
            Result._Status = ECk_NavSurface_QueryStatus::Success;

            return Result;
        }

        auto FloodQuery = FCk_GroundNav_FloodQuery{};
        FloodQuery._Source = InQuery._Origin;
        FloodQuery._VerticalToleranceUu = InQuery._VerticalToleranceUu;
        FloodQuery._Agent = InQuery._Agent;
        FloodQuery._MaxDistanceUu = InQuery._MaxDistanceUu;

        const auto Flood = Get_FloodFill(InField, FloodQuery);

        Result._Cost = Flood._Cost;

        if (NOT Flood.Get_IsSuccess())
        {
            Result._Status = Flood._Status;

            return Result;
        }

        // The whole of every reached plate is in play: what bounds this generator is the walked
        // distance, tested per drawn point below, and no horizontal reach at all.
        const auto Disc = FDiscFilter{};

        auto Candidates = TArray<FPlateCandidate>{};
        auto PrefixSums = TArray<int32>{};

        auto TileWasTouched = TBitArray<>{false, InField.Get_TileCount()};

        const auto FlatPlateCount = Get_FlatPlateCount(InField);

        for (auto FlatPlate = 0; FlatPlate < FlatPlateCount; ++FlatPlate)
        {
            if (NOT Flood.Get_IsPlateReached(FlatPlate))
            { continue; }

            auto TileIndex = int32{INDEX_NONE};
            auto PlateIndex = int32{INDEX_NONE};

            if (NOT Get_TileAndPlate(InField, FlatPlate, TileIndex, PlateIndex))
            { continue; }

            if (NOT InField._Tiles.IsValidIndex(TileIndex))
            { continue; }

            const auto& Tile = InField._Tiles[TileIndex];

            if (NOT Tile.Get_IsBuilt() || NOT Tile._Plates._Plates.IsValidIndex(PlateIndex))
            { continue; }

            if (TileWasTouched.IsValidIndex(TileIndex) && NOT TileWasTouched[TileIndex])
            {
                TileWasTouched[TileIndex] = true;
                ++Result._Cost._TilesTouched;
            }

            const auto& Plate = Tile._Plates._Plates[PlateIndex];

            auto Candidate = FPlateCandidate{};
            Candidate._TileIndex = TileIndex;
            Candidate._PlateIndex = PlateIndex;
            Candidate._LayerIndex = Plate._LayerIndex;
            Candidate._Window = Make_Window(Plate);

            DoAdd_Candidate(Tile, Candidate, InQuery._Agent, Disc, Candidates, PrefixSums, Result._Cost);
        }

        if (Candidates.IsEmpty())
        {
            Result._Status = Get_EmptyStatus(Result._Cost);

            return Result;
        }

        auto Stream = FRandomStream{InQuery._Seed};

        const auto MinDistanceUu = static_cast<double>(InQuery._MinDistanceUu);
        const auto MaxDistanceUu = static_cast<double>(InQuery._MaxDistanceUu);

        const auto MaxDraws = static_cast<int64>(kDrawsPerRequestedPoint) * static_cast<int64>(InQuery._Count);

        Result._Points.Reserve(InQuery._Count);

        for (auto Draw = int64{0}; Draw < MaxDraws && Result._Points.Num() < InQuery._Count; ++Draw)
        {
            ++Result._Attempts;

            auto Point = FCk_GroundNav_GeneratedPoint{};

            if (NOT Get_DrawnPoint(
                InField, Candidates, PrefixSums, InQuery._Agent, Disc, Stream, Result._Cost, Point))
            { continue; }

            const auto DistanceUu = Get_FloodDistanceTo(
                InField, Flood, Point._Location, InQuery._VerticalToleranceUu, InQuery._Agent);

            if (NOT DistanceUu.IsSet())
            { continue; }

            if (DistanceUu.GetValue() < MinDistanceUu || DistanceUu.GetValue() > MaxDistanceUu)
            { continue; }

            Result._Points.Add(Point);
        }

        Result._Status = ECk_NavSurface_QueryStatus::Success;

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_GridPoints(
            const FCk_GroundNav_Field&           InField,
            const FCk_GroundNav_GridPointsQuery& InQuery)
        -> FCk_GroundNav_PointsResult
    {
        using namespace pointsquery_private;

        auto Result = FCk_GroundNav_PointsResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        const auto SpacingUu = static_cast<double>(InQuery._SpacingUu);

        const auto BoundsAreUsable = InQuery._Bounds.IsValid != 0 && SpacingUu > 0.0 &&
            InQuery._Bounds.Min.X <= InQuery._Bounds.Max.X &&
            InQuery._Bounds.Min.Y <= InQuery._Bounds.Max.Y &&
            InQuery._Bounds.Min.Z <= InQuery._Bounds.Max.Z;

        if (NOT BoundsAreUsable)
        {
            Result._Status = ECk_NavSurface_QueryStatus::NoSurface;

            return Result;
        }

        const auto PhaseXY = InQuery._AlignToLattice == ECk_EnableDisable::Enable
            ? InField._Params._OriginXY
            : FVector2D{InQuery._Bounds.Min.X, InQuery._Bounds.Min.Y};

        const auto FirstIndexX = static_cast<int64>(
            FMath::CeilToDouble((InQuery._Bounds.Min.X - PhaseXY.X) / SpacingUu));
        const auto FirstIndexY = static_cast<int64>(
            FMath::CeilToDouble((InQuery._Bounds.Min.Y - PhaseXY.Y) / SpacingUu));

        auto TileWasTouched = TBitArray<>{false, InField.Get_TileCount()};

        auto Addresses = TArray<FCk_GroundNav_CellAddress, TInlineAllocator<4>>{};

        // A lattice position on a cell line stands on the closed squares either side of it, and those
        // are the same ground on the same storey: one point per storey, or the lattice doubles itself
        // on every cell line it happens to land on.
        auto EmittedLayers = TArray<int32, TInlineAllocator<8>>{};

        for (auto IndexY = FirstIndexY; ; ++IndexY)
        {
            const auto PositionY = PhaseXY.Y + (static_cast<double>(IndexY) * SpacingUu);

            if (PositionY > InQuery._Bounds.Max.Y)
            { break; }

            for (auto IndexX = FirstIndexX; ; ++IndexX)
            {
                const auto PositionX = PhaseXY.X + (static_cast<double>(IndexX) * SpacingUu);

                if (PositionX > InQuery._Bounds.Max.X)
                { break; }

                Get_CellAddressesAt(InField, FVector2D{PositionX, PositionY}, Addresses);

                EmittedLayers.Reset();

                for (const auto& Address : Addresses)
                {
                    if (TileWasTouched.IsValidIndex(Address._TileIndex) && NOT TileWasTouched[Address._TileIndex])
                    {
                        TileWasTouched[Address._TileIndex] = true;
                        ++Result._Cost._TilesTouched;
                    }

                    if (Get_TileStatus(InField, Address._TileIndex) != ECk_GroundNav_BuildStatus::Built)
                    {
                        Result._Cost._TouchedUnbuiltTile = true;
                        continue;
                    }

                    const auto& Tile = InField._Tiles[Address._TileIndex];

                    for (auto Layer = 0; Layer < Tile._LayerCount; ++Layer)
                    {
                        if (EmittedLayers.Contains(Layer))
                        { continue; }

                        ++Result._Cost._CellsRead;

                        auto Surface = FCk_GroundNav_SurfaceRef{};
                        auto SurfaceZUu = 0.0f;
                        auto ClearanceUu = 0.0f;

                        if (NOT Get_SurfaceAt(InField, Address, Layer, Surface, SurfaceZUu, ClearanceUu))
                        { continue; }

                        if (NOT Get_IsAdmitted(ClearanceUu, InQuery._Agent))
                        { continue; }

                        const auto SurfaceZ = static_cast<double>(SurfaceZUu);

                        if (SurfaceZ < InQuery._Bounds.Min.Z || SurfaceZ > InQuery._Bounds.Max.Z)
                        { continue; }

                        EmittedLayers.Add(Layer);

                        auto Point = FCk_GroundNav_GeneratedPoint{};
                        Point._Location = FVector{PositionX, PositionY, SurfaceZ};
                        Point._Surface = Surface;

                        Result._Points.Add(Point);
                    }
                }
            }
        }

        Result._Status = Result._Points.IsEmpty()
            ? Get_EmptyStatus(Result._Cost)
            : ECk_NavSurface_QueryStatus::Success;

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
