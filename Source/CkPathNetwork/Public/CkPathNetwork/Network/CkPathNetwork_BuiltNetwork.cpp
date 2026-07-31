#include "CkPathNetwork_BuiltNetwork.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    Analyze_NetworkTopology(
        const FBuiltNetwork& InNetwork)
        -> FNetworkTopologyAnalysis
    {
        auto Result = FNetworkTopologyAnalysis{};
        Result._NodeCount = InNetwork._Nodes.Num();
        Result._EdgeCount = InNetwork._Edges.Num();
        Result._ComponentByNode.Init(INDEX_NONE, Result._NodeCount);
        Result._LogicalDegreeByNode.Init(0, Result._NodeCount);

        auto AdjacentNodes = TArray<TArray<int32>>{};
        auto IncidentEdges = TArray<TArray<int32>>{};
        AdjacentNodes.SetNum(Result._NodeCount);
        IncidentEdges.SetNum(Result._NodeCount);

        for (auto EdgeId = 0; EdgeId < InNetwork._Edges.Num(); ++EdgeId)
        {
            const auto& Edge = InNetwork._Edges[EdgeId];
            if (NOT InNetwork._Nodes.IsValidIndex(Edge._NodeA)
                || NOT InNetwork._Nodes.IsValidIndex(Edge._NodeB))
            { continue; }

            // A self-loop contributes two to its node's logical degree.
            ++Result._LogicalDegreeByNode[Edge._NodeA];
            ++Result._LogicalDegreeByNode[Edge._NodeB];
            IncidentEdges[Edge._NodeA].Add(EdgeId);
            AdjacentNodes[Edge._NodeA].Add(Edge._NodeB);
            if (Edge._NodeB != Edge._NodeA)
            {
                IncidentEdges[Edge._NodeB].Add(EdgeId);
                AdjacentNodes[Edge._NodeB].Add(Edge._NodeA);
            }
        }

        for (auto NodeId = 0; NodeId < Result._NodeCount; ++NodeId)
        {
            const auto Degree = Result._LogicalDegreeByNode[NodeId];
            if (Degree <= 0)
            {
                ++Result._IsolatedNodeCount;
                continue;
            }

            ++Result._RoutableNodeCount;
            if (Degree == 1)
            { ++Result._DeadEndNodeCount; }
        }

        for (auto SeedNodeId = 0; SeedNodeId < Result._NodeCount; ++SeedNodeId)
        {
            if (Result._LogicalDegreeByNode[SeedNodeId] <= 0
                || Result._ComponentByNode[SeedNodeId] != INDEX_NONE)
            { continue; }

            const auto ComponentId = Result._ComponentCount++;
            auto PendingNodes = TArray<int32>{SeedNodeId};
            auto PendingIndex = 0;
            auto ComponentNodeCount = 0;
            auto ComponentEdgeIds = TSet<int32>{};
            Result._ComponentByNode[SeedNodeId] = ComponentId;

            while (PendingIndex < PendingNodes.Num())
            {
                const auto NodeId = PendingNodes[PendingIndex++];
                ++ComponentNodeCount;

                for (const auto EdgeId : IncidentEdges[NodeId])
                { ComponentEdgeIds.Add(EdgeId); }

                for (const auto OtherNodeId : AdjacentNodes[NodeId])
                {
                    if (Result._ComponentByNode[OtherNodeId] != INDEX_NONE)
                    { continue; }

                    Result._ComponentByNode[OtherNodeId] = ComponentId;
                    PendingNodes.Add(OtherNodeId);
                }
            }

            if (ComponentNodeCount > Result._LargestComponentNodeCount)
            {
                Result._LargestComponentNodeCount = ComponentNodeCount;
                Result._LargestComponentEdgeCount = ComponentEdgeIds.Num();
            }
        }

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FBuiltNetwork::
        Get_ChunkIndex(const FVector& InLocation) const
        -> int32
    {
        if (_ChunkCountX <= 0 || _ChunkCountY <= 0)
        { return INDEX_NONE; }

        const auto CellX = FMath::FloorToInt32((InLocation.X - _GridOrigin.X) / _ChunkSize);
        const auto CellY = FMath::FloorToInt32((InLocation.Y - _GridOrigin.Y) / _ChunkSize);

        if (CellX < 0 || CellY < 0 || CellX >= _ChunkCountX || CellY >= _ChunkCountY)
        { return INDEX_NONE; }

        return CellY * _ChunkCountX + CellX;
    }

    auto
        FBuiltNetwork::
        Query_EdgesNear(const FVector& InLocation, float InRadius) const
        -> TArray<int32>
    {
        auto Result = TArray<int32>{};

        if (_ChunkCountX <= 0 || _ChunkCountY <= 0)
        { return Result; }

        const auto MinX = FMath::Clamp(FMath::FloorToInt32((InLocation.X - InRadius - _GridOrigin.X) / _ChunkSize), 0, _ChunkCountX - 1);
        const auto MaxX = FMath::Clamp(FMath::FloorToInt32((InLocation.X + InRadius - _GridOrigin.X) / _ChunkSize), 0, _ChunkCountX - 1);
        const auto MinY = FMath::Clamp(FMath::FloorToInt32((InLocation.Y - InRadius - _GridOrigin.Y) / _ChunkSize), 0, _ChunkCountY - 1);
        const auto MaxY = FMath::Clamp(FMath::FloorToInt32((InLocation.Y + InRadius - _GridOrigin.Y) / _ChunkSize), 0, _ChunkCountY - 1);

        auto Seen = TSet<int32>{};
        for (auto CellY = MinY; CellY <= MaxY; ++CellY)
        {
            for (auto CellX = MinX; CellX <= MaxX; ++CellX)
            {
                for (const auto EdgeId : _ChunkEdgeIds[CellY * _ChunkCountX + CellX])
                {
                    auto AlreadySeen = false;
                    Seen.Add(EdgeId, &AlreadySeen);
                    if (NOT AlreadySeen)
                    { Result.Add(EdgeId); }
                }
            }
        }

        return Result;
    }

    auto
        FBuiltNetwork::
        Project_OntoEdge(int32 InEdgeId, const FVector& InLocation) const
        -> FEdgeProjection
    {
        auto Result = FEdgeProjection{};

        CK_ENSURE_IF_NOT(_Edges.IsValidIndex(InEdgeId), TEXT("Project_OntoEdge: invalid edge id [{}]"), InEdgeId)
        { return Result; }

        const auto& Edge = _Edges[InEdgeId];

        CK_ENSURE_IF_NOT(Edge._Points.Num() >= 2, TEXT("Project_OntoEdge: edge [{}] is degenerate ({} points)"), InEdgeId, Edge._Points.Num())
        { return Result; }

        Result._EdgeId = InEdgeId;

        auto BestDistSq = TNumericLimits<double>::Max();

        for (auto Index = 0; Index < Edge._Points.Num() - 1; ++Index)
        {
            const auto& A = Edge._Points[Index];
            const auto& B = Edge._Points[Index + 1];

            const auto Closest = FMath::ClosestPointOnSegment(InLocation, A, B);
            const auto DistSq = FVector::DistSquared(InLocation, Closest);

            if (DistSq < BestDistSq)
            {
                BestDistSq = DistSq;

                const auto SegmentLength = FVector::Dist(A, B);
                const auto AlongSegment = SegmentLength > UE_KINDA_SMALL_NUMBER
                    ? FVector::Dist(A, Closest)
                    : 0.0;

                Result._Location = Closest;
                Result._DistAlong = Edge._CumulativeLengths[Index] + static_cast<float>(AlongSegment);
            }
        }

        Result._Distance = static_cast<float>(FMath::Sqrt(BestDistSq));
        return Result;
    }

    auto
        FBuiltNetwork::
        Sample_Edge(int32 InEdgeId, float InDistAlong) const
        -> FEdgeSample
    {
        auto Result = FEdgeSample{};

        CK_ENSURE_IF_NOT(_Edges.IsValidIndex(InEdgeId), TEXT("Sample_Edge: invalid edge id [{}]"), InEdgeId)
        { return Result; }

        const auto& Edge = _Edges[InEdgeId];

        CK_ENSURE_IF_NOT(Edge._Points.Num() >= 2, TEXT("Sample_Edge: edge [{}] is degenerate ({} points)"), InEdgeId, Edge._Points.Num())
        { return Result; }

        const auto Dist = FMath::Clamp(InDistAlong, 0.0f, Edge._Length);

        // Find the segment containing Dist. _CumulativeLengths is sorted ascending.
        auto SegmentIndex = 0;
        for (auto Index = 1; Index < Edge._CumulativeLengths.Num(); ++Index)
        {
            if (Edge._CumulativeLengths[Index] >= Dist)
            {
                SegmentIndex = Index - 1;
                break;
            }

            SegmentIndex = Index - 1;
        }

        const auto& A = Edge._Points[SegmentIndex];
        const auto& B = Edge._Points[SegmentIndex + 1];
        const auto SegmentStart = Edge._CumulativeLengths[SegmentIndex];
        const auto SegmentEnd = Edge._CumulativeLengths[SegmentIndex + 1];
        const auto SegmentSpan = SegmentEnd - SegmentStart;

        const auto Alpha = SegmentSpan > UE_KINDA_SMALL_NUMBER
            ? (Dist - SegmentStart) / SegmentSpan
            : 0.0f;

        Result._Location = FMath::Lerp(A, B, Alpha);
        Result._HalfWidth = FMath::Lerp(Edge._HalfWidths[SegmentIndex], Edge._HalfWidths[SegmentIndex + 1], Alpha);

        const auto Tangent = (B - A).GetSafeNormal();
        Result._Tangent = Tangent.IsNearlyZero() ? FVector::ForwardVector : Tangent;

        return Result;
    }

    auto
        FBuiltNetwork::
        Get_ChunksTouchedByEdge(int32 InEdgeId) const
        -> TArray<int32>
    {
        auto Result = TArray<int32>{};

        for (auto CellIndex = 0; CellIndex < _ChunkEdgeIds.Num(); ++CellIndex)
        {
            if (_ChunkEdgeIds[CellIndex].Contains(InEdgeId))
            { Result.Add(CellIndex); }
        }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
