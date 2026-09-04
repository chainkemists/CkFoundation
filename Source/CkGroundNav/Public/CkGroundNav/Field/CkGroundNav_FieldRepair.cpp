#include "CkGroundNav_FieldRepair.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace fieldrepair_private
    {
        auto Get_TileWorldBoundsFromParams(
            const FCk_GroundNav_FieldParams& InParams,
            int32                            InTileIndex) -> FBox
        {
            const auto Coord = Get_TileCoord(InParams._Divisions, InTileIndex);

            return Get_TileBounds(InParams.Get_TileBakeParams(Coord, FCk_GroundNav_Epoch{}));
        }

        auto Get_HaloInflatedBounds(
            const FCk_GroundNav_FieldParams& InParams,
            const FBox&                      InBounds) -> FBox
        {
            const auto CellSize = InParams._Config.Get_CellSizeUu();
            const auto HaloUu = static_cast<double>(
                Get_HaloCellCount(InParams._MaxClearanceUu, CellSize)) * static_cast<double>(CellSize);

            return FBox{
                FVector{InBounds.Min.X - HaloUu, InBounds.Min.Y - HaloUu, InBounds.Min.Z},
                FVector{InBounds.Max.X + HaloUu, InBounds.Max.Y + HaloUu, InBounds.Max.Z}};
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_RepairTileIndices(
            const FCk_GroundNav_Field& InSource,
            const FBox&                InDirtyBounds)
        -> TArray<int32>
    {
        using namespace fieldrepair_private;

        auto Indices = TArray<int32>{};

        if (InDirtyBounds.IsValid == 0)
        { return Indices; }

        const auto& Params = InSource._Params;
        const auto Inflated = Get_HaloInflatedBounds(Params, InDirtyBounds);

        const auto TileCount = InSource.Get_TileCount();

        for (auto TileIndex = 0; TileIndex < TileCount; ++TileIndex)
        {
            if (NOT Inflated.Intersect(Get_TileWorldBoundsFromParams(Params, TileIndex)))
            { continue; }

            Indices.Emplace(TileIndex);
        }

        return Indices;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_BeginRepair(
            FCk_GroundNav_FieldRepairState&             OutState,
            const FCk_GroundNav_FieldPtr&               InSource,
            const FBox&                                 InDirtyBounds,
            const FCk_GroundNav_Epoch&                  InEpoch,
            TConstArrayView<FCk_GroundNav_MarkupRecord> InCurrentMarkupRecords)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        OutState = FCk_GroundNav_FieldRepairState{};

        const auto SourceIsPublished = InSource.IsValid() &&
                                       InSource->_Params.Get_IsValid() &&
                                       InSource->_Epoch.Get_IsBuilt() &&
                                       InSource->Get_TileCount() > 0;

        CK_ENSURE_IF_NOT(SourceIsPublished,
            TEXT("Cannot repair a GroundNav field that was never published - a local repair carries every "
                 "tile OUTSIDE its dirty bounds over from the previous bake, and there is none to carry."))
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto EpochIsNewer = InEpoch.Get_IsNewerThan(InSource->_Epoch);

        CK_ENSURE_IF_NOT(EpochIsNewer,
            TEXT("Cannot repair a GroundNav field published at epoch [{}] under epoch [{}] - a repaired "
                 "tile stamped at or behind the field it came from is one no reader can tell apart from "
                 "the tile it replaced, and liveness asks for an epoch strictly past the stamp."),
            InSource->_Epoch._Value, InEpoch._Value)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto DirtyBoundsAreABox = InDirtyBounds.IsValid != 0;

        CK_ENSURE_IF_NOT(DirtyBoundsAreABox,
            TEXT("Cannot repair a GroundNav field against a degenerate dirty box - a box that bounds "
                 "nothing and a box that reaches no tile are different answers, and only the second is "
                 "admissible."))
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        OutState._Source = InSource;
        OutState._Repaired = MakeShared<FCk_GroundNav_Field>(*InSource);
        OutState._Repaired->_Params._MarkupRecords = InCurrentMarkupRecords;
        OutState._DirtyBounds = InDirtyBounds;
        OutState._Epoch = InEpoch;
        OutState._RepairTileIndices = Get_RepairTileIndices(*InSource, InDirtyBounds);

        if (OutState._RepairTileIndices.IsEmpty())
        { OutState._Status = ECk_GroundNav_BuildStatus::Built; }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        return Result;
    }

    auto
        Request_AdvanceRepair(
            const ICk_GroundNav_GeometryBackend& InBackend,
            int32                                InProbeBudget,
            FCk_GroundNav_FieldRepairState&      InOutState)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        if (NOT InOutState._Repaired.IsValid() || NOT InOutState._Repaired->_Params.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        // A repair whose tile set was empty is already whole, and a driver that calls this unconditionally
        // gets the same answer it would get from the slice that finished one.
        if (InOutState._Status == ECk_GroundNav_BuildStatus::Built)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
            return Result;
        }

        if (NOT InBackend.Get_IsValid())
        {
            InOutState._Status = ECk_GroundNav_BuildStatus::Failed;

            Result.Set_Status(ECk_GroundNav_BakeStatus::BackendUnavailable);
            return Result;
        }

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

        auto& Repaired = *InOutState._Repaired;

        const auto RepairCount = InOutState._RepairTileIndices.Num();

        auto SpentThisSlice = 0;
        auto DroppedThisSlice = 0;
        auto Geometry = FCk_GroundNav_GeometryBatch{};
        auto Bodies = TArray<FCk_GroundNav_BodyRef>{};

        while (InOutState._NextRepairCursor < RepairCount)
        {
            if (SpentThisSlice > 0 && SpentThisSlice >= InProbeBudget)
            { break; }

            if (InBackend.Get_WorldRevision() != InOutState._GeometryRevision)
            {
                InOutState._Status = ECk_GroundNav_BuildStatus::Failed;

                Result.Set_Status(ECk_GroundNav_BakeStatus::StaleGeometry);
                return Result;
            }

            const auto TileIndex = InOutState._RepairTileIndices[InOutState._NextRepairCursor];
            const auto Coord = Get_TileCoord(Repaired._Params._Divisions, TileIndex);
            const auto TileParams = Repaired._Params.Get_TileBakeParams(Coord, InOutState._Epoch);

            const auto HaloBounds = Get_TileHaloBounds(TileParams);

            Geometry.Reset();
            InBackend.Get_TrianglesInBounds(HaloBounds, Geometry);

            const auto TileResult = DoBake_Tile(Geometry, TileParams, Repaired._Tiles[TileIndex]);

            SpentThisSlice += TileResult.Get_ProbesSpent();
            DroppedThisSlice += TileResult.Get_DroppedInputCount();

            ++InOutState._NextRepairCursor;
        }

        if (InOutState._NextRepairCursor < RepairCount)
        {
            InOutState._ProbesSpent += SpentThisSlice;

            Result.Set_ProbesSpent(SpentThisSlice);
            Result.Set_DroppedInputCount(DroppedThisSlice);
            Result.Set_Status(ECk_GroundNav_BakeStatus::BudgetExhausted);

            return Result;
        }

        // The closure pass runs over EVERY tile, ascending, against a cleared set - not over the repaired
        // tiles alone. _OpenBodies is first-touch ordered, so a pass that visited only some tiles would
        // report the same open bodies in a different order and against a different set of tiles from the
        // one a full bake reports them under, and byte-identity is the whole claim of a repair.
        InOutState._CheckedBodies.Reset();
        Repaired._OpenBodies.Reset();

        for (auto TileIndex = 0; TileIndex < Repaired.Get_TileCount(); ++TileIndex)
        {
            if (InBackend.Get_WorldRevision() != InOutState._GeometryRevision)
            {
                InOutState._Status = ECk_GroundNav_BuildStatus::Failed;

                Result.Set_Status(ECk_GroundNav_BakeStatus::StaleGeometry);
                return Result;
            }

            const auto Coord = Get_TileCoord(Repaired._Params._Divisions, TileIndex);
            const auto TileParams = Repaired._Params.Get_TileBakeParams(Coord, InOutState._Epoch);

            const auto HaloBounds = Get_TileHaloBounds(TileParams);

            InBackend.Get_StaticBodiesInBounds(HaloBounds, Bodies);
            DoCheck_GeometryClosure(InBackend, Bodies, InOutState._CheckedBodies,
                Repaired._OpenBodies, SpentThisSlice);
        }

        DoDerive_SeamPortals(Repaired);
        DoLabel_Reachability(Repaired);

        DoReport_OpenBodies(Repaired._OpenBodies);

        Repaired._Epoch = InOutState._Epoch;

        InOutState._Status = ECk_GroundNav_BuildStatus::Built;
        InOutState._ProbesSpent += SpentThisSlice;

        Result.Set_ProbesSpent(SpentThisSlice);
        Result.Set_DroppedInputCount(DroppedThisSlice);
        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        return Result;
    }

    auto
        Get_RepairedField(
            const FCk_GroundNav_FieldRepairState& InState)
        -> FCk_GroundNav_FieldPtr
    {
        return InState.Get_IsBuilt() ? FCk_GroundNav_FieldPtr{InState._Repaired} : FCk_GroundNav_FieldPtr{};
    }

    auto
        Request_ReleaseRepairedField(
            FCk_GroundNav_FieldRepairState& InOutState)
        -> FCk_GroundNav_FieldPtr
    {
        if (NOT InOutState.Get_IsBuilt())
        { return {}; }

        auto Released = FCk_GroundNav_FieldPtr{MoveTemp(InOutState._Repaired)};

        // The repair is SPENT. Without this a second release would hand back the same field again, and a
        // caller could not tell one publish from two.
        InOutState._Source = {};
        InOutState._RepairTileIndices.Reset();
        InOutState._NextRepairCursor = 0;
        InOutState._CheckedBodies.Reset();
        InOutState._Status = ECk_GroundNav_BuildStatus::Unbuilt;

        return Released;
    }
}

// --------------------------------------------------------------------------------------------------------------------
