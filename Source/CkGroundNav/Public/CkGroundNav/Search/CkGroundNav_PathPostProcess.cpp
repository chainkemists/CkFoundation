#include "CkGroundNav_PathPostProcess.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Query/CkGroundNav_Funnel.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Attributes.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Boundary.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.h"
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

        // The ray budget is a sum of FLOAT segment prices - _AccumulatedCost is a float - accumulated
        // in double, and the cap that sum becomes is a float again, so a chord a body can exactly
        // afford is exposed to BOTH roundings: the summands it is measured against were rounded once
        // where the ray answered them, and the cap rounds a second time and can land one ulp under
        // the chord's own cost. The slack is relative and three orders of magnitude under a cell, so
        // it absorbs both and nothing a plate price could hide in.
        constexpr auto kShortcutBudgetSlack = 1.0e-5;

        // A link endpoint stands ON the corridor, so it can miss the segment carrying it only by the
        // arithmetic that produced that segment's own ends. A wider window would let a route that
        // doubles back claim the leg beside the one the endpoint is actually on.
        constexpr auto kOnSegmentToleranceUu = 0.1;

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

        /**
         * Whether a point lies on a polyline leg, in XY - the arithmetic the funnel string-pulls in,
         * so a point the corridor put on the line is on it by the same measure that placed it.
         *
         * The parameter bound is the dot-product one, so a point beyond either end is rejected however
         * exactly it sits on the leg's infinite line.
         */
        auto Get_IsOnSegmentXY(
            const FVector& InFrom,
            const FVector& InTo,
            const FVector& InPoint) -> bool
        {
            const auto SegmentX = InTo.X - InFrom.X;
            const auto SegmentY = InTo.Y - InFrom.Y;
            const auto LengthSquared = (SegmentX * SegmentX) + (SegmentY * SegmentY);

            if (LengthSquared <= 0.0)
            { return false; }

            const auto ToPointX = InPoint.X - InFrom.X;
            const auto ToPointY = InPoint.Y - InFrom.Y;

            const auto Parameter = ((ToPointX * SegmentX) + (ToPointY * SegmentY)) / LengthSquared;

            if (Parameter < 0.0 || Parameter > 1.0)
            { return false; }

            const auto Cross = (ToPointX * SegmentY) - (ToPointY * SegmentX);

            return FMath::Abs(Cross) <= kOnSegmentToleranceUu * FMath::Sqrt(LengthSquared);
        }

        /** The index of the first leg carrying a point, as the index of that leg's FIRST waypoint. */
        auto Get_SegmentContaining(
            TConstArrayView<FVector> InWaypoints,
            const FVector&           InPoint) -> int32
        {
            for (auto Index = 1; Index < InWaypoints.Num(); ++Index)
            {
                if (Get_IsOnSegmentXY(InWaypoints[Index - 1], InWaypoints[Index], InPoint))
                { return Index - 1; }
            }

            return INDEX_NONE;
        }

        /**
         * The resolved link a point stands on an end of, as an index into the field's own array, and
         * INDEX_NONE for a point no link put there.
         *
         * Compared EXACTLY, for the reason the pinned-point rule above is: the portal's point and the
         * waypoint are both copies of the one resolved endpoint, so an epsilon here would claim a
         * neighbouring corner for the link.
         */
        auto Get_LinkIndexAt(
            const FCk_GroundNav_PathResult& InResult,
            const FVector&                  InPoint) -> int32
        {
            for (const auto& Portal : InResult._FunnelPortals)
            {
                if (Portal._LinkIndex == INDEX_NONE)
                { continue; }

                if (Portal._Left == InPoint)
                { return Portal._LinkIndex; }
            }

            return INDEX_NONE;
        }

        /**
         * Which of the finished waypoints an authored link put on the route, and which way it is walked.
         *
         * The first waypoint of a link in WALK order is its entry and the next one its exit. The
         * direction is decided at the entry - entered at the record's _Start is Forward, the rule
         * Get_IsEnteredAtLinkStart already applies - and then carried onto the exit, so both ends of
         * one traversal answer the same.
         *
         * A link the field no longer resolves is left unstamped rather than stamped with a broken id:
         * the portals were enumerated from this field, so the index is expected to be good, and a
         * waypoint that says nothing is what a consumer can act on.
         */
        auto DoStamp_LinkWaypoints(
            const FCk_GroundNav_PathResult&     InResult,
            const FCk_GroundNav_Field&          InField,
            TArray<FCk_GroundNav_PathWaypoint>& InOutWaypoints) -> void
        {
            auto EntryDirectionByLinkIndex = TMap<int32, ECk_GroundNav_LinkDirection>{};

            for (auto& Waypoint : InOutWaypoints)
            {
                const int32 LinkIndex = Get_LinkIndexAt(InResult, Waypoint._Location);

                if (LinkIndex == INDEX_NONE || NOT InField._ResolvedLinks.IsValidIndex(LinkIndex))
                { continue; }

                const auto& Link = InField._ResolvedLinks[LinkIndex];

                Waypoint._LinkId = Link._Id;

                if (const auto* EntryDirection = EntryDirectionByLinkIndex.Find(LinkIndex))
                {
                    Waypoint._LinkRole = ECk_GroundNav_LinkWaypointRole::Exit;
                    Waypoint._LinkEntryDirection = *EntryDirection;

                    continue;
                }

                const auto Direction = Waypoint._Location == Link._Start
                    ? ECk_GroundNav_LinkDirection::Forward
                    : ECk_GroundNav_LinkDirection::Backward;

                Waypoint._LinkRole = ECk_GroundNav_LinkWaypointRole::Entry;
                Waypoint._LinkEntryDirection = Direction;

                EntryDirectionByLinkIndex.Add(LinkIndex, Direction);
            }
        }

        // ------------------------------------------------------------------------------------------------------------

        struct FStrictCellLinkStamp
        {
            int32 _WaypointIndex = INDEX_NONE;
            int32 _LinkStableId = INDEX_NONE;
            ECk_GroundNav_LinkWaypointRole _Role = ECk_GroundNav_LinkWaypointRole::None;
            ECk_GroundNav_LinkDirection _Direction = ECk_GroundNav_LinkDirection::Bidirectional;
        };

        struct FStrictCellLegCost
        {
            int32 _WaypointIndex = INDEX_NONE;
            float _Cost = 0.0f;
        };

        /**
         * Turns the strict cell collector's exact predecessor rows into the line it proved.  Unlike a
         * portal corridor this line is already the graph's collision-safe answer, so it deliberately
         * does not go through the funnel, corner offset, or shortcut passes.
         */
        auto Get_StrictCellLocations(
            const FCk_GroundNav_PathResult& InResult,
            TArray<FStrictCellLinkStamp>&   OutLinkStamps,
            TArray<FStrictCellLegCost>&     OutLegCosts) -> TOptional<TArray<FVector>>
        {
            auto Locations = TArray<FVector>{};
            Locations.Emplace(InResult._StartPoint);

            auto PreviousEdgeWasLink = false;
            for (const auto& Edge : InResult._CellRoute)
            {
                // Predecessor rows are contiguous. Treat a broken handoff as no route rather than
                // drawing an invented bridge between two points the strict graph never connected.
                if (Locations.Last() != Edge._FromPoint)
                { return {}; }

                // One physical endpoint can be the exit of one link and the entry of the next. Keep
                // two zero-length rows in that case: one waypoint can carry only one role, and losing
                // either role would publish an incomplete traversal to the follower.
                if (PreviousEdgeWasLink && Edge._Kind == ECk_GroundNav_CellRouteEdgeKind::Link)
                {
                    Locations.Emplace(Locations.Last());
                    OutLegCosts.Emplace(FStrictCellLegCost{Locations.Num() - 1, 0.0f});
                }

                const auto FromIndex = Locations.Num() - 1;
                if (NOT FMath::IsFinite(Edge._Cost) || Edge._Cost < 0.0f)
                { return {}; }

                // Retain every collector row, including a zero-length one: it has its own priced
                // predecessor leg and might be an authored link endpoint with distinct metadata.
                Locations.Emplace(Edge._ToPoint);
                const auto ToIndex = Locations.Num() - 1;
                OutLegCosts.Emplace(FStrictCellLegCost{ToIndex, Edge._Cost});

                if (Edge._Kind != ECk_GroundNav_CellRouteEdgeKind::Link)
                {
                    PreviousEdgeWasLink = false;
                    continue;
                }

                OutLinkStamps.Emplace(FStrictCellLinkStamp{
                    FromIndex, Edge._LinkStableId, ECk_GroundNav_LinkWaypointRole::Entry, Edge._LinkDirection});
                OutLinkStamps.Emplace(FStrictCellLinkStamp{
                    ToIndex, Edge._LinkStableId, ECk_GroundNav_LinkWaypointRole::Exit, Edge._LinkDirection});
                PreviousEdgeWasLink = true;
            }

            if (Locations.Last() != InResult._GoalPoint)
            { return {}; }

            return Locations;
        }

        auto DoApply_StrictCellLegCosts(
            TConstArrayView<FStrictCellLegCost>     InLegCosts,
            TArray<FCk_GroundNav_PathWaypoint>&     InOutWaypoints) -> bool
        {
            auto TotalCost = 0.0;
            for (const auto& Leg : InLegCosts)
            {
                if (NOT InOutWaypoints.IsValidIndex(Leg._WaypointIndex))
                { return false; }

                TotalCost += static_cast<double>(Leg._Cost);
                if (NOT FMath::IsFinite(TotalCost))
                { return false; }

                InOutWaypoints[Leg._WaypointIndex]._CostFromStart = TotalCost;
            }

            return true;
        }

        /**
         * A route starting inside a dynamic union may leave it once.  Once it has left, neither an
         * ordinary predecessor edge nor any later endpoint is allowed to enter again.  The snapshot
         * came from the search result, so this validates the same immutable geometry rather than
         * sampling a changed world while formatting the answer.
         */
        auto Get_IsStrictCellRouteMonotonicOutsideDynamicUnion(
            TConstArrayView<FCk_GroundNav_CellRouteEdge>      InEdges,
            const FCk_GroundNav_DynamicObstacleSnapshot&     InDynamicObstacles) -> bool
        {
            // The only legal covered prefix is the one that begins at source. Once a clear segment
            // has appeared, a later ExitsOnce would necessarily have re-entered the union first.
            auto MayStillBeInsideInitialUnion = true;

            for (const auto& Edge : InEdges)
            {
                switch (Edge._Kind)
                {
                    case ECk_GroundNav_CellRouteEdgeKind::Link:
                    {
                        // An authored traversal may cross a covered interior, but it cannot serve as
                        // the initial escape from the source union. Both exact ends must already stand
                        // clear in the immutable search snapshot.
                        if (MayStillBeInsideInitialUnion || Edge._LinkStableId == INDEX_NONE ||
                            (Edge._LinkDirection != ECk_GroundNav_LinkDirection::Forward &&
                             Edge._LinkDirection != ECk_GroundNav_LinkDirection::Backward))
                        { return false; }

                        const auto FromEndpoint = Get_DynamicUnionEdge(
                            InDynamicObstacles, Edge._FromPoint, Edge._FromPoint);
                        const auto ToEndpoint = Get_DynamicUnionEdge(
                            InDynamicObstacles, Edge._ToPoint, Edge._ToPoint);

                        if (NOT FromEndpoint.IsSet() || NOT ToEndpoint.IsSet() ||
                            FromEndpoint.GetValue() != ECk_GroundNav_DynamicUnionEdge::Clear ||
                            ToEndpoint.GetValue() != ECk_GroundNav_DynamicUnionEdge::Clear)
                        { return false; }

                        continue;
                    }
                    case ECk_GroundNav_CellRouteEdgeKind::Ordinary:
                    case ECk_GroundNav_CellRouteEdgeKind::Terminal:
                    { break; }
                    default:
                    { return false; }
                }

                const auto UnionEdge = Get_DynamicUnionEdge(
                    InDynamicObstacles, Edge._FromPoint, Edge._ToPoint);

                if (NOT UnionEdge.IsSet())
                { return false; }

                switch (UnionEdge.GetValue())
                {
                    case ECk_GroundNav_DynamicUnionEdge::Clear:
                    {
                        MayStillBeInsideInitialUnion = false;
                        break;
                    }
                    case ECk_GroundNav_DynamicUnionEdge::InsideAll:
                    {
                        if (NOT MayStillBeInsideInitialUnion)
                        { return false; }
                        break;
                    }
                    case ECk_GroundNav_DynamicUnionEdge::ExitsOnce:
                    {
                        if (NOT MayStillBeInsideInitialUnion)
                        { return false; }
                        MayStillBeInsideInitialUnion = false;
                        break;
                    }
                    case ECk_GroundNav_DynamicUnionEdge::NonMonotonic:
                    default:
                    { return false; }
                }
            }

            return true;
        }

        auto DoStamp_StrictCellLinkWaypoints(
            TConstArrayView<FStrictCellLinkStamp>        InStamps,
            TArray<FCk_GroundNav_PathWaypoint>&          InOutWaypoints) -> bool
        {
            for (const auto& Stamp : InStamps)
            {
                if (NOT InOutWaypoints.IsValidIndex(Stamp._WaypointIndex))
                { return false; }

                auto& Waypoint = InOutWaypoints[Stamp._WaypointIndex];
                Waypoint._LinkId = Stamp._LinkStableId;
                Waypoint._LinkRole = Stamp._Role;
                Waypoint._LinkEntryDirection = Stamp._Direction;
            }

            return true;
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

        /**
         * What the ground under each waypoint is priced at, one entry per point, so a span's budget
         * is a max over a slice rather than a projection re-run per candidate.
         *
         * Read through Get_AreaMultiplier, which is where the field's own baked plate price and the
         * query's table are merged — the same one function the fill prices its legs through, so the
         * budget a chord is judged against and the cost the plan reports cannot come from two rules.
         */
        auto Get_PlateMultipliers(
            TConstArrayView<FVector>            InWaypoints,
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu) -> TArray<float>
        {
            const auto Shared = Make_SharedData(InField, InCost, InAgent);

            auto Multipliers = TArray<float>{};
            Multipliers.Reserve(InWaypoints.Num());

            for (const auto& Location : InWaypoints)
            {
                const auto Attributes = Get_SurfaceAttributesAt(
                    InField, Make_IsNavigableQuery(Location, InAgent, InVerticalToleranceUu));

                Multipliers.Emplace(
                    Get_AreaMultiplier(InField, Shared, Get_FlatPlateOf(InField, Attributes)));
            }

            return Multipliers;
        }

        /**
         * The last point of the span starting at an index: the next PINNED point, or the end of the
         * polyline when nothing is pinned beyond here.
         *
         * Pinned by exact position, the same comparison the corner offset's own rule makes and for
         * the same reason: both points are copies of one resolved link endpoint.
         */
        auto Get_SpanEndIndex(
            TConstArrayView<FVector> InWaypoints,
            TConstArrayView<FVector> InPinnedWaypoints,
            int32                    InFromIndex) -> int32
        {
            for (auto Index = InFromIndex + 1; Index < InWaypoints.Num(); ++Index)
            {
                if (InPinnedWaypoints.Contains(InWaypoints[Index]))
                { return Index; }
            }

            return InWaypoints.Num() - 1;
        }

        /**
         * The cost query a chord and a replaced segment are BOTH priced through, so the two sides of
         * the comparison can only ever differ in their cap.
         *
         * The plate table rides the query and the baked price is merged into it, which is
         * Get_AreaMultiplier's own rule - so a ray through marked ground is charged for it whether or
         * not a waypoint ever stood on that plate.
         */
        auto Make_ChordCostQuery(
            const FVector&                      InFrom,
            const FVector&                      InTo,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu) -> FCk_GroundNav_RaycastQuery
        {
            auto Query = FCk_GroundNav_RaycastQuery{};
            Query._Start = InFrom;
            Query._End = InTo;
            Query._StartVerticalToleranceUu = InVerticalToleranceUu;
            Query._Agent = InAgent;
            Query._PlateCostMultipliers = InCost._PlateCostMultipliers;
            Query._UseBakedPlateCost = true;

            // The chord is judged by the same refusal the search routed under, or a shortcut would
            // cut straight across the ground the corridor exists to avoid - a route the search never
            // priced and never admitted.
            Query._DeniedPlates = InCost._DeniedPlates;

            return Query;
        }

        /**
         * What ONE polyline segment costs under the ray's own per-cell pricing, and unset where the
         * segment's own ray is not clear.
         *
         * Uncapped, so the answer is the segment's price rather than a cap it was measured against.
         * A funnel apex hugs its wall at exactly one radius, so a segment between two of them can be
         * refused on geometry by the same ray the chord is judged with - which is why this answers
         * with an optional rather than a number the caller would have to guess the meaning of.
         */
        auto Get_SegmentRayCostUu(
            const FCk_GroundNav_Field&          InField,
            const FVector&                      InFrom,
            const FVector&                      InTo,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu) -> TOptional<double>
        {
            auto Query = Make_ChordCostQuery(InFrom, InTo, InCost, InAgent, InVerticalToleranceUu);

            // Zero is SurfaceWalk's "no cap" (CapIsActive in Get_SurfaceRaycast), which is what this
            // asks for: the segment's price, not a verdict against a budget.
            Query._MaxCost = 0.0f;

            const auto Result = Get_SurfaceRaycast(InField, Query);

            if (NOT Result.Get_IsClear())
            { return {}; }

            return static_cast<double>(Result._AccumulatedCost);
        }

        /**
         * The ray prices of the ORIGINAL segments of one pass, cast on first use and kept, so a
         * segment spanned by five candidates is cast once rather than five times.
         *
         * The fallback count is how many segments answered with no price at all - the ones the ray
         * budget below had to price the endpoint-max way. It is reported and never asserted: it says
         * how much of the ray budget is really the ray's.
         */
        struct FSegmentRayCosts
        {
            TArray<TOptional<double>> _CostUu;
            TArray<bool>              _Asked;
            int32                     _FallbackCount = 0;
        };

        /**
         * The two numbers a stretch is worth, carried together because a chord is only taken when it
         * is inside both and a caller that could pass one without the other is a caller that can
         * check half the rule.
         */
        struct FStretchBudgets
        {
            double _RayBudgetUu = 0.0;
            double _FillBudgetUu = 0.0;
        };

        /**
         * The two prices of one replaced stretch, because a chord is judged by two different rules
         * and each needs the stretch measured in its own units.
         *
         * The RAY budget is the sum of the segments' own ray prices, which is the same per-cell
         * arithmetic the chord's ray will answer in - so ground no waypoint stands on is charged on
         * both sides. Where a segment's ray is not clear there is no ray price to sum, and it falls
         * back to the endpoint-max price - the rule the whole budget used before.
         *
         * The FILL budget is the sum of Get_LegCost over the same segments, which is the arithmetic
         * the plan PUBLISHES through - so it carries the 3D length and the slope penalty the other
         * two sites do not see.
         */
        auto Get_ReplacedStretchBudgets(
            TConstArrayView<FVector>            InWaypoints,
            TConstArrayView<float>              InMultipliers,
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathSharedData& InShared,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu,
            int32                               InFromIndex,
            int32                               InToIndex,
            FSegmentRayCosts&                   InOutSegmentRayCosts) -> FStretchBudgets
        {
            auto Budgets = FStretchBudgets{};

            for (auto Index = InFromIndex; Index < InToIndex; ++Index)
            {
                const auto& From = InWaypoints[Index];
                const auto& To = InWaypoints[Index + 1];

                const auto Multiplier = FMath::Max(InMultipliers[Index], InMultipliers[Index + 1]);

                if (NOT InOutSegmentRayCosts._Asked[Index])
                {
                    InOutSegmentRayCosts._CostUu[Index] = Get_SegmentRayCostUu(
                        InField, From, To, InCost, InAgent, InVerticalToleranceUu);

                    InOutSegmentRayCosts._Asked[Index] = true;

                    if (NOT InOutSegmentRayCosts._CostUu[Index].IsSet())
                    { ++InOutSegmentRayCosts._FallbackCount; }
                }

                const auto& SegmentRayCostUu = InOutSegmentRayCosts._CostUu[Index];

                Budgets._RayBudgetUu += SegmentRayCostUu.IsSet()
                    ? SegmentRayCostUu.GetValue()
                    : static_cast<double>(Multiplier) * FVector::Dist2D(From, To);

                Budgets._FillBudgetUu += static_cast<double>(
                    Get_LegCost(InShared, From, To, Multiplier, kNoClearanceFactor));
            }

            return Budgets;
        }

        /**
         * Whether a body can walk the chord, and whether the chord is worth what the waypoints it
         * replaces were worth - under BOTH prices, because they answer different questions.
         *
         * (i) The ray's per-cell cost against the ray budget: never pay MORE for the chord than the
         * corridor it replaces cost. That bounds the TOTAL and not the dearness per unit - a short
         * chord across dear ground is admitted against a long cheap detour whenever the two numbers
         * come out that way. (ii) The fill's own price of the chord against the fill budget: never
         * publish a cost higher than the plan already carried. Neither implies the other - the ray
         * samples cells and knows no slope, the fill samples endpoints and does - so a chord must
         * clear both to be taken.
         *
         * A budget of zero REFUSES without casting, and so does one that merely ROUNDS to zero on
         * the way into the float cap. Zero is SurfaceWalk's "no cap" (CapIsActive in
         * Get_SurfaceRaycast), so either would hand the ray an unbounded budget and admit anything;
         * a stretch worth nothing buys nothing.
         */
        auto Get_IsChordWalkableWithinBudget(
            const FCk_GroundNav_Field&          InField,
            const FVector&                      InFrom,
            const FVector&                      InTo,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu,
            const FCk_GroundNav_PathSharedData& InShared,
            float                               InChordMultiplier,
            const FStretchBudgets&              InBudgets) -> bool
        {
            if (InBudgets._RayBudgetUu <= 0.0 || InBudgets._FillBudgetUu <= 0.0)
            { return false; }

            // The number SurfaceWalk reads is this FLOAT, not the double it was narrowed from, so a
            // budget positive in double that rounds to zero here would arrive as the ray's "no cap".
            // Refused on the value the ray will actually see.
            const auto RayCapUu = static_cast<float>(InBudgets._RayBudgetUu * (1.0 + kShortcutBudgetSlack));

            if (RayCapUu <= 0.0f)
            { return false; }

            const auto ChordFillCostUu = static_cast<double>(
                Get_LegCost(InShared, InFrom, InTo, InChordMultiplier, kNoClearanceFactor));

            if (ChordFillCostUu > InBudgets._FillBudgetUu * (1.0 + kShortcutBudgetSlack))
            { return false; }

            auto Query = Make_ChordCostQuery(InFrom, InTo, InCost, InAgent, InVerticalToleranceUu);
            Query._MaxCost = RayCapUu;

            return Get_SurfaceRaycast(InField, Query).Get_IsClear();
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
        Get_WithLinkEndpointsEmitted(
            TConstArrayView<FVector>        InWaypoints,
            const FCk_GroundNav_PathResult& InResult)
        -> TArray<FVector>
    {
        using namespace pathpostprocess_private;

        auto Emitted = TArray<FVector>{};
        Emitted.Append(InWaypoints.GetData(), InWaypoints.Num());

        // The start, which precedes every link endpoint in walk order, so the fallback below has an
        // endpoint to place after before any link point of its own has been placed.
        auto LastPlacedIndex = 0;

        for (const auto& Portal : InResult._FunnelPortals)
        {
            if (Portal._LinkIndex == INDEX_NONE)
            { continue; }

            // Exactly, for the reason the pinned-point rule is exact: the portal's point and the
            // waypoint are both copies of the one resolved endpoint.
            const auto ExistingIndex = Emitted.Find(Portal._Left);

            if (ExistingIndex != INDEX_NONE)
            {
                LastPlacedIndex = ExistingIndex;
                continue;
            }

            // A crossed link's endpoint stands on the corridor, so no leg carrying it would mean the
            // polyline and the portal list disagree. Placing it after the endpoint before it keeps
            // walk order - entry then exit - which is the one property the stamp downstream reads.
            const auto SegmentIndex = Get_SegmentContaining(Emitted, Portal._Left);

            const auto InsertAtIndex = FMath::Min(
                (SegmentIndex == INDEX_NONE ? LastPlacedIndex : SegmentIndex) + 1, Emitted.Num());

            Emitted.Insert(Portal._Left, InsertAtIndex);

            LastPlacedIndex = InsertAtIndex;
        }

        return Emitted;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_Shortcut(
            TConstArrayView<FVector>            InWaypoints,
            TConstArrayView<FVector>            InPinnedWaypoints,
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathCostParams& InCost,
            const FCk_GroundNav_QueryAgent&     InAgent,
            float                               InVerticalToleranceUu,
            int32*                              OutSegmentRayFallbacks)
        -> TArray<FVector>
    {
        using namespace pathpostprocess_private;

        if (OutSegmentRayFallbacks != nullptr)
        { *OutSegmentRayFallbacks = 0; }

        // Two, not one: the candidate loop below runs from the capped index down to two ahead, so a
        // cap of one leaves it empty and the pass would be a silent no-op rather than an off switch.
        const auto PassIsEnabled = InCost._ShortcutSpanCap >= 2;

        if (InWaypoints.Num() < kFewestPointsWithAnInterior || NOT PassIsEnabled)
        {
            auto Unchanged = TArray<FVector>{};
            Unchanged.Append(InWaypoints.GetData(), InWaypoints.Num());

            return Unchanged;
        }

        const auto Multipliers = Get_PlateMultipliers(
            InWaypoints, InField, InCost, InAgent, InVerticalToleranceUu);

        // The same shared data the fill prices its legs through, so the fill budget below and the
        // number the plan publishes are one arithmetic rather than two.
        const auto Shared = Make_SharedData(InField, InCost, InAgent);

        const auto LastIndex = InWaypoints.Num() - 1;

        auto SegmentRayCosts = FSegmentRayCosts{};
        SegmentRayCosts._CostUu.SetNum(LastIndex);
        SegmentRayCosts._Asked.Init(false, LastIndex);

        auto Kept = TArray<FVector>{};
        Kept.Reserve(InWaypoints.Num());
        Kept.Emplace(InWaypoints[0]);

        auto Index = 0;

        while (Index < LastIndex)
        {
            const auto SpanEndIndex = Get_SpanEndIndex(InWaypoints, InPinnedWaypoints, Index);

            // The span's own end capped by the reach, rather than the index plus the reach, which
            // overflows at the unbounded default.
            const auto FarthestIndex =
                Index + FMath::Min(SpanEndIndex - Index, InCost._ShortcutSpanCap);

            // The next point along, which the funnel already proved walkable, so the pass always
            // answers and never answers with something worse than what it was given.
            auto NextIndex = Index + 1;

            for (auto Candidate = FarthestIndex; Candidate >= Index + 2; --Candidate)
            {
                const auto Budgets = Get_ReplacedStretchBudgets(
                    InWaypoints,
                    Multipliers,
                    InField,
                    Shared,
                    InCost,
                    InAgent,
                    InVerticalToleranceUu,
                    Index,
                    Candidate,
                    SegmentRayCosts);

                const auto ChordMultiplier = FMath::Max(Multipliers[Index], Multipliers[Candidate]);

                if (Get_IsChordWalkableWithinBudget(
                        InField,
                        InWaypoints[Index],
                        InWaypoints[Candidate],
                        InCost,
                        InAgent,
                        InVerticalToleranceUu,
                        Shared,
                        ChordMultiplier,
                        Budgets))
                {
                    NextIndex = Candidate;
                    break;
                }
            }

            Kept.Emplace(InWaypoints[NextIndex]);

            Index = NextIndex;
        }

        if (OutSegmentRayFallbacks != nullptr)
        { *OutSegmentRayFallbacks = SegmentRayCosts._FallbackCount; }

        return Kept;
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

            if (Index == 0 && InWaypoints.Num() > 1 && SkipFirstThresholdSquared > 0.0f &&
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

        if (InResult._RouteKind == ECk_GroundNav_PathRouteKind::StrictCell)
        {
            auto LinkStamps = TArray<FStrictCellLinkStamp>{};
            auto LegCosts = TArray<FStrictCellLegCost>{};
            const auto Locations = Get_StrictCellLocations(InResult, LinkStamps, LegCosts);

            // Keep the resolved source and goal rows even when the body already occupies the source.
            // They are the exact endpoints the strict graph priced, and dropping the first would make
            // both the published distance and the one-time dynamic-union escape answer about a shorter
            // route than the search actually proved.
            if (NOT Locations.IsSet() || Locations->Num() < 2 ||
                NOT Get_IsStrictCellRouteMonotonicOutsideDynamicUnion(InResult._CellRoute, InResult._DynamicObstacles))
            {
                Plan._Status = ECk_GroundNav_PathStatus::Blocked;
                Plan._PlateCorridor.Reset();
                return Plan;
            }

            Plan._Waypoints = Get_FilledWaypoints(
                *Locations, InField, InParams._Cost, InParams._Agent, InParams._VerticalToleranceUu);
            if (NOT DoApply_StrictCellLegCosts(LegCosts, Plan._Waypoints))
            {
                Plan._Status = ECk_GroundNav_PathStatus::Blocked;
                Plan._Waypoints.Reset();
                Plan._PlateCorridor.Reset();
                return Plan;
            }
            if (NOT DoStamp_StrictCellLinkWaypoints(LinkStamps, Plan._Waypoints))
            {
                Plan._Status = ECk_GroundNav_PathStatus::Blocked;
                Plan._Waypoints.Reset();
                Plan._PlateCorridor.Reset();
                return Plan;
            }
            Plan._LengthUu = Plan._Waypoints.Last()._DistanceFromStart;
            return Plan;
        }

        const auto RadiusUu = InParams._Agent._RadiusUu;

        auto Funnelled = TArray<FVector>{};
        Get_Funnelled(InResult, RadiusUu, Funnelled);

        const auto Emitted = Get_WithLinkEndpointsEmitted(Funnelled, InResult);

        const auto Pinned = Get_LinkWaypoints(InResult);

        const auto Offset = Get_CornerOffset(
            Emitted,
            Pinned,
            InField,
            InParams._Cost._CornerOffsetK * RadiusUu,
            InParams._Agent,
            InParams._VerticalToleranceUu);

        // AFTER the offset, and that is measured rather than preferred: the funnel's apexes hug their
        // walls at exactly one radius, so a chord between two of them passes an obstacle standing
        // between at just under a radius and the radius-aware ray refuses it (the four-pillar slab
        // kept its false corner that way). Offset first and the same chord clears by a further radius.
        // A false corner the offset pushed out costs nothing - the shortcut drops it whole.
        const auto Shortcut = Get_Shortcut(
            Offset,
            Pinned,
            InField,
            InParams._Cost,
            InParams._Agent,
            InParams._VerticalToleranceUu);

        const auto Trimmed = Get_SkipFirstWaypoint(Shortcut, InParams._AgentLocation, RadiusUu);

        Plan._Waypoints = Get_FilledWaypoints(
            Trimmed,
            InField,
            InParams._Cost,
            InParams._Agent,
            InParams._VerticalToleranceUu);

        DoStamp_LinkWaypoints(InResult, InField, Plan._Waypoints);

        Plan._LengthUu = Plan._Waypoints.IsEmpty()
            ? 0.0
            : Plan._Waypoints.Last()._DistanceFromStart;

        return Plan;
    }
}

// --------------------------------------------------------------------------------------------------------------------
