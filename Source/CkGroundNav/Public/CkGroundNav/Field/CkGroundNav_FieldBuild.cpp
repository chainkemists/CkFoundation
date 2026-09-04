#include "CkGroundNav_FieldBuild.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Field/CkGroundNav_FieldLinks.h"

#include <UObject/Class.h>
#include <UObject/PropertyPortFlags.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace fieldbuild_private
    {
        // Reflection rather than a hand-written member sweep: these are USTRUCTs whose members include
        // an FCk_AnyShape and two FGameplayTags, and a comparison written out here would be a second
        // definition of their contents that a new member would silently fall out of - which is precisely
        // the drift a variant check exists to catch.
        template <typename T_Reflected>
        auto Get_ReflectedValuesAreEqual(
            const T_Reflected& InLeft,
            const T_Reflected& InRight) -> bool
        {
            return T_Reflected::StaticStruct()->CompareScriptStruct(&InLeft, &InRight, PPF_None);
        }

        template <typename T_Reflected>
        auto Get_ReflectedArraysAreEqual(
            const TArray<T_Reflected>& InLeft,
            const TArray<T_Reflected>& InRight) -> bool
        {
            if (InLeft.Num() != InRight.Num())
            { return false; }

            for (auto Index = 0; Index < InLeft.Num(); ++Index)
            {
                if (NOT Get_ReflectedValuesAreEqual(InLeft[Index], InRight[Index]))
                { return false; }
            }

            return true;
        }

        /**
         * The name of the first field two profile variants disagree on, or nothing when they differ
         * only in their _Profile.
         *
         * The NAME rather than a bool: a caller handed a rejection has to be told which of ten fields
         * it got wrong, and every one of them looks equally plausible from the call site.
         */
        auto Get_FirstNonProfileDifference(
            const FCk_GroundNav_FieldParams& InLeft,
            const FCk_GroundNav_FieldParams& InRight) -> FString
        {
            if (InLeft._OriginXY != InRight._OriginXY)
            { return TEXT("_OriginXY"); }

            if (InLeft._Divisions != InRight._Divisions)
            { return TEXT("_Divisions"); }

            if (InLeft._MinZUu != InRight._MinZUu)
            { return TEXT("_MinZUu"); }

            if (InLeft._MaxZUu != InRight._MaxZUu)
            { return TEXT("_MaxZUu"); }

            if (InLeft._MaxClearanceUu != InRight._MaxClearanceUu)
            { return TEXT("_MaxClearanceUu"); }

            if (NOT Get_ReflectedValuesAreEqual(InLeft._Config, InRight._Config))
            { return TEXT("_Config"); }

            if (NOT Get_ReflectedValuesAreEqual(InLeft._MergeTunables, InRight._MergeTunables))
            { return TEXT("_MergeTunables"); }

            if (NOT Get_ReflectedArraysAreEqual(InLeft._MarkupRecords, InRight._MarkupRecords))
            { return TEXT("_MarkupRecords"); }

            if (NOT Get_ReflectedArraysAreEqual(InLeft._Links, InRight._Links))
            { return TEXT("_Links"); }

            return {};
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_BeginBuild(
            const FCk_GroundNav_FieldParams& InParams,
            const FCk_GroundNav_Epoch&       InEpoch,
            FCk_GroundNav_FieldBuildState&   OutState)
        -> FCk_GroundNav_BakeStageResult
    {
        return Request_BeginBuild_MultiProfile(
            TConstArrayView<FCk_GroundNav_FieldParams>{&InParams, 1}, InEpoch, OutState);
    }

    auto
        Request_BeginBuild_MultiProfile(
            TConstArrayView<FCk_GroundNav_FieldParams> InParams,
            const FCk_GroundNav_Epoch&                 InEpoch,
            FCk_GroundNav_FieldBuildState&             OutState)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        OutState = FCk_GroundNav_FieldBuildState{};
        OutState._Params.Append(InParams.GetData(), InParams.Num());
        OutState._Epoch = InEpoch;

        const auto ThereIsSomethingToBake = NOT InParams.IsEmpty();

        CK_ENSURE_IF_NOT(ThereIsSomethingToBake,
            TEXT("A GroundNav field build was begun with no params, so there is no field for it to produce"))
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        for (auto ProfileIndex = 0; ProfileIndex < InParams.Num(); ++ProfileIndex)
        {
            const auto& Params = InParams[ProfileIndex];

            if (NOT Params.Get_IsValid())
            {
                Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
                return Result;
            }

            // A refused profile terminates the build here rather than reaching the tiles, where every
            // one of them would fail on its own and the build would go on to publish a field with
            // nothing built in it — a place with nowhere to walk, which is not what an unaskable
            // profile means.
            if (Get_ProfileRejection(Params._Profile) != EProfileRejection::None)
            {
                Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
                return Result;
            }

            const auto Difference = fieldbuild_private::Get_FirstNonProfileDifference(InParams[0], Params);
            const auto VariesOnlyByProfile = Difference.IsEmpty();

            CK_ENSURE_IF_NOT(VariesOnlyByProfile,
                TEXT("GroundNav field build params must differ only in their agent profile. Variant [{}] ")
                TEXT("disagrees with the first on [{}], so the two cannot share one geometry collection"),
                ProfileIndex, Difference)
            {
                Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
                return Result;
            }
        }

        OutState._Partial.SetNum(InParams.Num());

        for (auto ProfileIndex = 0; ProfileIndex < InParams.Num(); ++ProfileIndex)
        {
            auto& Partial = OutState._Partial[ProfileIndex];

            Partial._Params = InParams[ProfileIndex];
            Partial._Epoch = InEpoch;
            Partial._Tiles.SetNum(InParams[ProfileIndex].Get_TileCount());
        }

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

        // A begin that was refused leaves the params it was handed on the state and no field beside
        // them, so the two counts disagreeing is exactly the state a refused build is left in.
        const auto ParamsAreUsable = NOT InOutState._Params.IsEmpty() &&
                                     InOutState._Partial.Num() == InOutState._Params.Num() &&
                                     InOutState._Params[0].Get_IsValid();

        if (NOT ParamsAreUsable)
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

        const auto TileCount = InOutState.Get_TileCount();
        const auto ProfileCount = InOutState._Partial.Num();

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
            const auto Coord = Get_TileCoord(InOutState._Params[0]._Divisions, TileIndex);

            // Placed off the FIRST profile's params, which admission has already established every
            // other profile agrees with about the lattice and the clearance ceiling the halo is sized
            // from. The geometry a tile needs is therefore one question, not one per profile.
            const auto HaloBounds = Get_TileHaloBounds(
                InOutState._Params[0].Get_TileBakeParams(Coord, InOutState._Epoch));

            Geometry.Reset();
            InBackend.Get_TrianglesInBounds(HaloBounds, Geometry);

            ++InOutState._GeometryFetches;

            // _CheckedBodies lives on the build state, so a body straddling tiles baked in different
            // slices is still judged once — which is what keeps the whole build's probe total the same
            // number the one-shot bake spends, whatever budget it ran under.
            InBackend.Get_StaticBodiesInBounds(HaloBounds, Bodies);
            DoCheck_GeometryClosure(InBackend, Bodies, InOutState._CheckedBodies,
                InOutState._OpenBodies, SpentThisSlice);

            for (auto ProfileIndex = 0; ProfileIndex < ProfileCount; ++ProfileIndex)
            {
                const auto TileParams = InOutState._Params[ProfileIndex].Get_TileBakeParams(
                    Coord, InOutState._Epoch);

                const auto TileResult = DoBake_Tile(
                    Geometry, TileParams, InOutState._Partial[ProfileIndex]._Tiles[TileIndex]);

                SpentThisSlice += TileResult.Get_ProbesSpent();
                DroppedThisSlice += TileResult.Get_DroppedInputCount();
            }

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

        for (auto& Partial : InOutState._Partial)
        {
            // Every profile carries the same report because every profile bakes out of the same
            // collection; only the WARNING is per build, below.
            Partial._OpenBodies = InOutState._OpenBodies;

            // All three are derived from the finished tiles, so they belong to the end of the build and
            // not to whichever slice happened to bake the last tile.
            DoDerive_SeamPortals(Partial);
            DoResolve_Links(Partial);
            DoLabel_Reachability(Partial);
        }

        DoReport_OpenBodies(InOutState._OpenBodies);

        InOutState._Status = ECk_GroundNav_BuildStatus::Built;

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        return Result;
    }

    auto
        Get_CompletedField(
            const FCk_GroundNav_FieldBuildState& InState)
        -> const FCk_GroundNav_Field*
    {
        const auto Fields = Get_CompletedFields(InState);

        return Fields.IsEmpty() ? nullptr : &Fields[0];
    }

    auto
        Get_CompletedFields(
            const FCk_GroundNav_FieldBuildState& InState)
        -> TConstArrayView<FCk_GroundNav_Field>
    {
        return InState.Get_IsComplete()
            ? TConstArrayView<FCk_GroundNav_Field>{InState._Partial}
            : TConstArrayView<FCk_GroundNav_Field>{};
    }

    auto
        Request_ReleaseCompletedField(
            FCk_GroundNav_FieldBuildState& InOutState)
        -> FCk_GroundNav_FieldPtr
    {
        auto Released = Request_ReleaseCompletedFields(InOutState);

        return Released.IsEmpty() ? FCk_GroundNav_FieldPtr{} : Released[0];
    }

    auto
        Request_ReleaseCompletedFields(
            FCk_GroundNav_FieldBuildState& InOutState)
        -> TArray<FCk_GroundNav_FieldPtr>
    {
        if (NOT InOutState.Get_IsComplete())
        { return {}; }

        auto Released = TArray<FCk_GroundNav_FieldPtr>{};
        Released.Reserve(InOutState._Partial.Num());

        for (auto& Partial : InOutState._Partial)
        { Released.Emplace(MakeShared<const FCk_GroundNav_Field>(MoveTemp(Partial))); }

        // The build is SPENT. Without this the moved-from partials still report complete, and a
        // second release would hand back non-null EMPTY fields — which read exactly like a world
        // whose tiles have no floor, the failure Get_CompletedField's own contract warns about.
        InOutState._Status = ECk_GroundNav_BuildStatus::Unbuilt;
        InOutState._NextTileIndex = 0;

        return Released;
    }
}

// --------------------------------------------------------------------------------------------------------------------
