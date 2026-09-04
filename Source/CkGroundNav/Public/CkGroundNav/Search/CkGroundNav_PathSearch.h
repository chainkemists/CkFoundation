#pragma once

#include "CkAStar/Algorithm/CkAStar_Search.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Time/CkTime.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include <CoreMinimal.h>

#include "CkGroundNav_PathSearch.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// One path plan, from the checks that answer it without searching to the slices that answer it by
// searching.
//
// The cheap answers come first and in one fixed order — no field, no room for the body, no ground
// under either end, two ends the labels prove apart, two ends on one plate — because each of them is
// a reason the search would have been wasted, and because a consumer distinguishes them: it waits on
// unbuilt ground and gives up on ground with nowhere to stand.
//
// SLICING CHANGES NOTHING BUT WHERE THE WORK STOPS. A sliced run expands the same nodes in the same
// order as a one-shot run and answers with the same corridor, cost and expansion count; the two caps
// this driver owns are tested after every slice returns, including the one slice a one-shot run
// takes, so no verdict can depend on where a boundary fell.
// --------------------------------------------------------------------------------------------------------------------

/**
 * What a repair did with the corridor it was handed.
 *
 * It is a verdict on the PLAN, not on the search: a caller reads it to know whether an agent's
 * route survived a rebuild and how much of it did, and so whether a repair earned its name or a
 * full replan was paid for under it. None is the answer wherever no corridor was offered at all,
 * which every cold begin is.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_RepairVerdict : uint8
{
    None,
    StillValid,
    Repaired,
    FullReplan
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_RepairVerdict);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * What one call to the search may spend.
     *
     * Both ceilings are per-slice and both are off at zero, so a default-constructed slice runs to a
     * terminal status in one call — which is what makes the one-shot form the sliced form with no
     * limits rather than a second driver.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathSliceParams
    {
    public:
        // Expansions this slice may make. Zero means as many as it takes.
        int32 _MaxIterations = 0;

        // Wall-clock this slice may spend. Zero means as long as it takes.
        FCk_Time _Budget;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A search a caller keeps across frames.
     *
     * The graph is held beside the search rather than only inside it, because CkAStar takes the graph
     * BY VALUE and offers no way to read it back — and everything the search discovered that a
     * corridor is made of (the crossings, what the field cost to read, which node came closest) lives
     * in the pool the two copies share.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathSearch
    {
    public:
        /**
         * Answer the query outright, or stand a search up for the slices that will.
         *
         * The returned status is the result's status: a terminal one means the answer is already in
         * Get_Result() and ContinueSearch has nothing left to do.
         */
        auto Request_Begin(
            const FCk_GroundNav_FieldPtr&    InField,
            const FCk_GroundNav_PathQuery&   InQuery) -> ECk_GroundNav_PathStatus;

        /**
         * Answer the query with as much of a corridor kept from an earlier plan as still holds.
         *
         * The corridor arrives as CROSSING KEYS rather than node ids, because node ids are minted per
         * search into a pool no other search shares. The keys are walked back onto the graph over the
         * field given here, and what survives that walk decides the work: a corridor that re-resolves
         * whole against the epoch it was planned for is answered without expanding anything, one that
         * re-resolves in part seeds the search from where it stopped holding, and one that does not
         * leave the plate the start now stands on is a cold search with a name for why.
         *
         * The same pre-search checks answer it as answer Request_Begin, so no cheap refusal is
         * reachable through one entry point and not the other. Get_RepairVerdict() says which of the
         * three happened.
         */
        auto Request_BeginRepair(
            const FCk_GroundNav_FieldPtr&              InField,
            const FCk_GroundNav_PathQuery&             InQuery,
            TConstArrayView<FCk_GroundNav_CrossingKey> InExistingCorridor,
            FCk_GroundNav_Epoch                        InPlannedAgainstEpoch) -> ECk_GroundNav_PathStatus;

        /** One slice of work. A search that has already stopped answers with the status it stopped at. */
        auto ContinueSearch(
            const FCk_GroundNav_PathSliceParams& InSlice) -> ECk_GroundNav_PathStatus;

    public:
        auto Get_Result() const -> const FCk_GroundNav_PathResult& { return _Result; }

        /**
         * The corridor in the one form that outlives this search.
         *
         * A caller keeping a route for a later repair stores these rather than the crossings, so it
         * never has to know how a door is keyed.
         */
        auto Get_CorridorKeys() const -> TArray<FCk_GroundNav_CrossingKey>;

        auto Get_RepairVerdict() const -> ECk_GroundNav_RepairVerdict { return _RepairVerdict; }

        auto Get_Status() const -> ECk_GroundNav_PathStatus { return _Result._Status; }

        auto Get_IsTerminal() const -> bool
        {
            return _Result._Status != ECk_GroundNav_PathStatus::InProgress;
        }

    private:
        /** What the checks before the search left for the search to do, and the two plates it needs. */
        struct FSeedResult
        {
        public:
            ECk_GroundNav_PathStatus _Status = ECk_GroundNav_PathStatus::InProgress;

            int32 _StartFlatPlate = INDEX_NONE;
            int32 _GoalFlatPlate = INDEX_NONE;
        };

        /**
         * Everything both entry points do before either of them decides how to seed.
         *
         * It resets the search, answers whatever the cheap checks can answer outright, and on
         * InProgress leaves the shared seed built and the two plates named. One body, so a refusal
         * cannot be reachable through one entry point and not the other.
         */
        auto DoRun_PreSearch(
            const FCk_GroundNav_FieldPtr&  InField,
            const FCk_GroundNav_PathQuery& InQuery) -> FSeedResult;

        /** A search over a fresh pool, opened at the source and nowhere else. */
        auto DoSeed_Cold(
            int32 InStartFlatPlate) -> void;

        auto DoSet_Status(
            ECk_GroundNav_PathStatus InStatus) -> ECk_GroundNav_PathStatus;

        auto DoResolve_End(
            const FCk_GroundNav_Field& InField,
            const FVector&             InLocation) const -> FCk_GroundNav_IsNavigableResult;

        auto DoExtract_Corridor() -> bool;

        /** The corridor to the node the search came closest to the goal with, and what it cost. */
        auto DoExtract_PartialCorridor(
            FCk_GroundNav_PathNodeId InBestNode) -> void;

        /**
         * The three corridor shapes, derived once from one walk order.
         *
         * InNodes begins at the source node, which arrives through no crossing and so contributes
         * the start plate and nothing else.
         */
        auto DoBuild_Corridor(
            TConstArrayView<FCk_GroundNav_PathNodeId> InNodes) -> void;

        /**
         * The step from the last door onto the goal point.
         *
         * The search prices every leg but this one: its goal is a plate, so the walk stops at the door
         * into that plate and the step from there is nobody's edge.
         */
        auto DoGet_FinalLegCost(
            FCk_GroundNav_PathNodeId InLastNode) const -> float;

        /** Whether a search that stopped short of the goal may answer with what it did reach. */
        auto DoGet_PartialIsAvailable() const -> bool;

        auto DoRefresh_Cost() -> void;

        /**
         * What this call may spend, so that a run sliced any way at all expands the same nodes in
         * the same order as a run that was never sliced: the cap is spent down to the expansion, and
         * ONE more is allowed once it is reached, because a goal pop is not itself counted and a
         * search standing exactly on its cap may still be one pop from an answer.
         */
        auto DoGet_AllowedIterations(
            int32 InRequested) const -> int32;

        auto DoGet_HasExceededExpansionCap() const -> bool;

    private:
        FCk_GroundNav_PathQuery _Query;

        // The seed every copy of the graph reads, held here too because the legs the search never
        // prices — the step onto the goal, and a query whose two ends share one plate — must be
        // priced with the same numbers the route was chosen by.
        TSharedPtr<const FCk_GroundNav_PathSharedData> _Shared;

        FCk_GroundNav_PlatePortalGraph _Graph;

        astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph> _Search;

        FCk_GroundNav_PathResult _Result;

        ECk_GroundNav_RepairVerdict _RepairVerdict = ECk_GroundNav_RepairVerdict::None;

        // What the checks before the search cost, kept apart so the graph's running total can be
        // folded in after every slice without being counted twice.
        FCk_GroundNav_QueryCost _PreSearchCost;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The whole plan in one call: the same driver with no slice limits.
     *
     * It is the form the sliced form is measured against — same status, same corridor, same cost,
     * same expansion count — so it is deliberately not a second implementation of any of it.
     */
    CKGROUNDNAV_API auto
    Get_Path(
        const FCk_GroundNav_FieldPtr&  InField,
        const FCk_GroundNav_PathQuery& InQuery) -> FCk_GroundNav_PathResult;
}

// --------------------------------------------------------------------------------------------------------------------
