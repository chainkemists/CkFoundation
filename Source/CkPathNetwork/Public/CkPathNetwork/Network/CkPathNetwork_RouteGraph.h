#pragma once

#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The per-query search space: the built network plus a virtual Start/Goal and overlay points
// (start/goal projected onto nearby edges). Off-path hops cost euclidean x OffPathCostMultiplier —
// that one number IS the "prefer the sidewalk unless the shortcut is worth it" heuristic.
// LIFETIME: raw pointer to the built network, valid only for one synchronous search inside a
// processor tick. Never store it across frames.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    enum class ERouteNodeKind : uint8
    {
        Start,
        Goal,
        NetNode,
        OverlayPoint
    };

    struct CKPATHNETWORK_API FRouteNodeId
    {
        ERouteNodeKind _Kind = ERouteNodeKind::Start;
        int32 _Index = 0;

        auto operator==(const FRouteNodeId&) const -> bool = default;
    };

    CKPATHNETWORK_API auto
    GetTypeHash(const FRouteNodeId& InId) -> uint32;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKPATHNETWORK_API FRouteOverlayPoint
    {
        int32 _EdgeId = INDEX_NONE;
        float _DistAlong = 0.0f;
        FVector _Location = FVector::ZeroVector;
    };

    struct CKPATHNETWORK_API FRouteGraphSharedData
    {
        TArray<FRouteOverlayPoint> _OverlayPoints;
        TMap<int32, TArray<int32>> _OverlayPointsByEdge;

        // Off-path hops repriced by the navmesh validation pass. Key = PackOffPathKey(from, to).
        TMap<uint64, float> _RepricedOffPathCosts;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKPATHNETWORK_API FRouteGraph
    {
    public:
        FRouteGraph() = default;

        FRouteGraph(
            const FBuiltNetwork* InNetwork,
            const FVector& InStartLocation,
            const FVector& InGoalLocation,
            float InOffPathCostMultiplier,
            TSharedPtr<FRouteGraphSharedData> InShared)
            : _Network{InNetwork}
            , _StartLocation{InStartLocation}
            , _GoalLocation{InGoalLocation}
            , _OffPathCostMultiplier{InOffPathCostMultiplier}
            , _Shared{MoveTemp(InShared)}
        {
        }

        auto
        Neighbors(const FRouteNodeId& InNode) const -> TArray<FRouteNodeId>;

        auto
        Cost(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const -> float;

        auto
        Heuristic(const FRouteNodeId& InCurrent, const FRouteNodeId& InGoal) const -> float;

        auto
        IsGoal(const FRouteNodeId& InNode) const -> bool;

        auto
        Get_NodeLocation(const FRouteNodeId& InNode) const -> FVector;

        auto
        Get_IsOffPathHop(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const -> bool;

        auto
        Get_Shared() const -> const TSharedPtr<FRouteGraphSharedData>& { return _Shared; }

        static auto
        PackOffPathKey(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) -> uint64;

    private:
        auto
        DoGet_AlongEdgeCostToNode(const FRouteOverlayPoint& InPoint, int32 InNodeId) const -> float;

        auto
        DoGet_OffPathCost(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const -> float;

    private:
        const FBuiltNetwork* _Network = nullptr;
        FVector _StartLocation = FVector::ZeroVector;
        FVector _GoalLocation = FVector::ZeroVector;
        float _OffPathCostMultiplier = 3.0f;

        TSharedPtr<FRouteGraphSharedData> _Shared;
    };

    // ----------------------------------------------------------------------------------------------------------------

    static_assert(
        astar::AStarGraph<FRouteGraph, FRouteNodeId>,
        "FRouteGraph must satisfy the AStarGraph concept");

    // ----------------------------------------------------------------------------------------------------------------

    // One step of a solved route. On-network spans carry the edge and its [from, to] distance-along
    // span (from > to = traveling against edge direction); off-path spans carry only endpoints.
    struct CKPATHNETWORK_API FRouteLegSpan
    {
        bool _IsOffPath = false;
        FVector _FromLocation = FVector::ZeroVector;
        FVector _ToLocation = FVector::ZeroVector;
        int32 _EdgeId = INDEX_NONE;
        float _FromDist = 0.0f;
        float _ToDist = 0.0f;
        FRouteNodeId _FromId;
        FRouteNodeId _ToId;
    };

    CKPATHNETWORK_API auto
    ExtractLegSpans(
        const FRouteGraph& InGraph,
        const FBuiltNetwork& InNetwork,
        const TArray<FRouteNodeId>& InPath) -> TArray<FRouteLegSpan>;
}

// --------------------------------------------------------------------------------------------------------------------
