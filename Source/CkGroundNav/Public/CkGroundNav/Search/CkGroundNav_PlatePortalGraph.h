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

        // The lowest heuristic any node was measured at, and the node that was measured at it. Kept
        // here because CkAStar tracks no such node and recovering it afterwards would mean
        // re-deriving the heuristic over the whole closed set.
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
         *
         * Records the lowest estimate seen and the node that had it, so a search that ends without
         * reaching the goal can still answer with the corridor that came closest.
         */
        auto Heuristic(
            FCk_GroundNav_PathNodeId InNode,
            FCk_GroundNav_PathNodeId InGoal) const -> float;

        auto IsGoal(
            FCk_GroundNav_PathNodeId InNode) const -> bool;

    public:
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

        auto DoGet_AreaMultiplier(
            int32 InFlatPlate) const -> float;

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
