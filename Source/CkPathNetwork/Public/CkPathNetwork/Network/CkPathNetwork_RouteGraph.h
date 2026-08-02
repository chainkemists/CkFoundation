#pragma once

#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The per-query search space: the built network plus a virtual Start/Goal and overlay points
// (start/goal projected onto nearby edges). Endpoint-aware cost policy distinguishes local
// connectors from a long direct off-network shortcut while retaining a direct fallback.
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

    // Network- and construction-policy-dependent augmentation. Component
    // transfers, their edge-interior overlay points, and local shortcuts do not
    // depend on a route's start or goal, so runtime followers may safely share
    // this immutable seed across queries.
    struct CKPATHNETWORK_API FRouteGraphStaticData
    {
        TArray<FRouteOverlayPoint> _OverlayPoints;
        TMap<int32, TArray<int32>> _OverlayPointsByEdge;

        // Optional, symmetric off-network transfers between nodes in distinct
        // connected components. The route-node map is canonical and can join
        // static edge-interior overlay points as well as authored nodes.
        // The node-only map remains as a compatibility/diagnostic view.
        TMap<FRouteNodeId, TArray<FRouteNodeId>>
            _ComponentTransfersByRouteNode;
        TMap<int32, TArray<int32>> _ComponentTransfersByNode;
        int32 _ComponentTransferCandidateCount = 0;
        int32 _ComponentTransferEdgeInteriorCandidateCount = 0;
        int32 _ComponentTransferRejectedByCellCapCount = 0;

        // Optional, symmetric off-network shortcuts between nearby nodes in the
        // same connected component.
        TMap<int32, TArray<int32>> _LocalNetworkShortcutsByNode;
        int32 _LocalNetworkShortcutCandidateCount = 0;
        int32 _LocalNetworkShortcutCandidateSourceCount = 0;
        bool _LocalNetworkShortcutBudgetExceeded = false;
    };

    // Per-query data starts as a copy of FRouteGraphStaticData, then adds
    // start/goal overlay points and navmesh-derived repricing. It must never be
    // retained as the network-static cache value.
    struct CKPATHNETWORK_API FRouteGraphSharedData
        : FRouteGraphStaticData
    {
        // The default preserves the runtime direct-fallback route. Editor diagnostics can
        // explicitly omit this synthetic edge when asking whether the network is connected.
        bool _AllowDirectStartToGoal = true;

        // Off-path hops repriced by the navmesh validation pass. Key = PackOffPathKey(from, to).
        TMap<uint64, float> _RepricedOffPathCosts;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Resolved per-query policy. Public follower tuning uses zero-valued compatibility
    // sentinels; the processor resolves those before constructing a graph.
    struct CKPATHNETWORK_API FRouteCostPolicy
    {
        float _FarOrDirectCostMultiplier = 3.0f;
        float _NearEndpointCostMultiplier = 3.0f;
        float _NetworkGapCostMultiplier = 3.0f;
        float _EndpointJoinMaxDistance = 0.0f;
        float _ComponentTransferMaxDistance = 0.0f;
        float _LocalNetworkShortcutMaxDistance = 0.0f;
        float _DirectTripGraceDistance = 0.0f;
        float _DirectRouteMinimumSavingsFraction = 0.0f;

        auto
        Get_IsEndpointJoinPermitted(float InDistance) const -> bool
        {
            return _EndpointJoinMaxDistance <= 0.0f || InDistance <= _EndpointJoinMaxDistance;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Exact key for the subset of route policy that constructs static graph
    // augmentation. Zero is canonicalized so +0 and -0 obey TMap's equal-key
    // hash contract.
    struct CKPATHNETWORK_API FRouteGraphStaticDataKey
    {
        float _NetworkGapCostMultiplier = 0.0f;
        float _ComponentTransferMaxDistance = 0.0f;
        float _LocalNetworkShortcutMaxDistance = 0.0f;

        static auto
        TryFromPolicy(
            const FRouteCostPolicy& InPolicy)
            -> TOptional<FRouteGraphStaticDataKey>
        {
            const bool InputsAreValid =
                FMath::IsFinite(
                    InPolicy._NetworkGapCostMultiplier)
                && InPolicy._NetworkGapCostMultiplier
                    >= 1.0f
                && FMath::IsFinite(
                    InPolicy._ComponentTransferMaxDistance)
                && InPolicy._ComponentTransferMaxDistance
                    >= 0.0f
                && FMath::IsFinite(
                    InPolicy._LocalNetworkShortcutMaxDistance)
                && InPolicy._LocalNetworkShortcutMaxDistance
                    >= 0.0f;
            if (NOT InputsAreValid)
            { return {}; }

            const auto CanonicalizeZero =
                [](const float InValue)
                {
                    return InValue == 0.0f
                        ? 0.0f
                        : InValue;
                };
            return TOptional<FRouteGraphStaticDataKey>{
                FRouteGraphStaticDataKey{
                    CanonicalizeZero(
                        InPolicy._NetworkGapCostMultiplier),
                    CanonicalizeZero(
                        InPolicy._ComponentTransferMaxDistance),
                    CanonicalizeZero(
                        InPolicy._LocalNetworkShortcutMaxDistance)}};
        }

        auto
        operator==(
            const FRouteGraphStaticDataKey&) const
            -> bool = default;

        friend auto
        GetTypeHash(
            const FRouteGraphStaticDataKey& InKey)
            -> uint32
        {
            return HashCombineFast(
                HashCombineFast(
                    ::GetTypeHash(
                        InKey._NetworkGapCostMultiplier),
                    ::GetTypeHash(
                        InKey._ComponentTransferMaxDistance)),
                ::GetTypeHash(
                    InKey._LocalNetworkShortcutMaxDistance));
        }
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
            const FRouteCostPolicy& InCostPolicy,
            TSharedPtr<FRouteGraphSharedData> InShared)
            : _Network{InNetwork}
            , _StartLocation{InStartLocation}
            , _GoalLocation{InGoalLocation}
            , _CostPolicy{InCostPolicy}
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
        Get_IsComponentTransferHop(
            const FRouteNodeId& InFrom,
            const FRouteNodeId& InTo) const -> bool;

        auto
        Get_IsLocalNetworkShortcutHop(
            const FRouteNodeId& InFrom,
            const FRouteNodeId& InTo) const -> bool;

        auto
        Get_OffPathCostForResolvedLength(
            const FRouteNodeId& InFrom,
            const FRouteNodeId& InTo,
            float InResolvedLength) const -> float;

        auto
        Get_Shared() const -> const TSharedPtr<FRouteGraphSharedData>& { return _Shared; }

        static auto
        PackOffPathKey(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) -> uint64;

    private:
        auto
        DoGet_AlongEdgeCostToNode(const FRouteOverlayPoint& InPoint, int32 InNodeId) const -> float;

        auto
        DoGet_OffPathCost(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const -> float;

        auto
        DoGet_OffPathCostMultiplier(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const -> float;

    private:
        const FBuiltNetwork* _Network = nullptr;
        FVector _StartLocation = FVector::ZeroVector;
        FVector _GoalLocation = FVector::ZeroVector;
        FRouteCostPolicy _CostPolicy;

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
