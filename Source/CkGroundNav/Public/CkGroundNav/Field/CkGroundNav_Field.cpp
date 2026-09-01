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

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);
        Result.Set_DroppedInputCount(DroppedInputCount);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
