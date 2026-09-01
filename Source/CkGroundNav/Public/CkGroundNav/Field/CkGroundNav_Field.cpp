#include "CkGroundNav_Field.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        FCk_GroundNav_FieldParams::
        Get_IsValid() const
        -> bool
    {
        return _Config.Get_IsValid() &&
               _Divisions.X > 0 && _Divisions.Y > 0 &&
               _MaxZUu > _MinZUu &&
               _MaxClearanceUu >= 0.0f;
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_TileSpanUu() const
        -> double
    {
        const auto CellSize = static_cast<double>(_Config.Get_CellSizeUu());

        if (CellSize <= 0.0)
        { return 0.0; }

        const auto Cells = FMath::Max(1, FMath::CeilToInt32(_Config.Get_TileSizeUu() / CellSize));

        return static_cast<double>(Cells) * CellSize;
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_Bounds() const
        -> FBox
    {
        const auto SpanUu = Get_TileSpanUu();

        return FBox{
            FVector{_OriginXY.X, _OriginXY.Y, static_cast<double>(_MinZUu)},
            FVector{
                _OriginXY.X + (SpanUu * _Divisions.X),
                _OriginXY.Y + (SpanUu * _Divisions.Y),
                static_cast<double>(_MaxZUu)}};
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_TileBakeParams(
            const FCk_GroundNav_TileCoord& InCoord,
            const FCk_GroundNav_Epoch&     InEpoch) const
        -> FCk_GroundNav_TileBakeParams
    {
        auto Params = FCk_GroundNav_TileBakeParams{};

        Params._Coord = InCoord;
        Params._Epoch = InEpoch;
        Params._FieldOriginXY = _OriginXY;
        Params._MinZUu = _MinZUu;
        Params._MaxZUu = _MaxZUu;
        Params._Config = _Config;
        Params._Profile = _Profile;
        Params._MergeTunables = _MergeTunables;
        Params._MaxClearanceUu = _MaxClearanceUu;

        return Params;
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_TileCoordAt(
            const FVector& InWorldPosition) const
        -> FCk_GroundNav_TileCoord
    {
        const auto SpanUu = Get_TileSpanUu();

        if (SpanUu <= 0.0)
        { return FCk_GroundNav_TileCoord{INDEX_NONE, INDEX_NONE}; }

        return FCk_GroundNav_TileCoord{
            FMath::FloorToInt32((InWorldPosition.X - _OriginXY.X) / SpanUu),
            FMath::FloorToInt32((InWorldPosition.Y - _OriginXY.Y) / SpanUu)};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_Field::
        Get_Tile(
            const FCk_GroundNav_TileCoord& InCoord) const
        -> const FCk_GroundNav_Tile*
    {
        const auto TileIndex = Get_TileIndex(_Params._Divisions, InCoord);

        return _Tiles.IsValidIndex(TileIndex) ? &_Tiles[TileIndex] : nullptr;
    }

    auto
        FCk_GroundNav_Field::
        Get_TileAt(
            const FVector& InWorldPosition) const
        -> const FCk_GroundNav_Tile*
    {
        return Get_Tile(_Params.Get_TileCoordAt(InWorldPosition));
    }

    auto
        FCk_GroundNav_Field::
        Get_BuiltTileCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& Tile : _Tiles)
        {
            if (Tile.Get_IsBuilt())
            { ++Count; }
        }

        return Count;
    }

    auto
        FCk_GroundNav_Field::
        Get_AggregatedTileEpochSum() const
        -> int64
    {
        auto Sum = int64{0};

        for (const auto& Tile : _Tiles)
        { Sum += Tile._Epoch._Value; }

        return Sum;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_FieldPublisher::
        Request_Publish(
            const TSharedRef<const FCk_GroundNav_Field>& InField)
        -> void
    {
        _Published = InField;
        _Epoch = InField->_Epoch;
        _Status = ECk_GroundNav_BuildStatus::Built;
    }

    auto
        FCk_GroundNav_FieldPublisher::
        Request_RecordFailure()
        -> void
    {
        _Status = ECk_GroundNav_BuildStatus::Failed;
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace field_private
    {
        // Only the two positive directions are composed. Every seam is shared by exactly one +X or +Y
        // step, so walking all four would emit each crossing twice under two names.
        constexpr int32 kPositiveDirectionCount = 2;

        /** One matched pair of stubs, before neighbouring pairs are run together. */
        struct FSeamCrossing
        {
            int32 _TileIndexA = INDEX_NONE;
            int32 _TileIndexB = INDEX_NONE;
            int32 _PlateA = FCk_GroundNav_Plate::kNoPlate;
            int32 _PlateB = FCk_GroundNav_Plate::kNoPlate;
            int32 _Direction = 0;
            int32 _Along = 0;

            float _ClearanceUu = 0.0f;
            float _MidZUu = 0.0f;
        };

        auto Get_IsSameRun(
            const FSeamCrossing& InPrevious,
            const FSeamCrossing& InCurrent) -> bool
        {
            return InCurrent._TileIndexA == InPrevious._TileIndexA &&
                   InCurrent._TileIndexB == InPrevious._TileIndexB &&
                   InCurrent._PlateA     == InPrevious._PlateA     &&
                   InCurrent._PlateB     == InPrevious._PlateB     &&
                   InCurrent._Direction  == InPrevious._Direction  &&
                   InCurrent._Along      == InPrevious._Along + 1;
        }

        /**
         * The neighbour's account of the same crossing, or nothing.
         *
         * Matching on BOTH surfaces is what makes this exact rather than plausible: this tile's far
         * surface must be the one the neighbour stands on, and the neighbour's far surface must be the
         * one this tile stands on. Two floors stacked over the same seam cell would otherwise be
         * indistinguishable.
         */
        auto Get_MatchingStub(
            const FCk_GroundNav_Tile&     InNeighbour,
            const FCk_GroundNav_SeamStub& InStub,
            int32                         InOppositeDirection) -> const FCk_GroundNav_SeamStub*
        {
            for (const auto& Candidate : InNeighbour._SeamStubs)
            {
                if (Candidate._Direction != InOppositeDirection ||
                    Candidate._AlongIndex != InStub._AlongIndex)
                { continue; }

                if (Candidate._NearSurfaceZUu != InStub._FarSurfaceZUu ||
                    Candidate._FarSurfaceZUu != InStub._NearSurfaceZUu)
                { continue; }

                return &Candidate;
            }

            return nullptr;
        }
    }

    auto
        DoDerive_SeamPortals(
            FCk_GroundNav_Field& InOutField)
        -> void
    {
        using namespace field_private;

        InOutField._SeamPortals.Reset();

        auto Crossings = TArray<FSeamCrossing>{};

        for (auto TileIndexA = 0; TileIndexA < InOutField._Tiles.Num(); ++TileIndexA)
        {
            const auto& TileA = InOutField._Tiles[TileIndexA];

            if (NOT TileA.Get_IsBuilt())
            { continue; }

            for (auto Direction = 0; Direction < kPositiveDirectionCount; ++Direction)
            {
                const auto Offset = Get_DirectionOffset(Direction);
                const auto NeighbourCoord = FCk_GroundNav_TileCoord{
                    TileA._Coord._X + Offset.X, TileA._Coord._Y + Offset.Y};

                const auto TileIndexB = Get_TileIndex(InOutField._Params._Divisions, NeighbourCoord);

                if (NOT InOutField._Tiles.IsValidIndex(TileIndexB))
                { continue; }

                const auto& TileB = InOutField._Tiles[TileIndexB];

                // An unbuilt neighbour is a place nothing is known about, not a wall. No portal is
                // emitted and the boundary reads as unbuilt to whatever tries to cross it.
                if (NOT TileB.Get_IsBuilt())
                { continue; }

                const auto Opposite = Get_OppositeDirection(Direction);

                for (const auto& Stub : TileA._SeamStubs)
                {
                    if (Stub._Direction != Direction)
                    { continue; }

                    const auto* Match = Get_MatchingStub(TileB, Stub, Opposite);

                    if (Match == nullptr)
                    { continue; }

                    auto Crossing = FSeamCrossing{};

                    Crossing._TileIndexA = TileIndexA;
                    Crossing._TileIndexB = TileIndexB;
                    Crossing._PlateA = Stub._PlateIndex;
                    Crossing._PlateB = Match->_PlateIndex;
                    Crossing._Direction = Direction;
                    Crossing._Along = Stub._AlongIndex;
                    Crossing._ClearanceUu = FMath::Min(Stub._ClearanceUu, Match->_ClearanceUu);
                    Crossing._MidZUu = 0.5f * (Stub._NearSurfaceZUu + Stub._FarSurfaceZUu);

                    Crossings.Emplace(Crossing);
                }
            }
        }

        Crossings.Sort([](const FSeamCrossing& InLeft, const FSeamCrossing& InRight) -> bool
        {
            if (InLeft._TileIndexA != InRight._TileIndexA) { return InLeft._TileIndexA < InRight._TileIndexA; }
            if (InLeft._TileIndexB != InRight._TileIndexB) { return InLeft._TileIndexB < InRight._TileIndexB; }
            if (InLeft._Direction  != InRight._Direction)  { return InLeft._Direction  < InRight._Direction; }
            if (InLeft._PlateA     != InRight._PlateA)     { return InLeft._PlateA     < InRight._PlateA; }
            if (InLeft._PlateB     != InRight._PlateB)     { return InLeft._PlateB     < InRight._PlateB; }

            return InLeft._Along < InRight._Along;
        });

        for (auto Index = 0; Index < Crossings.Num(); ++Index)
        {
            const auto& Crossing = Crossings[Index];

            if (Index > 0 && Get_IsSameRun(Crossings[Index - 1], Crossing))
            {
                auto& Portal = InOutField._SeamPortals.Last();

                Portal._AlongMax = Crossing._Along;
                Portal._MaxEndZUu = Crossing._MidZUu;
                Portal._TraversalClearanceUu = FMath::Max(
                    Portal._TraversalClearanceUu, Crossing._ClearanceUu);

                continue;
            }

            auto Portal = FCk_GroundNav_SeamPortal{};

            Portal._TileIndexA = Crossing._TileIndexA;
            Portal._TileIndexB = Crossing._TileIndexB;
            Portal._PlateA = Crossing._PlateA;
            Portal._PlateB = Crossing._PlateB;
            Portal._Direction = Crossing._Direction;
            Portal._AlongMin = Crossing._Along;
            Portal._AlongMax = Crossing._Along;
            Portal._MinEndZUu = Crossing._MidZUu;
            Portal._MaxEndZUu = Crossing._MidZUu;
            Portal._TraversalClearanceUu = Crossing._ClearanceUu;

            InOutField._SeamPortals.Emplace(Portal);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoBake_Field(
            const ICk_GroundNav_GeometryBackend& InBackend,
            const FCk_GroundNav_FieldParams&     InParams,
            const FCk_GroundNav_Epoch&           InEpoch,
            FCk_GroundNav_Field&                 OutField)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        OutField = FCk_GroundNav_Field{};

        if (NOT InParams.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        if (NOT InBackend.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::BackendUnavailable);
            return Result;
        }

        OutField._Params = InParams;
        OutField._Epoch = InEpoch;
        OutField._Tiles.SetNum(InParams.Get_TileCount());

        auto ProbesSpent = 0;
        auto DroppedInputCount = 0;

        auto Geometry = FCk_GroundNav_GeometryBatch{};

        for (auto TileIndex = 0; TileIndex < OutField._Tiles.Num(); ++TileIndex)
        {
            const auto Coord = Get_TileCoord(InParams._Divisions, TileIndex);
            const auto TileParams = InParams.Get_TileBakeParams(Coord, InEpoch);

            // The HALO bounds, not the tile's own. A tile handed only its own geometry reads short at
            // every edge, and the assembled field claims a pinch at every seam.
            Geometry.Reset();
            InBackend.Get_TrianglesInBounds(Get_TileHaloBounds(TileParams), Geometry);

            const auto TileResult = DoBake_Tile(Geometry, TileParams, OutField._Tiles[TileIndex]);

            ProbesSpent += TileResult.Get_ProbesSpent();
            DroppedInputCount += TileResult.Get_DroppedInputCount();
        }

        DoDerive_SeamPortals(OutField);

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);
        Result.Set_DroppedInputCount(DroppedInputCount);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
