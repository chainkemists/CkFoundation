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

        // The build's claim to be a statement about ONE world: captured on the first slice, re-read on
        // every later one AND before every tile's geometry fetch. See _GeometryRevision's contract — a
        // mismatch fails the whole build rather than baking a tile the rest of the field disagrees with.
        if (NOT InOutState._HasGeometryRevision)
        {
            InOutState._GeometryRevision = InBackend.Get_WorldRevision();
            InOutState._HasGeometryRevision = true;
        }
        else if (InBackend.Get_WorldRevision() != InOutState._GeometryRevision)
        {
            InOutState._Status = ECk_GroundNav_BuildStatus::Failed;

            Result.Set_Status(ECk_GroundNav_BakeStatus::StaleGeometry);
            return Result;
        }

        const auto TileCount = InOutState._Partial.Get_TileCount();

        auto SpentThisSlice = 0;
        auto DroppedThisSlice = 0;
        auto Geometry = FCk_GroundNav_GeometryBatch{};
        auto Bodies = TArray<FCk_GroundNav_BodyRef>{};

        while (InOutState._NextTileIndex < TileCount)
        {
            // Checked AFTER the first tile of the slice rather than before it, so a budget smaller than
            // any single tile still advances the build instead of spinning forever on a resume point it
            // can never get past.
            if (SpentThisSlice > 0 && SpentThisSlice >= InProbeBudget)
            { break; }

            // Re-read per TILE and not just per slice: a backend whose world moves between two tiles of
            // the SAME slice produces exactly the disagreeing seam this whole check exists to refuse.
            if (InBackend.Get_WorldRevision() != InOutState._GeometryRevision)
            {
                InOutState._Status = ECk_GroundNav_BuildStatus::Failed;

                Result.Set_Status(ECk_GroundNav_BakeStatus::StaleGeometry);
                return Result;
            }

            const auto TileIndex = InOutState._NextTileIndex;
            const auto Coord = Get_TileCoord(InOutState._Params._Divisions, TileIndex);
            const auto TileParams = InOutState._Params.Get_TileBakeParams(Coord, InOutState._Epoch);

            const auto HaloBounds = Get_TileHaloBounds(TileParams);

            Geometry.Reset();
            InBackend.Get_TrianglesInBounds(HaloBounds, Geometry);

            // _CheckedBodies lives on the build state, so a body straddling tiles baked in different
            // slices is still judged once — which is what keeps the whole build's probe total the same
            // number the one-shot bake spends, whatever budget it ran under.
            InBackend.Get_StaticBodiesInBounds(HaloBounds, Bodies);
            DoCheck_GeometryClosure(InBackend, Bodies, InOutState._CheckedBodies,
                InOutState._Partial._OpenBodies, SpentThisSlice);

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

        DoReport_OpenBodies(InOutState._Partial._OpenBodies);

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

        auto Released = MakeShared<const FCk_GroundNav_Field>(MoveTemp(InOutState._Partial));

        // The build is SPENT. Without this the moved-from _Partial still reports complete, and a
        // second release would hand back a non-null EMPTY field — which reads exactly like a world
        // whose tiles have no floor, the failure Get_CompletedField's own contract warns about.
        InOutState._Status = ECk_GroundNav_BuildStatus::Unbuilt;
        InOutState._NextTileIndex = 0;

        return Released;
    }
}

// --------------------------------------------------------------------------------------------------------------------
