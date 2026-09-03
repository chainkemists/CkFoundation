#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------
// What turns a corridor into a line a body can walk: the funnel, then the passes that shape its
// output, then the fill that measures it.
//
// Pure functions over a field the caller already holds. No world, no registry, no state — so any
// stage can be run alone by a test or a debug view, and the composed answer is exactly the four
// stages in order and nothing else.
//
// The plan is a SEPARATE value from FCk_GroundNav_PathResult on purpose. A result is rewritten every
// slice a sliced search runs; a plan is computed once, after a terminal status, and would otherwise
// be visible half-built to anybody reading the result mid-slice.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * One point of a walkable line, with everything a follower would otherwise re-derive by asking
     * the field again at the same position.
     *
     * The distance is XY, matching what the funnel itself returns, so a consumer integrating along
     * the waypoints and a consumer reading the string-pull length are reading one arithmetic rather
     * than two that drift apart on slopes.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathWaypoint
    {
    public:
        FVector _Location = FVector::ZeroVector;

        // Normalized in 3D toward the next waypoint; zero on the last, which has no next.
        FVector _DirectionToNext = FVector::ZeroVector;

        double _DistanceFromStart = 0.0;

        // The graph's own price of the walk to here, so a follower and the search agree on what a
        // detour was worth without the follower knowing the cost model.
        double _CostFromStart = 0.0;

        FVector _SurfaceNormal = FVector::UpVector;

        FGameplayTagContainer _AreaTags;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A finished plan: self-contained, so nothing downstream needs the result it came from.
     *
     * A status that is neither Ready nor Partial answers with no waypoints and no corridor. What a
     * caller does with the waypoints it already had is the caller's decision, made where the plan is
     * installed and not here.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathPlan
    {
    public:
        ECk_GroundNav_PathStatus _Status = ECk_GroundNav_PathStatus::NoStartSurface;

        TArray<FCk_GroundNav_PathWaypoint> _Waypoints;

        TArray<int32> _PlateCorridor;

        // The last waypoint's _DistanceFromStart, and zero when there are none.
        double _LengthUu = 0.0;

        FCk_GroundNav_Epoch _PlannedAgainstEpoch;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Everything the post-process needs that the result does not already carry.
     *
     * The agent location is where the body actually stands, which is not the plan's start point: the
     * start was resolved onto a surface before the search ran, and the first waypoint is dropped
     * against where the body is now.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathPostParams
    {
    public:
        FCk_GroundNav_QueryAgent _Agent;

        float _VerticalToleranceUu = 0.0f;

        FVector _AgentLocation = FVector::ZeroVector;

        FCk_GroundNav_PathCostParams _Cost;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The result's corridor, string-pulled between the two resolved ends, returning the XY length.
     *
     * The radius inset lives inside the funnel; there is no second inset here and adding one would
     * pull the line off ground the search already proved admissible.
     */
    CKGROUNDNAV_API auto
    Get_Funnelled(
        const FCk_GroundNav_PathResult& InResult,
        float                           InRadiusUu,
        TArray<FVector>&                OutWaypoints) -> double;

    /**
     * Every interior waypoint pushed off its corner along the interior bisector, endpoints untouched.
     *
     * The funnel emits an interior waypoint only where the string bends around a portal vertex, so
     * every one of them is an inside corner already sitting exactly a radius from the wall; pushing
     * along the bisector moves it into the free space rather than around the geometry.
     *
     * A moved point is accepted only when the field still calls it navigable AND the nearest boundary
     * is at least a radius away. A refusal halves the offset and asks again, four times, and then
     * keeps the un-offset point — which the funnel already proved legal, so the pass always answers
     * and never answers with something worse than what it was given. A degenerate bisector, which a
     * funnel cannot emit, leaves the waypoint alone.
     */
    CKGROUNDNAV_API auto
    Get_CornerOffset(
        TConstArrayView<FVector>        InWaypoints,
        const FCk_GroundNav_Field&      InField,
        float                           InOffsetUu,
        const FCk_GroundNav_QueryAgent& InAgent,
        float                           InVerticalToleranceUu) -> TArray<FVector>;

    /**
     * The first waypoint dropped when the body is already standing on top of it.
     *
     * Deliberately the same threshold as the Recast path takes — twice the radius, index zero only,
     * disabled at a radius of zero — because a body switching between the two providers must not
     * change which waypoint it steers at first.
     */
    CKGROUNDNAV_API auto
    Get_SkipFirstWaypoint(
        TConstArrayView<FVector> InWaypoints,
        const FVector&           InAgentLocation,
        float                    InAgentRadiusUu) -> TArray<FVector>;

    /**
     * Locations turned into waypoints: the surface answer at each point, the running XY distance, and
     * the running graph cost of the polyline.
     *
     * A segment can straddle two plates. It is priced at the greater of the two endpoints' plate
     * multipliers, which is the cost model's own rule for overlapping policy and costs two lookups
     * rather than a projection that could disagree with what the search itself charged.
     */
    CKGROUNDNAV_API auto
    Get_FilledWaypoints(
        TConstArrayView<FVector>            InWaypoints,
        const FCk_GroundNav_Field&          InField,
        const FCk_GroundNav_PathCostParams& InCost,
        const FCk_GroundNav_QueryAgent&     InAgent,
        float                               InVerticalToleranceUu) -> TArray<FCk_GroundNav_PathWaypoint>;

    /**
     * Several plate-multiplier tables merged, greater wins, so overlapping policy has one answer.
     *
     * Pure and free-standing because the merge rule must be the same one whether a test writes the
     * tables by hand or a compiler builds them from markup.
     */
    CKGROUNDNAV_API auto
    Get_MaxMerged(
        TConstArrayView<TMap<int32, float>> InTables) -> TMap<int32, float>;

    /**
     * The whole post-process: funnel, corner offset, skip-first, fill — in that order, with the
     * status, the plate corridor and the planned-against epoch carried through.
     *
     * The corner offset is the cost model's own multiple of the agent radius, so one number tunes the
     * pass and zero switches it off.
     */
    CKGROUNDNAV_API auto
    Get_PathPlan(
        const FCk_GroundNav_PathResult&     InResult,
        const FCk_GroundNav_Field&          InField,
        const FCk_GroundNav_PathPostParams& InParams) -> FCk_GroundNav_PathPlan;
}

// --------------------------------------------------------------------------------------------------------------------
