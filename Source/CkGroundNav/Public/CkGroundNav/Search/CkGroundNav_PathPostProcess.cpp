#include "CkGroundNav_PathPostProcess.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Query/CkGroundNav_Funnel.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Attributes.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Boundary.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace pathpostprocess_private
    {
        // Four halvings, so the last candidate is a sixteenth of the asked offset before the pass
        // gives up and keeps the point the funnel produced.
        constexpr auto kMaxCornerHalvings = 4;

        // The probe only has to prove one wall is far enough, so it never looks further than a point
        // that passed could possibly need.
        constexpr auto kCornerProbeRadiusMultiplier = 2.0f;

        // The Recast path's own threshold. A body switching providers must not change which waypoint
        // it steers at first, so this number is copied and never re-derived.
        constexpr auto kSkipFirstRadiusMultiplier = 2.0f;

        // A polyline segment passes through no door, so nothing narrows it.
        constexpr auto kNoClearanceFactor = 1.0f;

        constexpr auto kFewestPointsWithAnInterior = 3;

        // ------------------------------------------------------------------------------------------------------------

        /**
         * The points an authored link put on the route, which the corner offset must leave alone.
         *
         * A link contributes one degenerate portal per endpoint, so either end of one names the
         * point, and the two portals of one link together name its entry and its exit.
         */
        auto Get_LinkWaypoints(
            const FCk_GroundNav_PathResult& InResult) -> TArray<FVector>
        {
            auto Pinned = TArray<FVector>{};

            for (const auto& Portal : InResult._FunnelPortals)
            {
                if (Portal._LinkIndex == INDEX_NONE)
                { continue; }

                Pinned.Emplace(Portal._Left);
            }

            return Pinned;
        }

        // ------------------------------------------------------------------------------------------------------------

        auto Make_IsNavigableQuery(
            const FVector&                  InLocation,
            const FCk_GroundNav_QueryAgent& InAgent,
            float                           InVerticalToleranceUu) -> FCk_GroundNav_IsNavigableQuery
        {
            auto Query = FCk_GroundNav_IsNavigableQuery{};
            Query._Location = InLocation;
            Query._VerticalToleranceUu = InVerticalToleranceUu;
            Query._Agent = InAgent;

            return Query;
        }

        /**
         * The shared data leg pricing reads, assembled from a field the caller owns. Leg pricing
         * reads the constants and the multiplier table only, so the field pointer is left unset
         * rather than borrowed from a reference this value could outlive — the field a plate's own
         * price is read from is passed to Get_AreaMultiplier beside this, for the same reason.
         */
        auto Make_SharedData(
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent) -> FCk_GroundNav_PathSharedData
        {
            auto Shared = FCk_GroundNav_PathSharedData{};
            Shared._Epoch = InField._Epoch;
            Shared._Agent = InAgent;
            Shared._SlopePenaltyK = InCost._SlopePenaltyK;
            Shared._ClearanceBiasK = InCost._ClearanceBiasK;
            Shared._CellSizeUu = InField._Params._Config.Get_CellSizeUu();
            Shared._PlateCostMultipliers = InCost._PlateCostMultipliers;

            return Shared;
        }

        auto Get_FlatPlateOf(
            const FCk_GroundNav_Field&             InField,
            const FCk_GroundNav_SurfaceAttributes& InAttributes) -> int32
        {
            if (NOT InAttributes.Get_IsSuccess())
            { return INDEX_NONE; }

            return Get_FlatPlateIndex(
                InField, InAttributes._Surface._TileIndex, InAttributes._Surface._PlateIndex);
        }

        auto Get_IsCornerAccepted(
            const FCk_GroundNav_Field&      InField,
            const FVector&                  InCandidate,
            const FCk_GroundNav_QueryAgent& InAgent,
            float                           InVerticalToleranceUu,
            float                           InOffsetUu) -> bool
        {
            const auto Navigable = Get_IsNavigable(
                InField, Make_IsNavigableQuery(InCandidate, InAgent, InVerticalToleranceUu));

            if (NOT Navigable.Get_IsSuccess())
            { return false; }

            auto BoundaryQuery = FCk_GroundNav_ClosestBoundaryQuery{};
            BoundaryQuery._Location = InCandidate;
            BoundaryQuery._MaxRadiusUu = kCornerProbeRadiusMultiplier * (InAgent._RadiusUu + InOffsetUu);
            BoundaryQuery._VerticalWindowUu = InVerticalToleranceUu;
            BoundaryQuery._Agent = InAgent;

            const auto Boundary = Get_ClosestBoundary(InField, BoundaryQuery);

            // No answer means no wall inside a radius wider than anything an accepted point needs:
            // open floor, and the strongest pass the probe can report.
            if (NOT Boundary.Get_IsSuccess())
            { return true; }

            return Boundary._DistanceUu >= InAgent._RadiusUu;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_Funnelled(
            const FCk_GroundNav_PathResult& InResult,
            float                           InRadiusUu,
            TArray<FVector>&                OutWaypoints)
        -> double
    {
        return Get_StringPull(
            InResult._StartPoint,
            InResult._GoalPoint,
            InResult._FunnelPortals,
            InRadiusUu,
            OutWaypoints);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CornerOffset(
            TConstArrayView<FVector>        InWaypoints,
            TConstArrayView<FVector>        InPinnedWaypoints,
            const FCk_GroundNav_Field&      InField,
            float                           InOffsetUu,
            const FCk_GroundNav_QueryAgent& InAgent,
            float                           InVerticalToleranceUu)
        -> TArray<FVector>
    {
        using namespace pathpostprocess_private;

        auto Offset = TArray<FVector>{};
        Offset.Append(InWaypoints.GetData(), InWaypoints.Num());

        if (Offset.Num() < kFewestPointsWithAnInterior || InOffsetUu <= 0.0f)
        { return Offset; }

        const auto LastInteriorIndex = Offset.Num() - 2;

        for (auto Index = 1; Index <= LastInteriorIndex; ++Index)
        {
            const auto Corner = InWaypoints[Index];

            // Where a record put it rather than where a string bent, so it stays: both this point
            // and the pinned one were copied from the same resolved endpoint, which is what makes
            // an exact comparison the right one and an epsilon a way to move the wrong waypoint.
            if (InPinnedWaypoints.Contains(Corner))
            { continue; }

            const auto ToPrevious = (InWaypoints[Index - 1] - Corner).GetSafeNormal();
            const auto ToNext = (InWaypoints[Index + 1] - Corner).GetSafeNormal();

            // The legs bend around a vertex that sits on the bisector's side, so the free space a
            // body wants lies the other way; a near-zero sum is the reversal a funnel cannot emit
            // and is left alone.
            const auto Bisector = (ToPrevious + ToNext).GetSafeNormal();

            if (Bisector.IsNearlyZero())
            { continue; }

            auto AttemptOffsetUu = InOffsetUu;

            for (auto Attempt = 0; Attempt <= kMaxCornerHalvings; ++Attempt)
            {
                const auto Candidate = Corner - (Bisector * static_cast<double>(AttemptOffsetUu));

                if (Get_IsCornerAccepted(InField, Candidate, InAgent, InVerticalToleranceUu, AttemptOffsetUu))
                {
                    Offset[Index] = Candidate;
                    break;
                }

                AttemptOffsetUu *= 0.5f;
            }
        }

        return Offset;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_SkipFirstWaypoint(
            TConstArrayView<FVector> InWaypoints,
            const FVector&           InAgentLocation,
            float                    InAgentRadiusUu)
        -> TArray<FVector>
    {
        using namespace pathpostprocess_private;

        auto Kept = TArray<FVector>{};
        Kept.Reserve(InWaypoints.Num());

        const auto SkipFirstThresholdSquared = (InAgentRadiusUu > 0.0f)
            ? FMath::Square(InAgentRadiusUu * kSkipFirstRadiusMultiplier)
            : -1.0f;

        for (auto Index = 0; Index < InWaypoints.Num(); ++Index)
        {
            const auto& Point = InWaypoints[Index];

            if (Index == 0 && SkipFirstThresholdSquared > 0.0f &&
                FVector::DistSquared(Point, InAgentLocation) <= SkipFirstThresholdSquared)
            { continue; }

            Kept.Emplace(Point);
        }

        return Kept;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_FilledWaypoints(
            TConstArrayView<FVector>            InWaypoints,
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu)
        -> TArray<FCk_GroundNav_PathWaypoint>
    {
        using namespace pathpostprocess_private;

        auto Filled = TArray<FCk_GroundNav_PathWaypoint>{};

        if (InWaypoints.IsEmpty())
        { return Filled; }

        const auto Shared = Make_SharedData(InField, InCost, InAgent);

        auto FlatPlates = TArray<int32>{};

        Filled.Reserve(InWaypoints.Num());
        FlatPlates.Reserve(InWaypoints.Num());

        for (const auto& Location : InWaypoints)
        {
            const auto Attributes = Get_SurfaceAttributesAt(
                InField, Make_IsNavigableQuery(Location, InAgent, InVerticalToleranceUu));

            auto Waypoint = FCk_GroundNav_PathWaypoint{};
            Waypoint._Location = Location;
            Waypoint._SurfaceNormal = Attributes._SurfaceNormal;
            Waypoint._AreaTags = Attributes._AreaTags;

            Filled.Emplace(MoveTemp(Waypoint));
            FlatPlates.Emplace(Get_FlatPlateOf(InField, Attributes));
        }

        for (auto Index = 1; Index < Filled.Num(); ++Index)
        {
            const auto From = Filled[Index - 1]._Location;
            const auto To = Filled[Index]._Location;

            const auto AreaMultiplier = FMath::Max(
                Get_AreaMultiplier(InField, Shared, FlatPlates[Index - 1]),
                Get_AreaMultiplier(InField, Shared, FlatPlates[Index]));

            const auto LegCostUu = Get_LegCost(Shared, From, To, AreaMultiplier, kNoClearanceFactor);

            Filled[Index]._DistanceFromStart =
                Filled[Index - 1]._DistanceFromStart + FVector::Dist2D(From, To);

            Filled[Index]._CostFromStart =
                Filled[Index - 1]._CostFromStart + static_cast<double>(LegCostUu);

            Filled[Index - 1]._DirectionToNext = (To - From).GetSafeNormal();
        }

        return Filled;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MaxMerged(
            TConstArrayView<TMap<int32, float>> InTables)
        -> TMap<int32, float>
    {
        auto Merged = TMap<int32, float>{};

        for (const auto& Table : InTables)
        {
            for (const auto& Entry : Table)
            {
                if (auto* Existing = Merged.Find(Entry.Key))
                {
                    *Existing = FMath::Max(*Existing, Entry.Value);
                    continue;
                }

                Merged.Add(Entry.Key, Entry.Value);
            }
        }

        return Merged;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_PathPlan(
            const FCk_GroundNav_PathResult&     InResult,
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathPostParams& InParams)
        -> FCk_GroundNav_PathPlan
    {
        using namespace pathpostprocess_private;

        auto Plan = FCk_GroundNav_PathPlan{};
        Plan._Status = InResult._Status;
        Plan._PlannedAgainstEpoch = InResult._PlannedAgainstEpoch;

        const auto IsWalkableStatus =
            InResult._Status == ECk_GroundNav_PathStatus::Ready ||
            InResult._Status == ECk_GroundNav_PathStatus::Partial;

        if (NOT IsWalkableStatus)
        { return Plan; }

        Plan._PlateCorridor = InResult._PlateCorridor;

        const auto RadiusUu = InParams._Agent._RadiusUu;

        auto Funnelled = TArray<FVector>{};
        Get_Funnelled(InResult, RadiusUu, Funnelled);

        const auto Pinned = Get_LinkWaypoints(InResult);

        const auto Offset = Get_CornerOffset(
            Funnelled,
            Pinned,
            InField,
            InParams._Cost._CornerOffsetK * RadiusUu,
            InParams._Agent,
            InParams._VerticalToleranceUu);

        const auto Trimmed = Get_SkipFirstWaypoint(Offset, InParams._AgentLocation, RadiusUu);

        Plan._Waypoints = Get_FilledWaypoints(
            Trimmed,
            InField,
            InParams._Cost,
            InParams._Agent,
            InParams._VerticalToleranceUu);

        Plan._LengthUu = Plan._Waypoints.IsEmpty()
            ? 0.0
            : Plan._Waypoints.Last()._DistanceFromStart;

        return Plan;
    }
}

// --------------------------------------------------------------------------------------------------------------------
