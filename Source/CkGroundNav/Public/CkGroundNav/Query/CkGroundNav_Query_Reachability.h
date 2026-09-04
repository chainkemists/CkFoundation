#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THREAD CONTRACT: every function here is safe on any thread against the immutable field snapshot
// the caller holds. No statics, no shared scratch, no lazily built index; the only allocations are
// into the caller's own result and locals.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    // ----------------------------------------------------------------------------------------------------------------
    // Flat plate addressing — the index space of the reachability labels.
    // ----------------------------------------------------------------------------------------------------------------

    CKGROUNDNAV_API auto
    Get_FlatPlateIndex(
        const FCk_GroundNav_Field& InField,
        int32                      InTileIndex,
        int32                      InPlateIndex) -> int32;

    /** False for an index no plate of the field has. */
    CKGROUNDNAV_API auto
    Get_TileAndPlate(
        const FCk_GroundNav_Field& InField,
        int32                      InFlatPlate,
        int32&                     OutTileIndex,
        int32&                     OutPlateIndex) -> bool;

    CKGROUNDNAV_API auto
    Get_FlatPlateCount(
        const FCk_GroundNav_Field& InField) -> int32;

    /**
     * Every crossing that leaves a plate: its tile's own portals on either side of the plate, then
     * the seam portals of the field that have it on either side, then the field's resolved links
     * that leave it — in that order, by index within each block, and forward before backward for one
     * bidirectional link. Each is oriented AWAY from the plate.
     *
     * The ORDER is part of the contract: a sliced search expands node for node like a one-shot one
     * only because the push sequence, and so the heap layout that breaks equal scores, is identical.
     *
     * Appends to the caller's array; bills one read per portal and per link examined.
     */
    CKGROUNDNAV_API auto
    Get_CrossingsFrom(
        const FCk_GroundNav_Field&      InField,
        int32                           InFlatPlate,
        TArray<FCk_GroundNav_Crossing>& OutCrossings,
        FCk_GroundNav_QueryCost&        InOutCost) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Whether two points can possibly be joined, read off the component labels in constant time.
     *
     * A label is connectivity for a body of NO size: it says a chain of crossings exists, not that
     * any of them is wide enough for the body asking. So an equal label is PossiblyReachable and never
     * a promise; only the flood fill, which admits crossings by clearance, can turn it into a distance.
     * Different labels are Unreachable when both components are closed, and Unknown when either
     * borders unbaked ground. Expands nothing: _ExpansionCount is always zero.
     */
    CKGROUNDNAV_API auto
    Get_IsReachable(
        const FCk_GroundNav_Field&            InField,
        const FCk_GroundNav_ReachabilityQuery& InQuery) -> FCk_GroundNav_ReachabilityResult;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Dijkstra over the crossings from a source point, where each settled crossing carries the true
     * string-pulled distance from the source (Get_StringPull_ToSegment over its predecessor chain),
     * not a sum of portal centres. A crossing is admitted when its clearance covers the body's radius.
     * Stops when the frontier is empty, when _MaxDistanceUu or _MaxExpansions is reached, or when the
     * stop predicate answers true for the crossing about to be settled (it is then not settled).
     */
    CKGROUNDNAV_API auto
    Get_FloodFill(
        const FCk_GroundNav_Field&                            InField,
        const FCk_GroundNav_FloodQuery&                       InQuery,
        TFunctionRef<bool(const FCk_GroundNav_FloodCrossing&)> InShouldStop) -> FCk_GroundNav_FloodResult;

    /** The overload with no predicate: the query's own limits are the only early exit. */
    CKGROUNDNAV_API auto
    Get_FloodFill(
        const FCk_GroundNav_Field&      InField,
        const FCk_GroundNav_FloodQuery& InQuery) -> FCk_GroundNav_FloodResult;

    /**
     * The walked distance from the flood's source to a point, or unset when the point's plate was not
     * reached (or the point is on no surface). On the source plate it is a straight line — a plate is a
     * convex rectangle, so the line stays on it. Elsewhere it is the least, over every settled crossing
     * entering the point's plate, of the funnel from the source through that crossing's chain to the
     * point.
     */
    CKGROUNDNAV_API auto
    Get_FloodDistanceTo(
        const FCk_GroundNav_Field&       InField,
        const FCk_GroundNav_FloodResult& InFlood,
        const FVector&                   InTarget,
        float                            InVerticalToleranceUu,
        const FCk_GroundNav_QueryAgent&  InAgent) -> TOptional<double>;

    /** One-to-many: Get_FloodDistanceTo for each target, in order. */
    CKGROUNDNAV_API auto
    Get_FloodDistancesTo(
        const FCk_GroundNav_Field&       InField,
        const FCk_GroundNav_FloodResult& InFlood,
        TConstArrayView<FVector>         InTargets,
        float                            InVerticalToleranceUu,
        const FCk_GroundNav_QueryAgent&  InAgent,
        TArray<TOptional<double>>&       OutDistances) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
