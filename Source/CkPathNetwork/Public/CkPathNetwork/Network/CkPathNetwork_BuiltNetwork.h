#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Built (derived) network data. Produced from ribbons by Build_NetworkFromRibbons — pure math,
// runtime-callable. Owned at runtime by the network entity's FFragment_PathNetwork_Graph; nothing
// outside this module mutates it. Plain structs (not USTRUCTs) on purpose: the reflected surface
// is the authored ribbon layer; this layer is derived and rebuildable.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    struct CKPATHNETWORK_API FBuiltNode
    {
        FVector _Location = FVector::ZeroVector;
        TArray<int32> _EdgeIds;
    };

    // A ribbon span between two nodes. _Points runs NodeA -> NodeB; _HalfWidths and
    // _CumulativeLengths are per-point parallel arrays (_CumulativeLengths[0] == 0,
    // _CumulativeLengths.Last() == _Length).
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

    struct CKPATHNETWORK_API FBuiltNetwork
    {
        TArray<FBuiltNode> _Nodes;
        TArray<FBuiltEdge> _Edges;

        // Uniform XY chunk grid over the network bounds: _ChunkEdgeIds[cell] = ids of edges whose
        // inflated polyline AABB touches the cell. _ChunkVersions[cell] bumps on rebuild — corridors
        // record the versions of the chunks they cross and replan when any of them moved on.
        FVector _GridOrigin = FVector::ZeroVector;
        float _ChunkSize = 3200.0f;
        int32 _ChunkCountX = 0;
        int32 _ChunkCountY = 0;
        TArray<TArray<int32>> _ChunkEdgeIds;
        TArray<uint32> _ChunkVersions;

    public:
        auto Get_ChunkIndex(const FVector& InLocation) const -> int32;

        // All edge ids whose chunk cells intersect the XY circle (InLocation, InRadius). Deduped;
        // distance-UNfiltered — callers project to get true distances.
        auto Query_EdgesNear(const FVector& InLocation, float InRadius) const -> TArray<int32>;

        // Closest point on the edge's polyline to InLocation (3D distance).
        auto Project_OntoEdge(int32 InEdgeId, const FVector& InLocation) const -> FEdgeProjection;

        // Interpolated point/tangent/half-width at a distance along the edge (clamped to [0, Length]).
        auto Sample_Edge(int32 InEdgeId, float InDistAlong) const -> FEdgeSample;

        auto Get_ChunksTouchedByEdge(int32 InEdgeId) const -> TArray<int32>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
