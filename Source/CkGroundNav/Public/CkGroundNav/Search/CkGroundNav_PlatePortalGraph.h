#pragma once

#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The A* search space over one published field: the plates, joined by the crossings that leave them.
//
// It is the flood fill's walk with distance semantics instead of Dijkstra's — the same node (a
// crossing arrival), the same enumeration (Get_CrossingsFrom), the same admission rule
// (Get_IsAdmitted) and the same back-edge skip. A second definition of "which door does this route
// go through" is what the shared funnel exists to prevent, so there is not one here either.
//
// LIFETIME: unlike the single-tick octree and route graphs, this one holds the field by shared
// pointer. A sliced search outlives a rebuild by construction — the publisher swaps its pointer and
// the search keeps reading the snapshot it started on, with the epoch stamp telling the installer
// which field it planned against.
//
// COPIES: CkAStar's TSearchState takes the graph BY VALUE into a private member it exposes no way to
// read back. Everything the search discovers therefore lives in a node pool the graph holds by
// SHARED pointer, so the copy inside the search and the copy the driver kept are two views of one
// pool: the driver reads the crossings, the accumulated query cost and the closest-node memo off its
// own copy after each slice. One graph, its copies, and one search: never two searches over one.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The canonical identity of a crossing, keyed exactly the way the flood fill keys it.
     *
     * A plate is legitimately entered through several crossings and each one is its own node; only
     * the same interval leaving the same plate is a duplicate. The endpoints compare EXACTLY because
     * both sides come from the same arithmetic over the same portal — a tolerance here would merge
     * two doors that a body can tell apart.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_CrossingKey
    {
    public:
        int32 _FromFlatPlate = INDEX_NONE;
        int32 _ToFlatPlate = INDEX_NONE;
        int32 _Direction = 0;

        FVector _Left = FVector::ZeroVector;
        FVector _Right = FVector::ZeroVector;

    public:
        auto operator==(const FCk_GroundNav_CrossingKey&) const -> bool = default;
    };

    CKGROUNDNAV_API auto
    GetTypeHash(const FCk_GroundNav_CrossingKey& InKey) -> uint32;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The key a crossing is canonicalised under, in the one place that decides it.
     *
     * The node pool keys with this, and a caller that kept a corridor across a rebuild re-resolves it
     * with the same builder, so a stored door and a live one can never be keyed two ways.
     */
    CKGROUNDNAV_API auto
    Make_CrossingKey(
        const FCk_GroundNav_Crossing& InCrossing) -> FCk_GroundNav_CrossingKey;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The per-query seed: everything a search needs that does not vary with the node being expanded.
     *
     * Immutable once built, so every copy of the graph shares it rather than duplicating it. The
     * agent's radius is the ONLY clearance number here — it is what Get_IsAdmitted tests a crossing
     * against and what the funnel insets by, and a second minimum beside it would be a second answer
     * to the same question.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathSharedData
    {
    public:
        // The snapshot the whole search reads, held so a rebuild underneath it cannot take it away.
        FCk_GroundNav_FieldPtr _Field;

        // Stamped at plan time and carried into the result; staleness is derived, never flagged.
        FCk_GroundNav_Epoch _Epoch;

        FCk_GroundNav_QueryAgent _Agent;

        int32 _GoalFlatPlate = INDEX_NONE;

        FVector _GoalPoint = FVector::ZeroVector;
        FVector _SourcePoint = FVector::ZeroVector;

        // w >= 1. One is admissible; above one the answered length is within (w - 1) of optimal.
        float _GreedyWeightW = 1.0f;

        // Penalty per unit of rise over run. Zero until it is measured against real content.
        float _SlopePenaltyK = 0.0f;

        // Bias away from tight crossings, in cell widths of clearance. Zero for the same reason.
        float _ClearanceBiasK = 0.0f;

        // Flat plate id to the multiplier its ground is priced at. A plate the table does not name
        // is priced at one.
        TMap<int32, float> _PlateCostMultipliers;

        // What one cell of the field is worth, which is what turns a clearance into cell widths.
        float _CellSizeUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Everything one search discovers as it runs, held apart from the graph because the graph is
     * copied and this must not be.
     *
     * Node ids index the two arrays from ONE: zero is the source node, which arrives through no
     * crossing and stands on no interval, so it is in neither.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathNodePool
    {
    public:
        TArray<FCk_GroundNav_Crossing> _Crossings;
        TArray<FVector> _TransitionPoints;

        TMap<FCk_GroundNav_CrossingKey, FCk_GroundNav_PathNodeId> _NodeIds;

        FCk_GroundNav_QueryCost _Cost;

        // The lowest heuristic any EXPANDED node stood at, and that node. Kept here because CkAStar
        // tracks no such node and recovering it afterwards would mean re-deriving the heuristic over
        // the whole closed set.
        float _BestHeuristic = TNumericLimits<float>::Max();
        FCk_GroundNav_PathNodeId _BestNode = kPathSourceNode;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The point on a crossing a leg is priced through: the midpoint of the interval the funnel is
     * allowed to pass through.
     *
     * The agent radius does not appear: a symmetric inset moves the midpoint nowhere, and an
     * interval narrower than twice the radius collapses onto that same point, so the raw midpoint
     * is the answer in every case. Pure arithmetic over two vectors: deterministic, on the
     * interval by construction, never a funnel.
     */
    CKGROUNDNAV_API auto
    Get_CrossingTransitionPoint(
        const FCk_GroundNav_Crossing& InCrossing) -> FVector;

    // ----------------------------------------------------------------------------------------------------------------

    /** What a plate's ground is priced at. A plate the table does not name is priced at one. */
    CKGROUNDNAV_API auto
    Get_AreaMultiplier(
        const FCk_GroundNav_PathSharedData& InShared,
        int32                               InFlatPlate) -> float;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What one straight leg between two points costs.
     *
     * Lifted out of the graph because the search does not price every leg of the route it answers
     * with: the step onto the goal point crosses no door, and the segments of a funnelled polyline
     * are not edges at all. One arithmetic, so none of them can drift from the one that chose the
     * route. The two factors are arguments rather than lookups because their sources differ per
     * caller — a leg through a door carries that door's clearance factor and a leg through none
     * carries one.
     */
    CKGROUNDNAV_API auto
    Get_LegCost(
        const FCk_GroundNav_PathSharedData& InShared,
        const FVector&                      InFrom,
        const FVector&                      InTo,
        float                               InAreaMultiplier,
        float                               InClearanceFactor) -> float;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_PlatePortalGraph
    {
    public:
        // Default-constructible so a search state can hold one before it has a query. Answers
        // nothing until it is given a seed.
        FCk_GroundNav_PlatePortalGraph() = default;

        /** InSourceFlatPlate is the plate the source point stands on: the one plate no crossing names. */
        FCk_GroundNav_PlatePortalGraph(
            TSharedPtr<const FCk_GroundNav_PathSharedData> InShared,
            int32                                         InSourceFlatPlate);

    public:
        auto Neighbors(
            FCk_GroundNav_PathNodeId InNode) const -> TArray<FCk_GroundNav_PathNodeId>;

        auto Cost(
            FCk_GroundNav_PathNodeId InFrom,
            FCk_GroundNav_PathNodeId InTo) const -> float;

        /**
         * Distance from the node to the goal POINT, scaled by the greedy weight.
         *
         * InGoal is not read. This search's goal is a plate and a point, not a node anybody could
         * name in advance: which crossing enters the goal plate is unknown until the search finds
         * one, which is the question IsGoal answers instead.
         */
        auto Heuristic(
            FCk_GroundNav_PathNodeId InNode,
            FCk_GroundNav_PathNodeId InGoal) const -> float;

        /**
         * Whether the node arrives on the goal plate, asked exactly once per expansion.
         *
         * Which is why the closest-node memo is kept here and not in Heuristic: the node a partial
         * answer walks back from is one the search EXPANDED, not one it merely pushed and might
         * never have looked at again.
         */
        auto IsGoal(
            FCk_GroundNav_PathNodeId InNode) const -> bool;

    public:
        /**
         * The node a stored crossing resolves to on THIS graph, or INDEX_NONE when the crossing no
         * longer leaves the plate InFromNode arrives on.
         *
         * Node ids are pool ids minted per search, so a corridor that outlives its search is kept as
         * keys and walked back onto a live graph one door at a time. It cannot mint a node for a door
         * that is gone: the only candidates it compares against are the ones Neighbors returns, and
         * Neighbors mints from the field's own enumeration under the same admission rule the search
         * expanded by.
         */
        auto TryGet_NodeForKey(
            FCk_GroundNav_PathNodeId         InFromNode,
            const FCk_GroundNav_CrossingKey& InKey) const -> FCk_GroundNav_PathNodeId;

        /** The crossing a node arrives through. The source node arrives through none. */
        auto Get_Crossing(
            FCk_GroundNav_PathNodeId InNode) const -> const FCk_GroundNav_Crossing&;

        /** Where a node is entered: the source point for the source node, the transition point else. */
        auto Get_TransitionPoint(
            FCk_GroundNav_PathNodeId InNode) const -> FVector;

        /** What the search has read off the field so far, in the bake's own probe unit. */
        auto Get_Cost() const -> FCk_GroundNav_QueryCost;

        auto Get_BestNode() const -> FCk_GroundNav_PathNodeId;

        auto Get_BestHeuristic() const -> float;

        auto Get_SourceFlatPlate() const -> int32 { return _SourceFlatPlate; }

        auto Get_IsValid() const -> bool;

    private:
        auto DoGet_ArrivalPlate(
            FCk_GroundNav_PathNodeId InNode) const -> int32;

        auto DoGet_Point(
            FCk_GroundNav_PathNodeId InNode) const -> FVector;

        auto DoGet_HeuristicTo_Goal(
            FCk_GroundNav_PathNodeId InNode) const -> float;

        auto DoGet_ClearanceFactor(
            FCk_GroundNav_PathNodeId InNode) const -> double;

        auto DoGet_OrAdd_Node(
            const FCk_GroundNav_Crossing& InCrossing) const -> FCk_GroundNav_PathNodeId;

    private:
        TSharedPtr<const FCk_GroundNav_PathSharedData> _Shared;

        int32 _SourceFlatPlate = INDEX_NONE;

        // Written through by the const members above, because the concept requires them const and
        // the pool is the search's state rather than the graph's. Shared, never copied: see the
        // header note on copies.
        TSharedPtr<FCk_GroundNav_PathNodePool> _Pool;
    };

    // ----------------------------------------------------------------------------------------------------------------

    static_assert(
        astar::AStarGraph<FCk_GroundNav_PlatePortalGraph, FCk_GroundNav_PathNodeId>,
        "FCk_GroundNav_PlatePortalGraph must satisfy the AStarGraph concept");
}

// --------------------------------------------------------------------------------------------------------------------
