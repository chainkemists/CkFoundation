#include "CkGroundNav_FieldBuild.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Request_BeginBuild(
            const FCk_GroundNav_FieldParams& InParams,
            const FCk_GroundNav_Epoch&       InEpoch,
            FCk_GroundNav_FieldBuildState&   OutState)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        OutState = FCk_GroundNav_FieldBuildState{};
        OutState._Params = InParams;
        OutState._Epoch = InEpoch;

        if (NOT InParams.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        OutState._Partial._Params = InParams;
        OutState._Partial._Epoch = InEpoch;
        OutState._Partial._Tiles.SetNum(InParams.Get_TileCount());

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        return Result;
    }

    auto
        Request_AdvanceBuild(
            const ICk_GroundNav_GeometryBackend& InBackend,
            int32                                InProbeBudget,
            FCk_GroundNav_FieldBuildState&       InOutState)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        if (NOT InOutState._Params.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        if (NOT InBackend.Get_IsValid())
        {
            InOutState._Status = ECk_GroundNav_BuildStatus::Failed;

            Result.Set_Status(ECk_GroundNav_BakeStatus::BackendUnavailable);
            return Result;
        }

        const auto TileCount = InOutState._Partial.Get_TileCount();

        auto SpentThisSlice = 0;
        auto DroppedThisSlice = 0;
        auto Geometry = FCk_GroundNav_GeometryBatch{};

        while (InOutState._NextTileIndex < TileCount)
        {
            // Checked AFTER the first tile of the slice rather than before it, so a budget smaller than
            // any single tile still advances the build instead of spinning forever on a resume point it
            // can never get past.
            if (SpentThisSlice > 0 && SpentThisSlice >= InProbeBudget)
            { break; }

            const auto TileIndex = InOutState._NextTileIndex;
            const auto Coord = Get_TileCoord(InOutState._Params._Divisions, TileIndex);
            const auto TileParams = InOutState._Params.Get_TileBakeParams(Coord, InOutState._Epoch);

            Geometry.Reset();
            InBackend.Get_TrianglesInBounds(Get_TileHaloBounds(TileParams), Geometry);

            const auto TileResult = DoBake_Tile(
                Geometry, TileParams, InOutState._Partial._Tiles[TileIndex]);

            SpentThisSlice += TileResult.Get_ProbesSpent();
            DroppedThisSlice += TileResult.Get_DroppedInputCount();

            ++InOutState._NextTileIndex;
        }

        InOutState._ProbesSpent += SpentThisSlice;

        Result.Set_ProbesSpent(SpentThisSlice);
        Result.Set_DroppedInputCount(DroppedThisSlice);

        if (InOutState._NextTileIndex < TileCount)
        {
            // Paused with a resume point recorded. Nothing is published: the field is not reachable
            // until it is whole.
            Result.Set_Status(ECk_GroundNav_BakeStatus::BudgetExhausted);
            return Result;
        }

        // Both are derived from the finished tiles, so they belong to the end of the build and not to
        // whichever slice happened to bake the last tile.
        DoDerive_SeamPortals(InOutState._Partial);
        DoLabel_Reachability(InOutState._Partial);

        InOutState._Status = ECk_GroundNav_BuildStatus::Built;

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        return Result;
    }

    auto
        Get_CompletedField(
            const FCk_GroundNav_FieldBuildState& InState)
        -> const FCk_GroundNav_Field*
    {
        return InState.Get_IsComplete() ? &InState._Partial : nullptr;
    }

    auto
        Request_ReleaseCompletedField(
            FCk_GroundNav_FieldBuildState& InOutState)
        -> FCk_GroundNav_FieldPtr
    {
        if (NOT InOutState.Get_IsComplete())
        { return {}; }

        return MakeShared<const FCk_GroundNav_Field>(MoveTemp(InOutState._Partial));
    }
}

// --------------------------------------------------------------------------------------------------------------------
