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
// stage can be run alone by a test or a debug view, and the composed answer is exactly the six
// stages in order and nothing else.
//
// The plan is a SEPARATE value from FCk_GroundNav_PathResult on purpose. A result is rewritten every
// slice a sliced search runs; a plan is computed once, after a terminal status, and would otherwise
// be visible half-built to anybody reading the result mid-slice.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Which end of an authored link a waypoint stands on, and None for every point the lattice put
     * there.
     *
     * Unreflected and module-local for the same reason the waypoint it rides on is: the plan is an
     * internal value, and the reflected twin a consumer reads is minted where the plan is flattened.
     */
    enum class ECk_GroundNav_LinkWaypointRole : uint8
    {
        None,
        Entry,
        Exit
    };

    // ----------------------------------------------------------------------------------------------------------------

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

        // The STABLE authored id of the link this point is an end of, and INDEX_NONE for a point no
        // link put here. The id rather than the field-local index because a plan outlives the field it
        // was made against, and that index would name a different link after the next derive.
        int32 _LinkId = INDEX_NONE;

        ECk_GroundNav_LinkWaypointRole _LinkRole = ECk_GroundNav_LinkWaypointRole::None;

        // Which way the link is being walked, carried on BOTH of its points so the exit answers what
        // its entry did. Bidirectional is what a point of no link reads: a traversal has one direction.
        ECk_GroundNav_LinkDirection _LinkEntryDirection = ECk_GroundNav_LinkDirection::Bidirectional;
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
     * Every endpoint of an authored link the corridor crossed, present in the polyline.
     *
     * The funnel emits an apex only where the string BENDS, so a link whose two degenerate portals lie
     * on the line the apex already sees leaves no waypoint for either of them. The stamp downstream
     * recognises a link endpoint by its exact position, so a route that provably crossed a link would
     * carry no waypoint saying so. An authored endpoint is a place a body must pass THROUGH rather
     * than a corner it hugs, which is why it is put back rather than left to the funnel's bend rule.
     *
     * An absent endpoint is inserted on the segment carrying it - collinear in XY, which is the
     * arithmetic the funnel itself string-pulls in, and between that segment's ends by its own
     * dot-product parameter. Portals are walked in corridor order, so an entry is placed before its
     * exit and the two are adjacent wherever the segment between them carries nothing else.
     *
     * A route whose corridor crossed no link comes back element for element: the pass inserts only
     * where a link portal's endpoint is MISSING, and a corridor with no link portals has none to miss.
     */
    CKGROUNDNAV_API auto
    Get_WithLinkEndpointsEmitted(
        TConstArrayView<FVector>        InWaypoints,
        const FCk_GroundNav_PathResult& InResult) -> TArray<FVector>;

    /**
     * Every waypoint a straight walkable line makes unnecessary, dropped: from each point it keeps,
     * the farthest point that point can both SEE and AFFORD, and everything between the two gone.
     *
     * The funnel is taut over the corridor it was HANDED, and a decomposition into rectangles hands
     * it corners that are artefacts of where a slab was cut rather than of anything a body must walk
     * around. A surface raycast is the only thing that can tell those two apart, because it re-asks
     * the field instead of the corridor: a corner a ray walks straight through was never a corner.
     *
     * A candidate is AFFORDED when it is inside BOTH of the prices the stretch it replaces carries,
     * rather than a plate-set membership test: leaving the corridor's own plates is exactly what a
     * shortcut must be allowed to do, while paying more for the ground than the detour it removes is
     * exactly what it must not.
     *
     * (i) The chord's raycast, priced per cell, against the sum of the replaced segments' OWN
     * uncapped raycasts - so ground no waypoint stands on is charged on both sides. The bound is on
     * the TOTAL, never on dearness per unit: a short chord across dear ground is admitted against a
     * long cheap detour whenever the two numbers come out that way, and what (i) forbids is paying
     * MORE for the chord than the corridor it replaces cost. Each original segment is cast at most
     * once per pass. A segment whose own ray is not clear - a funnel apex hugs its wall at exactly
     * one radius, so this happens - has no ray price to give, and falls back to its XY length times
     * the greater of its endpoints' plate multipliers: the rule the whole budget used before.
     *
     * (ii) Get_LegCost over the chord against the sum of Get_LegCost over the replaced segments -
     * the arithmetic the fill PUBLISHES through, which is the only one of the three that carries the
     * 3D length and the slope penalty. That makes the plan's _CostFromStart monotone across this
     * pass by construction rather than by hope.
     *
     * A budget of zero REFUSES, without a ray. Zero is the raycast's own "no cap", so a stretch that
     * cost nothing would hand the ray an unbounded budget and admit anything; a stretch worth nothing
     * buys nothing.
     *
     * Link endpoints are hard span SPLITS: never a candidate, never crossed, never dropped. An
     * authored endpoint is a place a body must pass THROUGH, so a chord spanning one would walk the
     * route around a link the search decided to take. The two ends of the polyline are pinned for the
     * same reason.
     *
     * Idempotent at the unbounded cap, to within the rounding slack, and now for a reason that can be
     * stated. Take either conjunct alone. Its budget is a SUM over segments of one function - the
     * segment's own uncapped raycast for (i), Get_LegCost for (ii) - so it is additive over
     * sub-stretches, and the chord under test is priced by that SAME function. A second run's spans
     * are delimited by the same pinned points, and every segment a second run is handed is one of two
     * things. Either it is a chord the first run ACCEPTED - and then its capped ray came back clear,
     * so its uncapped ray is clear too and prices it at the very number the first run measured
     * against the sub-stretch it replaced, with no fallback reachable for it - or it is an ORIGINAL
     * segment the first run kept, which both runs price by casting the same ray between the same two
     * points. Either way the second run's budget for any span is no larger than the first's, and the
     * chord's own price does not depend on which run offers it. So a first-run refusal under a
     * conjunct is a second-run refusal under that same conjunct, and a conjunction of two idempotent
     * accept rules is idempotent. A FINITE _ShortcutSpanCap trades the property away outright, which
     * is why unbounded is the default: a shorter list puts points inside a reach the first run never
     * looked as far as.
     *
     * OutSegmentRayFallbacks, when given, answers how many ORIGINAL segments took the fallback in (i).
     * It is a diagnostic for how much of the ray budget is really the ray's, and nothing reads it to
     * decide anything.
     */
    CKGROUNDNAV_API auto
    Get_Shortcut(
        TConstArrayView<FVector>            InWaypoints,
        TConstArrayView<FVector>            InPinnedWaypoints,
        const FCk_GroundNav_Field&          InField,
        const FCk_GroundNav_PathCostParams& InCost,
        const FCk_GroundNav_QueryAgent&     InAgent,
        float                               InVerticalToleranceUu,
        int32*                              OutSegmentRayFallbacks = nullptr) -> TArray<FVector>;

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
     *
     * A waypoint EXACTLY equal to one of the pinned points is emitted where it is. Those are the
     * endpoints an authored link put on the route, and a link endpoint is not a corner the string
     * bent around: pushing it along a bisector would walk it off the link. Compared exactly rather
     * than within an epsilon because both points are copies of the one resolved endpoint.
     */
    CKGROUNDNAV_API auto
    Get_CornerOffset(
        TConstArrayView<FVector>        InWaypoints,
        TConstArrayView<FVector>        InPinnedWaypoints,
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
     *
     * Never empties a non-empty route: a single-point route keeps its one point rather than dropping
     * it, because no listener ever sees an empty waypoint list published as Ready.
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
     * The whole post-process: plate-portal results run funnel, link endpoints, corner offset,
     * shortcut, skip-first, and fill in that order. Strict-cell results retain the collector's exact
     * source-to-goal predecessor rows, their per-leg prices, and their link endpoints; they do not
     * reshape a line the strict graph already proved against its immutable dynamic snapshot.
     *
     * The shortcut runs AFTER the corner offset. The funnel's apexes hug their walls at exactly one
     * radius, so a chord between two of them passes an obstacle standing between them at just under
     * a radius and the radius-aware ray refuses it; offset first and that chord clears. A false
     * corner the offset pushed a radius further out costs nothing, because the shortcut drops it
     * whole rather than having to undo it.
     *
     * The corner offset is the cost model's own multiple of the agent radius, so one number tunes the
     * pass and zero switches it off.
     *
     * The link stamp is applied AFTER the fill, not at the funnel: the passes in between speak
     * locations and not waypoints, so an index taken earlier would name a different point by the time
     * the fill runs. A filled waypoint EXACTLY equal to a link portal's endpoint is that link's, the
     * same exact comparison the corner offset's pinned-point rule already makes.
     */
    CKGROUNDNAV_API auto
    Get_PathPlan(
        const FCk_GroundNav_PathResult&     InResult,
        const FCk_GroundNav_Field&          InField,
        const FCk_GroundNav_PathPostParams& InParams) -> FCk_GroundNav_PathPlan;
}

// --------------------------------------------------------------------------------------------------------------------
