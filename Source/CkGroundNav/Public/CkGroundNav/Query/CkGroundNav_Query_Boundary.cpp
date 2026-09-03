#include "CkGroundNav_Query_Boundary.h"

#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace boundaryquery_private
    {
        /** What a run is tested against: the point's own component, its height, and its reach. */
        struct FBoundaryFilter
        {
            int32 _Label = INDEX_NONE;

            FVector2D _QueryXY = FVector2D::ZeroVector;

            double _CentreZUu = 0.0;
            double _VerticalWindowUu = 0.0;
            double _RadiusUu = 0.0;
        };

        struct FBoundaryHit
        {
            bool _Qualifies = false;

            double _DistanceUu = 0.0;
            double _T = 0.0;

            FVector2D _ClosestXY = FVector2D::ZeroVector;
        };

        struct FBoundaryCandidate
        {
            double _DistanceUu = 0.0;

            int32 _TileIndex = INDEX_NONE;

            FCk_GroundNav_BoundarySegment _Segment;
        };

        // ------------------------------------------------------------------------------------------------------------

        /**
         * The point of a run's XY projection nearest to a world XY, and where along the run it lies.
         *
         * A run of zero XY length answers its own start at parameter zero rather than dividing by it:
         * a NaN parameter would pass every comparison downstream instead of failing one.
         */
        auto Get_ClosestPointOnSegmentXY(
            const FCk_GroundNav_BoundarySegment& InSegment,
            const FVector2D&                     InPointXY,
            double&                              OutT) -> FVector2D
        {
            const auto StartXY = FVector2D{InSegment._Start.X, InSegment._Start.Y};
            const auto EndXY = FVector2D{InSegment._End.X, InSegment._End.Y};

            const auto Delta = EndXY - StartXY;
            const auto LengthSquared = Delta.SizeSquared();

            OutT = LengthSquared > 0.0
                ? FMath::Clamp(FVector2D::DotProduct(InPointXY - StartXY, Delta) / LengthSquared, 0.0, 1.0)
                : 0.0;

            return StartXY + (Delta * OutT);
        }

        /**
         * Whether one run belongs in the answer, and where on it the point is nearest. A run qualifies
         * on three counts: the point's own reachability component, an end inside the vertical window,
         * and the radius in XY. Bills one probe whatever the verdict.
         */
        auto Get_BoundaryHit(
            const FCk_GroundNav_Field&           InField,
            int32                                InTileIndex,
            const FCk_GroundNav_BoundarySegment& InSegment,
            const FBoundaryFilter&               InFilter,
            FCk_GroundNav_QueryCost&             InOutCost) -> FBoundaryHit
        {
            ++InOutCost._CellsRead;

            auto Hit = FBoundaryHit{};

            if (InField.Get_ReachabilityLabel(InTileIndex, InSegment._PlateIndex) != InFilter._Label)
            { return Hit; }

            const auto StartIsInWindow =
                FMath::Abs(InSegment._Start.Z - InFilter._CentreZUu) <= InFilter._VerticalWindowUu;
            const auto EndIsInWindow =
                FMath::Abs(InSegment._End.Z - InFilter._CentreZUu) <= InFilter._VerticalWindowUu;

            if (NOT StartIsInWindow && NOT EndIsInWindow)
            { return Hit; }

            Hit._ClosestXY = Get_ClosestPointOnSegmentXY(InSegment, InFilter._QueryXY, Hit._T);
            Hit._DistanceUu = FVector2D::Distance(Hit._ClosestXY, InFilter._QueryXY);
            Hit._Qualifies = Hit._DistanceUu <= InFilter._RadiusUu;

            return Hit;
        }

        /** The point resolved onto the surface it stands on. Radius zero: the body is already there. */
        auto Get_Centre(
            const FCk_GroundNav_Field& InField,
            const FVector&             InLocation,
            float                      InVerticalWindowUu) -> FCk_GroundNav_IsNavigableResult
        {
            auto CentreQuery = FCk_GroundNav_IsNavigableQuery{};
            CentreQuery._Location = InLocation;
            CentreQuery._VerticalToleranceUu = InVerticalWindowUu;

            return Get_IsNavigable(InField, CentreQuery);
        }

        auto Make_Filter(
            const FCk_GroundNav_Field&             InField,
            const FCk_GroundNav_IsNavigableResult& InCentre,
            const FVector&                         InLocation,
            float                                  InVerticalWindowUu,
            float                                  InRadiusUu) -> FBoundaryFilter
        {
            auto Filter = FBoundaryFilter{};

            Filter._Label = InField.Get_ReachabilityLabel(InCentre._Surface._TileIndex, InCentre._Surface._PlateIndex);
            Filter._QueryXY = FVector2D{InLocation.X, InLocation.Y};
            Filter._CentreZUu = static_cast<double>(InCentre._SurfaceZUu);
            Filter._VerticalWindowUu = static_cast<double>(InVerticalWindowUu);
            Filter._RadiusUu = static_cast<double>(InRadiusUu);

            return Filter;
        }

        // The buckets at Chebyshev distance exactly InRing, in Y-then-X ascending order and each one once.
        auto DoCollect_RingBuckets(
            const FIntPoint&   InCentre,
            int32              InRing,
            TArray<FIntPoint>& OutBuckets) -> void
        {
            OutBuckets.Reset();

            if (InRing <= 0)
            {
                OutBuckets.Add(InCentre);
                return;
            }

            for (auto OffsetY = -InRing; OffsetY <= InRing; ++OffsetY)
            {
                const auto IsEdgeRow = OffsetY == -InRing || OffsetY == InRing;

                for (auto OffsetX = -InRing; OffsetX <= InRing; ++OffsetX)
                {
                    if (NOT IsEdgeRow && OffsetX != -InRing && OffsetX != InRing)
                    { continue; }

                    OutBuckets.Add(FIntPoint{InCentre.X + OffsetX, InCentre.Y + OffsetY});
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_BoundarySegments(
            const FCk_GroundNav_Field&             InField,
            const FCk_GroundNav_BoundaryQuery&     InQuery,
            TArray<FCk_GroundNav_BoundarySegment>& OutSegments)
        -> ECk_NavSurface_QueryStatus
    {
        using namespace boundaryquery_private;

        OutSegments.Reset();

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        { return ECk_NavSurface_QueryStatus::Blocked; }

        const auto Centre = Get_Centre(InField, InQuery._Location, InQuery._VerticalWindowUu);

        if (NOT Centre.Get_IsSuccess())
        { return Centre._Status; }

        const auto& Params = InField._Params;

        const auto Filter = Make_Filter(
            InField, Centre, InQuery._Location, InQuery._VerticalWindowUu, InQuery._RadiusUu);

        const auto CellsPerTile = Get_CellsPerTile(Params);

        const auto MinCorner = Get_FieldCellAt(
            Params, FVector2D{Filter._QueryXY.X - Filter._RadiusUu, Filter._QueryXY.Y - Filter._RadiusUu});
        const auto MaxCorner = Get_FieldCellAt(
            Params, FVector2D{Filter._QueryXY.X + Filter._RadiusUu, Filter._QueryXY.Y + Filter._RadiusUu});

        // Clamped into the field's own cell range BEFORE any division, so the tile range below is a
        // floor division of non-negative numbers, and a circle wholly off the field is empty here
        // rather than truncating onto tile zero.
        const auto MinCell = FIntPoint{FMath::Max(MinCorner.X, 0), FMath::Max(MinCorner.Y, 0)};
        const auto MaxCell = FIntPoint{
            FMath::Min(MaxCorner.X, (Params._Divisions.X * CellsPerTile) - 1),
            FMath::Min(MaxCorner.Y, (Params._Divisions.Y * CellsPerTile) - 1)};

        const auto RangeIsUsable = CellsPerTile > 0 && MinCell.X <= MaxCell.X && MinCell.Y <= MaxCell.Y;

        auto Candidates = TArray<FBoundaryCandidate>{};

        // This query answers a status and a set, not a cost, so the shared filter bills into a sink.
        auto DiscardedCost = FCk_GroundNav_QueryCost{};

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

                    const auto& Tile = InField._Tiles[TileIndex];

                    if (NOT Tile.Get_IsBuilt())
                    { continue; }

                    const auto DoConsider = [&](const FCk_GroundNav_BoundarySegment& InSegment) -> void
                    {
                        const auto Hit = Get_BoundaryHit(InField, TileIndex, InSegment, Filter, DiscardedCost);

                        if (NOT Hit._Qualifies)
                        { return; }

                        auto Candidate = FBoundaryCandidate{};
                        Candidate._DistanceUu = Hit._DistanceUu;
                        Candidate._TileIndex = TileIndex;
                        Candidate._Segment = InSegment;

                        Candidates.Add(Candidate);
                    };

                    const auto TileCornerX = TileX * CellsPerTile;
                    const auto TileCornerY = TileY * CellsPerTile;

                    const auto LocalMinX = FMath::Clamp(MinCell.X - TileCornerX, 0, Tile._SizeX - 1);
                    const auto LocalMaxX = FMath::Clamp(MaxCell.X - TileCornerX, 0, Tile._SizeX - 1);
                    const auto LocalMinY = FMath::Clamp(MinCell.Y - TileCornerY, 0, Tile._SizeY - 1);
                    const auto LocalMaxY = FMath::Clamp(MaxCell.Y - TileCornerY, 0, Tile._SizeY - 1);

                    const auto& Boundary = Tile._Boundary;

                    // A run is listed under every bucket its cells touch, so the same index arrives many
                    // times over a range of buckets and the mask is what keeps the scan linear in runs.
                    auto Visited = TBitArray<>{false, Boundary._Segments.Num()};

                    const auto MinBucketX = LocalMinX / FCk_GroundNav_BoundaryField::kBucketCells;
                    const auto MaxBucketX = LocalMaxX / FCk_GroundNav_BoundaryField::kBucketCells;
                    const auto MinBucketY = LocalMinY / FCk_GroundNav_BoundaryField::kBucketCells;
                    const auto MaxBucketY = LocalMaxY / FCk_GroundNav_BoundaryField::kBucketCells;

                    for (auto BucketY = MinBucketY; BucketY <= MaxBucketY; ++BucketY)
                    {
                        for (auto BucketX = MinBucketX; BucketX <= MaxBucketX; ++BucketX)
                        {
                            for (const auto SegmentIndex : Boundary.Get_Bucket(FIntPoint{BucketX, BucketY}))
                            {
                                if (NOT Visited.IsValidIndex(SegmentIndex) || Visited[SegmentIndex])
                                { continue; }

                                Visited[SegmentIndex] = true;

                                DoConsider(Boundary._Segments[SegmentIndex]);
                            }
                        }
                    }

                    // The rim runs live on the field rather than in the tile's own index, because only
                    // the neighbour says whether a rim edge is a wall.
                    for (const auto& EdgeSegment : InField.Get_TileEdgeBoundary(TileIndex))
                    { DoConsider(EdgeSegment); }
                }
            }
        }

        Candidates.StableSort([](const FBoundaryCandidate& InLeft, const FBoundaryCandidate& InRight) -> bool
        {
            if (InLeft._DistanceUu != InRight._DistanceUu)
            { return InLeft._DistanceUu < InRight._DistanceUu; }

            return InLeft._TileIndex < InRight._TileIndex;
        });

        const auto EmitCount = InQuery._MaxSegments > 0
            ? FMath::Min(InQuery._MaxSegments, Candidates.Num())
            : Candidates.Num();

        OutSegments.Reserve(EmitCount);

        for (auto Index = 0; Index < EmitCount; ++Index)
        { OutSegments.Add(Candidates[Index]._Segment); }

        return ECk_NavSurface_QueryStatus::Success;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ClosestBoundary(
            const FCk_GroundNav_Field&                InField,
            const FCk_GroundNav_ClosestBoundaryQuery& InQuery)
        -> FCk_GroundNav_ClosestBoundaryResult
    {
        using namespace boundaryquery_private;

        auto Result = FCk_GroundNav_ClosestBoundaryResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        const auto Centre = Get_Centre(InField, InQuery._Location, InQuery._VerticalWindowUu);

        Result._Cost = Centre._Cost;

        if (NOT Centre.Get_IsSuccess())
        {
            Result._Status = Centre._Status;

            return Result;
        }

        const auto& Params = InField._Params;

        const auto Filter = Make_Filter(
            InField, Centre, InQuery._Location, InQuery._VerticalWindowUu, InQuery._MaxRadiusUu);

        const auto CellsPerTile = Get_CellsPerTile(Params);
        const auto BucketsPerTile = FMath::DivideAndRoundUp(CellsPerTile, FCk_GroundNav_BoundaryField::kBucketCells);
        const auto BucketSpanUu =
            static_cast<double>(FCk_GroundNav_BoundaryField::kBucketCells) *
            static_cast<double>(Params._Config.Get_CellSizeUu());

        // Every ring's reach is a multiple of this span, so a field without one has no termination.
        const auto LatticeIsUsable = BucketsPerTile > 0 && BucketSpanUu > 0.0;
        CK_ENSURE_IF_NOT(LatticeIsUsable,
            TEXT("GroundNav field resolved a surface but has no usable cell lattice: cells per tile [{}], cell size [{}]"),
            CellsPerTile, Params._Config.Get_CellSizeUu())
        {
            Result._Status = ECk_NavSurface_QueryStatus::NoSurface;

            return Result;
        }

        const auto& CentreTile = InField._Tiles[Centre._Surface._TileIndex];

        const auto CentreBucket = FIntPoint{
            (CentreTile._Coord._X * BucketsPerTile) +
                (Centre._Surface._CellX / FCk_GroundNav_BoundaryField::kBucketCells),
            (CentreTile._Coord._Y * BucketsPerTile) +
                (Centre._Surface._CellY / FCk_GroundNav_BoundaryField::kBucketCells)};

        auto TileWasTouched = TBitArray<>{false, InField.Get_TileCount()};

        auto VisitedByTile = TArray<TBitArray<>>{};
        VisitedByTile.SetNum(InField.Get_TileCount());

        auto RingBuckets = TArray<FIntPoint>{};

        auto Found = false;
        auto BestDistanceUu = 0.0;
        auto BestT = 0.0;
        auto BestClosestXY = FVector2D::ZeroVector;
        auto BestSegment = FCk_GroundNav_BoundarySegment{};

        for (auto Ring = 0; ; ++Ring)
        {
            DoCollect_RingBuckets(CentreBucket, Ring, RingBuckets);

            for (const auto& GlobalBucket : RingBuckets)
            {
                if (GlobalBucket.X < 0 || GlobalBucket.Y < 0)
                { continue; }

                const auto Coord = FCk_GroundNav_TileCoord{
                    GlobalBucket.X / BucketsPerTile,
                    GlobalBucket.Y / BucketsPerTile};

                if (Coord._X >= Params._Divisions.X || Coord._Y >= Params._Divisions.Y)
                { continue; }

                const auto TileIndex = Get_TileIndex(Params._Divisions, Coord);

                if (NOT InField._Tiles.IsValidIndex(TileIndex))
                { continue; }

                const auto& Tile = InField._Tiles[TileIndex];

                if (NOT Tile.Get_IsBuilt())
                {
                    Result._Cost._TouchedUnbuiltTile = true;
                    continue;
                }

                const auto DoConsider = [&](const FCk_GroundNav_BoundarySegment& InSegment) -> void
                {
                    const auto Hit = Get_BoundaryHit(InField, TileIndex, InSegment, Filter, Result._Cost);

                    if (NOT Hit._Qualifies || (Found && Hit._DistanceUu >= BestDistanceUu))
                    { return; }

                    Found = true;
                    BestDistanceUu = Hit._DistanceUu;
                    BestT = Hit._T;
                    BestClosestXY = Hit._ClosestXY;
                    BestSegment = InSegment;
                };

                if (NOT TileWasTouched[TileIndex])
                {
                    TileWasTouched[TileIndex] = true;
                    ++Result._Cost._TilesTouched;

                    VisitedByTile[TileIndex].Init(false, Tile._Boundary._Segments.Num());

                    // The rim runs are not in the bucket index, so they are read once with the tile.
                    for (const auto& EdgeSegment : InField.Get_TileEdgeBoundary(TileIndex))
                    { DoConsider(EdgeSegment); }
                }

                const auto LocalBucket = FIntPoint{
                    GlobalBucket.X % BucketsPerTile,
                    GlobalBucket.Y % BucketsPerTile};

                auto& Visited = VisitedByTile[TileIndex];

                for (const auto SegmentIndex : Tile._Boundary.Get_Bucket(LocalBucket))
                {
                    if (NOT Visited.IsValidIndex(SegmentIndex) || Visited[SegmentIndex])
                    { continue; }

                    Visited[SegmentIndex] = true;

                    DoConsider(Tile._Boundary._Segments[SegmentIndex]);
                }
            }

            // Nothing in the next ring can sit closer than this ring's buckets already reach.
            const auto RingReachUu = static_cast<double>(Ring) * BucketSpanUu;

            if (Found && BestDistanceUu <= RingReachUu)
            { break; }

            if (RingReachUu > Filter._RadiusUu)
            { break; }
        }

        if (NOT Found)
        {
            Result._Status = ECk_NavSurface_QueryStatus::NoSurface;

            return Result;
        }

        Result._Status = ECk_NavSurface_QueryStatus::Success;
        Result._Segment = BestSegment;
        Result._ClosestPoint = FVector{
            BestClosestXY.X,
            BestClosestXY.Y,
            FMath::Lerp(BestSegment._Start.Z, BestSegment._End.Z, BestT)};
        Result._DistanceUu = static_cast<float>(BestDistanceUu);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
