#pragma once

#include "CkAStar/Algorithm/CkAStar_Search.h"

#include "CkCore/Time/CkTime.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include <CoreMinimal.h>

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

        /** One slice of work. A search that has already stopped answers with the status it stopped at. */
        auto ContinueSearch(
            const FCk_GroundNav_PathSliceParams& InSlice) -> ECk_GroundNav_PathStatus;

    public:
        auto Get_Result() const -> const FCk_GroundNav_PathResult& { return _Result; }

        auto Get_Status() const -> ECk_GroundNav_PathStatus { return _Result._Status; }

        auto Get_IsTerminal() const -> bool
        {
            return _Result._Status != ECk_GroundNav_PathStatus::InProgress;
        }

    private:
        auto DoSet_Status(
            ECk_GroundNav_PathStatus InStatus) -> ECk_GroundNav_PathStatus;

        auto DoResolve_End(
            const FCk_GroundNav_Field& InField,
            const FVector&             InLocation) const -> FCk_GroundNav_IsNavigableResult;

        auto DoExtract_Corridor() -> bool;

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

        FCk_GroundNav_PlatePortalGraph _Graph;

        astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph> _Search;

        FCk_GroundNav_PathResult _Result;

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
