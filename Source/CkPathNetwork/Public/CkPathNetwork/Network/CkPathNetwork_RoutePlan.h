#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared, world-free route planning. Runtime followers and editor previews both use this seam for
// endpoint candidate gathering, cost policy, and A*. Navmesh validation/repricing remains a
// runtime post-pass because an editor geometry preview may intentionally have no navigation data.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    enum class ERouteSearchOutcome : uint8
    {
        InvalidInput,
        Complete,
        Failed,
        InProgress,
        CostThresholdReached
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKPATHNETWORK_API FRoutePlanResult
    {
        bool _Succeeded = false;
        ERouteSearchOutcome _SearchOutcome = ERouteSearchOutcome::InvalidInput;
        TSharedPtr<FRouteGraphSharedData> _Shared;
        TArray<FRouteNodeId> _Path;
        TArray<FRouteLegSpan> _Spans;
        float _EstimatedCost = 0.0f;
    };

    CKPATHNETWORK_API auto
    Resolve_RouteCostPolicy(
        const FCk_PathNetworkFollower_Tuning& InTuning) -> FRouteCostPolicy;

    CKPATHNETWORK_API auto
    Resolve_RouteCostPolicy(
        const FCk_Fragment_PathNetworkFollower_ParamsData& InParams) -> FRouteCostPolicy;

    CKPATHNETWORK_API auto
    Gather_RouteEndpointCandidates(
        const FBuiltNetwork& InNetwork,
        const FVector& InLocation,
        const FRouteCostPolicy& InCostPolicy) -> TArray<FRouteOverlayPoint>;

    // Network use includes authored ribbon traversal and explicit transfer/shortcut
    // hops between network nodes. Endpoint joins and raw direct fallback do not count.
    CKPATHNETWORK_API auto
    Uses_Network(
        const FRoutePlanResult& InPlan) -> bool;

    // ----------------------------------------------------------------------------------------------------------------

    CKPATHNETWORK_API auto
    Build_RouteGraphStaticData(
        const FBuiltNetwork& InNetwork,
        const FRouteCostPolicy& InCostPolicy)
        -> TSharedPtr<const FRouteGraphStaticData>;

    CKPATHNETWORK_API auto
    Build_RouteGraphSharedData(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy,
        const TSharedPtr<const FRouteGraphStaticData>&
            InStaticData = {})
        -> TSharedPtr<FRouteGraphSharedData>;

    CKPATHNETWORK_API auto
    Search_RouteGraph(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy,
        const TSharedPtr<FRouteGraphSharedData>& InShared,
        int32 InMaxIterations = 200000) -> FRoutePlanResult;

    CKPATHNETWORK_API auto
    Plan_RouteGraph(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy) -> FRoutePlanResult;
}

// --------------------------------------------------------------------------------------------------------------------
