#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Built (derived) network data owned by the network entity's FFragment_PathNetwork_Graph; nothing
// outside this module mutates it. Plain structs on purpose — the reflected surface is the authored
// ribbon layer, this one is derived from it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    struct CKPATHNETWORK_API FBuiltNode
    {
        FVector _Location = FVector::ZeroVector;
        TArray<int32> _EdgeIds;
    };

    // _Points runs NodeA -> NodeB; _HalfWidths and _CumulativeLengths are per-point parallel
    // arrays (_CumulativeLengths[0] == 0, _CumulativeLengths.Last() == _Length).
    struct CKPATHNETWORK_API FBuiltEdge
    {
        int32 _NodeA = INDEX_NONE;
        int32 _NodeB = INDEX_NONE;
        TArray<FVector> _Points;
        TArray<float> _HalfWidths;
        TArray<float> _CumulativeLengths;
        float _Length = 0.0f;
        FGuid _SourceRibbonId;
    };

    struct CKPATHNETWORK_API FEdgeProjection
    {
        int32 _EdgeId = INDEX_NONE;
        float _DistAlong = 0.0f;
        FVector _Location = FVector::ZeroVector;
        float _Distance = 0.0f;
    };

    struct CKPATHNETWORK_API FEdgeSample
    {
        FVector _Location = FVector::ZeroVector;
        FVector _Tangent = FVector::ForwardVector;
        float _HalfWidth = 100.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // World-independent connectivity diagnostics over a built network. Components count only
    // nodes incident to at least one valid edge; malformed or isolated nodes remain explicitly
    // visible without being treated as routable islands.
    struct CKPATHNETWORK_API FNetworkTopologyAnalysis
    {
        int32 _NodeCount = 0;
        int32 _EdgeCount = 0;
        int32 _RoutableNodeCount = 0;
        int32 _ComponentCount = 0;
        int32 _LargestComponentNodeCount = 0;
        int32 _LargestComponentEdgeCount = 0;
        int32 _DeadEndNodeCount = 0;
        int32 _IsolatedNodeCount = 0;

        TArray<int32> _ComponentByNode;
        TArray<int32> _LogicalDegreeByNode;
    };

    CKPATHNETWORK_API auto
    Analyze_NetworkTopology(
        const struct FBuiltNetwork& InNetwork) -> FNetworkTopologyAnalysis;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKPATHNETWORK_API FBuiltNetwork
    {
        TArray<FBuiltNode> _Nodes;
        TArray<FBuiltEdge> _Edges;

        // Uniform XY chunk grid: _ChunkEdgeIds[cell] = ids of edges whose inflated polyline AABB
        // touches it; _ChunkVersions[cell] bumps on rebuild, which is what makes corridors stale.
        FVector _GridOrigin = FVector::ZeroVector;
        float _ChunkSize = 3200.0f;
        int32 _ChunkCountX = 0;
        int32 _ChunkCountY = 0;
        TArray<TArray<int32>> _ChunkEdgeIds;
        TArray<uint32> _ChunkVersions;

    public:
        auto Get_ChunkIndex(const FVector& InLocation) const -> int32;

        // Deduped but distance-UNfiltered — callers project to get true distances.
        auto Query_EdgesNear(const FVector& InLocation, float InRadius) const -> TArray<int32>;

        auto Project_OntoEdge(int32 InEdgeId, const FVector& InLocation) const -> FEdgeProjection;

        // InDistAlong is clamped to [0, Length].
        auto Sample_Edge(int32 InEdgeId, float InDistAlong) const -> FEdgeSample;

        auto Get_ChunksTouchedByEdge(int32 InEdgeId) const -> TArray<int32>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
