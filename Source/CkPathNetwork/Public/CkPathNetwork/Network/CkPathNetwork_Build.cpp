#include "CkPathNetwork_Build.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_build
{
    // Working copy of one ribbon during the build. Points/HalfWidths are parallel;
    // CumulativeLengths is derived and kept in sync after every geometry change.
    struct FWorkingPolyline
    {
        TArray<FVector> _Points;
        TArray<float> _HalfWidths;
        TArray<float> _CumulativeLengths;
        FGuid _SourceRibbonId;

        auto Get_Length() const -> float
        { return _CumulativeLengths.Num() > 0 ? _CumulativeLengths.Last() : 0.0f; }
    };

    auto
    Recompute_CumulativeLengths(FWorkingPolyline& InOutPolyline) -> void
    {
        InOutPolyline._CumulativeLengths.Reset(InOutPolyline._Points.Num());
        auto Accum = 0.0f;
        InOutPolyline._CumulativeLengths.Add(0.0f);

        for (auto Index = 1; Index < InOutPolyline._Points.Num(); ++Index)
        {
            Accum += static_cast<float>(FVector::Dist(InOutPolyline._Points[Index - 1], InOutPolyline._Points[Index]));
            InOutPolyline._CumulativeLengths.Add(Accum);
        }
    }

    struct FPolylineProjection
    {
        float _DistAlong = 0.0f;
        FVector _Location = FVector::ZeroVector;
        float _Distance = TNumericLimits<float>::Max();
    };

    auto
    Project_OntoPolyline(const FWorkingPolyline& InPolyline, const FVector& InLocation) -> FPolylineProjection
    {
        auto Result = FPolylineProjection{};
        auto BestDistSq = TNumericLimits<double>::Max();

        for (auto Index = 0; Index < InPolyline._Points.Num() - 1; ++Index)
        {
            const auto& A = InPolyline._Points[Index];
            const auto& B = InPolyline._Points[Index + 1];

            const auto Closest = FMath::ClosestPointOnSegment(InLocation, A, B);
            const auto DistSq = FVector::DistSquared(InLocation, Closest);

            if (DistSq < BestDistSq)
            {
                BestDistSq = DistSq;

                const auto AlongSegment = FVector::Dist(A, Closest);
                Result._Location = Closest;
                Result._DistAlong = InPolyline._CumulativeLengths[Index] + static_cast<float>(AlongSegment);
            }
        }

        Result._Distance = static_cast<float>(FMath::Sqrt(BestDistSq));
        return Result;
    }

    // Interpolated point/half-width at a distance along the polyline.
    auto
    Sample_Polyline(const FWorkingPolyline& InPolyline, float InDistAlong, FVector& OutLocation, float& OutHalfWidth) -> void
    {
        const auto Dist = FMath::Clamp(InDistAlong, 0.0f, InPolyline.Get_Length());

        auto SegmentIndex = 0;
        for (auto Index = 1; Index < InPolyline._CumulativeLengths.Num(); ++Index)
        {
            SegmentIndex = Index - 1;
            if (InPolyline._CumulativeLengths[Index] >= Dist)
            { break; }
        }

        const auto SegmentStart = InPolyline._CumulativeLengths[SegmentIndex];
        const auto SegmentEnd = InPolyline._CumulativeLengths[SegmentIndex + 1];
        const auto SegmentSpan = SegmentEnd - SegmentStart;

        const auto Alpha = SegmentSpan > UE_KINDA_SMALL_NUMBER
            ? (Dist - SegmentStart) / SegmentSpan
            : 0.0f;

        OutLocation = FMath::Lerp(InPolyline._Points[SegmentIndex], InPolyline._Points[SegmentIndex + 1], Alpha);
        OutHalfWidth = FMath::Lerp(InPolyline._HalfWidths[SegmentIndex], InPolyline._HalfWidths[SegmentIndex + 1], Alpha);
    }

    // Split one polyline at the given sorted, deduped distances. Cut points are interpolated;
    // each cut distance produces two coincident endpoints (end of one piece, start of the next).
    auto
    Split_Polyline(const FWorkingPolyline& InPolyline, const TArray<float>& InSortedCutDistances) -> TArray<FWorkingPolyline>
    {
        auto Pieces = TArray<FWorkingPolyline>{};

        auto CutQueue = InSortedCutDistances;
        auto Current = FWorkingPolyline{};
        Current._SourceRibbonId = InPolyline._SourceRibbonId;
        Current._Points.Add(InPolyline._Points[0]);
        Current._HalfWidths.Add(InPolyline._HalfWidths[0]);

        for (auto Index = 0; Index < InPolyline._Points.Num() - 1; ++Index)
        {
            const auto SegmentStart = InPolyline._CumulativeLengths[Index];
            const auto SegmentEnd = InPolyline._CumulativeLengths[Index + 1];

            while (CutQueue.Num() > 0 && CutQueue[0] <= SegmentEnd)
            {
                const auto CutDist = CutQueue[0];
                CutQueue.RemoveAt(0);

                auto CutLocation = FVector{};
                auto CutHalfWidth = 0.0f;
                Sample_Polyline(InPolyline, CutDist, CutLocation, CutHalfWidth);

                Current._Points.Add(CutLocation);
                Current._HalfWidths.Add(CutHalfWidth);
                Recompute_CumulativeLengths(Current);
                Pieces.Add(Current);

                Current = FWorkingPolyline{};
                Current._SourceRibbonId = InPolyline._SourceRibbonId;
                Current._Points.Add(CutLocation);
                Current._HalfWidths.Add(CutHalfWidth);
            }

            Current._Points.Add(InPolyline._Points[Index + 1]);
            Current._HalfWidths.Add(InPolyline._HalfWidths[Index + 1]);
        }

        Recompute_CumulativeLengths(Current);
        Pieces.Add(Current);

        return Pieces;
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct FUnionFind
    {
        TArray<int32> _Parent;

        explicit FUnionFind(int32 InCount)
        {
            _Parent.SetNumUninitialized(InCount);
            for (auto Index = 0; Index < InCount; ++Index)
            { _Parent[Index] = Index; }
        }

        auto Find(int32 InIndex) -> int32
        {
            while (_Parent[InIndex] != InIndex)
            {
                _Parent[InIndex] = _Parent[_Parent[InIndex]];
                InIndex = _Parent[InIndex];
            }
            return InIndex;
        }

        auto Union(int32 InA, int32 InB) -> void
        {
            const auto RootA = Find(InA);
            const auto RootB = Find(InB);
            if (RootA != RootB)
            { _Parent[RootB] = RootA; }
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    Build_NetworkFromRibbons(
        const TArray<FCk_PathNetwork_Ribbon>& InRibbons,
        const FCk_PathNetwork_BuildParams& InParams)
        -> FBuiltNetwork
    {
        using namespace ck_pathnetwork_build;

        auto Network = FBuiltNetwork{};
        Network._ChunkSize = InParams.Get_ChunkSize();

        const auto SnapRadius = InParams.Get_NodeSnapRadius();

        // ------------------------------------------------------------------------------------------------------------
        // Working copies. Ribbons with fewer than 2 points or near-zero length are authoring noise.
        // ------------------------------------------------------------------------------------------------------------

        auto Polylines = TArray<FWorkingPolyline>{};

        for (const auto& Ribbon : InRibbons)
        {
            if (Ribbon.Get_Points().Num() < 2)
            { continue; }

            auto Polyline = FWorkingPolyline{};
            Polyline._SourceRibbonId = Ribbon.Get_RibbonId();

            for (const auto& Point : Ribbon.Get_Points())
            {
                Polyline._Points.Add(Point.Get_Location());
                Polyline._HalfWidths.Add(Point.Get_HalfWidth());
            }

            Recompute_CumulativeLengths(Polyline);

            if (Polyline.Get_Length() < UE_KINDA_SMALL_NUMBER)
            { continue; }

            Polylines.Add(MoveTemp(Polyline));
        }

        if (Polylines.IsEmpty())
        { return Network; }

        // ------------------------------------------------------------------------------------------------------------
        // T-junction split pass. Every polyline endpoint is tested against every polyline's
        // interior (including its own, for P-shaped self-touches); hits record a cut distance.
        // All cuts are collected first, then applied once per polyline.
        // ------------------------------------------------------------------------------------------------------------

        auto CutsPerPolyline = TArray<TArray<float>>{};
        CutsPerPolyline.SetNum(Polylines.Num());

        for (auto SourceIndex = 0; SourceIndex < Polylines.Num(); ++SourceIndex)
        {
            const auto& Source = Polylines[SourceIndex];

            const FVector Endpoints[] = { Source._Points[0], Source._Points.Last() };
            const float EndpointParams[] = { 0.0f, Source.Get_Length() };

            for (auto EndpointIndex = 0; EndpointIndex < 2; ++EndpointIndex)
            {
                for (auto TargetIndex = 0; TargetIndex < Polylines.Num(); ++TargetIndex)
                {
                    const auto& Target = Polylines[TargetIndex];
                    const auto Projection = Project_OntoPolyline(Target, Endpoints[EndpointIndex]);

                    if (Projection._Distance > SnapRadius)
                    { continue; }

                    // Interior only — projections near the target's own ends are endpoint fusion,
                    // handled by node clustering below.
                    if (Projection._DistAlong < SnapRadius || Projection._DistAlong > Target.Get_Length() - SnapRadius)
                    { continue; }

                    // Self-touch: ignore projections near the endpoint's own polyline parameter
                    // (every endpoint trivially projects onto itself).
                    if (TargetIndex == SourceIndex &&
                        FMath::Abs(Projection._DistAlong - EndpointParams[EndpointIndex]) < 2.0f * SnapRadius)
                    { continue; }

                    CutsPerPolyline[TargetIndex].Add(Projection._DistAlong);
                }
            }
        }

        auto SplitPolylines = TArray<FWorkingPolyline>{};

        for (auto Index = 0; Index < Polylines.Num(); ++Index)
        {
            auto& Cuts = CutsPerPolyline[Index];

            if (Cuts.IsEmpty())
            {
                SplitPolylines.Add(MoveTemp(Polylines[Index]));
                continue;
            }

            Cuts.Sort();

            // Dedupe cuts closer than the snap radius along the polyline (multiple endpoints
            // touching the same spot become one junction).
            auto DedupedCuts = TArray<float>{};
            for (const auto Cut : Cuts)
            {
                if (DedupedCuts.IsEmpty() || Cut - DedupedCuts.Last() > SnapRadius)
                { DedupedCuts.Add(Cut); }
            }

            SplitPolylines.Append(Split_Polyline(Polylines[Index], DedupedCuts));
        }

        // Degenerate pieces (a cut landing within float noise of an end) have near-zero length.
        SplitPolylines.RemoveAll([](const FWorkingPolyline& InPolyline)
        { return InPolyline.Get_Length() < UE_KINDA_SMALL_NUMBER; });

        // ------------------------------------------------------------------------------------------------------------
        // Node clustering: union-find over all endpoints within SnapRadius of each other.
        // ------------------------------------------------------------------------------------------------------------

        struct FEndpoint
        {
            FVector _Location;
            int32 _PolylineIndex;
        };

        auto Endpoints = TArray<FEndpoint>{};
        for (auto Index = 0; Index < SplitPolylines.Num(); ++Index)
        {
            Endpoints.Add(FEndpoint{SplitPolylines[Index]._Points[0], Index});
            Endpoints.Add(FEndpoint{SplitPolylines[Index]._Points.Last(), Index});
        }

        auto UnionFind = FUnionFind{Endpoints.Num()};

        for (auto IndexA = 0; IndexA < Endpoints.Num(); ++IndexA)
        {
            for (auto IndexB = IndexA + 1; IndexB < Endpoints.Num(); ++IndexB)
            {
                if (FVector::Dist(Endpoints[IndexA]._Location, Endpoints[IndexB]._Location) <= SnapRadius)
                { UnionFind.Union(IndexA, IndexB); }
            }
        }

        auto RootToNodeId = TMap<int32, int32>{};

        for (auto Index = 0; Index < Endpoints.Num(); ++Index)
        {
            const auto Root = UnionFind.Find(Index);

            if (RootToNodeId.Contains(Root))
            { continue; }

            RootToNodeId.Add(Root, Network._Nodes.Num());
            Network._Nodes.Add(FBuiltNode{});
        }

        // Centroids.
        auto ClusterCounts = TArray<int32>{};
        ClusterCounts.SetNumZeroed(Network._Nodes.Num());

        for (auto Index = 0; Index < Endpoints.Num(); ++Index)
        {
            const auto NodeId = RootToNodeId[UnionFind.Find(Index)];
            Network._Nodes[NodeId]._Location += Endpoints[Index]._Location;
            ++ClusterCounts[NodeId];
        }

        for (auto NodeId = 0; NodeId < Network._Nodes.Num(); ++NodeId)
        { Network._Nodes[NodeId]._Location /= static_cast<double>(ClusterCounts[NodeId]); }

        // ------------------------------------------------------------------------------------------------------------
        // Edges: snap end points to node centroids, finalize geometry.
        // ------------------------------------------------------------------------------------------------------------

        for (auto Index = 0; Index < SplitPolylines.Num(); ++Index)
        {
            auto& Polyline = SplitPolylines[Index];

            const auto StartNodeId = RootToNodeId[UnionFind.Find(Index * 2)];
            const auto EndNodeId = RootToNodeId[UnionFind.Find(Index * 2 + 1)];

            Polyline._Points[0] = Network._Nodes[StartNodeId]._Location;
            Polyline._Points.Last() = Network._Nodes[EndNodeId]._Location;
            Recompute_CumulativeLengths(Polyline);

            // Self-loop noise: both ends fused into one node and barely any geometry between.
            if (StartNodeId == EndNodeId && Polyline.Get_Length() < 2.0f * SnapRadius)
            { continue; }

            if (Polyline.Get_Length() < UE_KINDA_SMALL_NUMBER)
            { continue; }

            const auto EdgeId = Network._Edges.Num();

            auto Edge = FBuiltEdge{};
            Edge._NodeA = StartNodeId;
            Edge._NodeB = EndNodeId;
            Edge._SourceRibbonId = Polyline._SourceRibbonId;
            Edge._Points = MoveTemp(Polyline._Points);
            Edge._HalfWidths = MoveTemp(Polyline._HalfWidths);
            Edge._CumulativeLengths = MoveTemp(Polyline._CumulativeLengths);
            Edge._Length = Edge._CumulativeLengths.Last();

            Network._Edges.Add(MoveTemp(Edge));
            Network._Nodes[StartNodeId]._EdgeIds.Add(EdgeId);

            if (EndNodeId != StartNodeId)
            { Network._Nodes[EndNodeId]._EdgeIds.Add(EdgeId); }
        }

        // ------------------------------------------------------------------------------------------------------------
        // Chunk grid. Cells cover the network bounds inflated by the widest half-width, so
        // Query_EdgesNear from just off the network still lands in populated cells.
        // ------------------------------------------------------------------------------------------------------------

        if (Network._Edges.IsEmpty())
        {
            pathnetwork::Warning(TEXT("Build_NetworkFromRibbons: [{}] input ribbons produced an empty network"), InRibbons.Num());
            return Network;
        }

        auto MaxHalfWidth = 0.0f;
        auto Bounds = FBox{ForceInit};

        for (const auto& Edge : Network._Edges)
        {
            for (const auto& Point : Edge._Points)
            { Bounds += Point; }

            for (const auto HalfWidth : Edge._HalfWidths)
            { MaxHalfWidth = FMath::Max(MaxHalfWidth, HalfWidth); }
        }

        const auto Pad = MaxHalfWidth + Network._ChunkSize;
        Bounds = Bounds.ExpandBy(FVector{Pad, Pad, 0.0});

        Network._GridOrigin = FVector{Bounds.Min.X, Bounds.Min.Y, 0.0};
        Network._ChunkCountX = FMath::Max(1, FMath::CeilToInt32((Bounds.Max.X - Bounds.Min.X) / Network._ChunkSize));
        Network._ChunkCountY = FMath::Max(1, FMath::CeilToInt32((Bounds.Max.Y - Bounds.Min.Y) / Network._ChunkSize));

        Network._ChunkEdgeIds.SetNum(Network._ChunkCountX * Network._ChunkCountY);
        Network._ChunkVersions.Init(1, Network._ChunkCountX * Network._ChunkCountY);

        for (auto EdgeId = 0; EdgeId < Network._Edges.Num(); ++EdgeId)
        {
            const auto& Edge = Network._Edges[EdgeId];

            auto EdgeMaxHalfWidth = 0.0f;
            for (const auto HalfWidth : Edge._HalfWidths)
            { EdgeMaxHalfWidth = FMath::Max(EdgeMaxHalfWidth, HalfWidth); }

            auto TouchedCells = TSet<int32>{};

            for (auto Index = 0; Index < Edge._Points.Num() - 1; ++Index)
            {
                const auto& A = Edge._Points[Index];
                const auto& B = Edge._Points[Index + 1];

                const auto MinX = FMath::Clamp(FMath::FloorToInt32((FMath::Min(A.X, B.X) - EdgeMaxHalfWidth - Network._GridOrigin.X) / Network._ChunkSize), 0, Network._ChunkCountX - 1);
                const auto MaxX = FMath::Clamp(FMath::FloorToInt32((FMath::Max(A.X, B.X) + EdgeMaxHalfWidth - Network._GridOrigin.X) / Network._ChunkSize), 0, Network._ChunkCountX - 1);
                const auto MinY = FMath::Clamp(FMath::FloorToInt32((FMath::Min(A.Y, B.Y) - EdgeMaxHalfWidth - Network._GridOrigin.Y) / Network._ChunkSize), 0, Network._ChunkCountY - 1);
                const auto MaxY = FMath::Clamp(FMath::FloorToInt32((FMath::Max(A.Y, B.Y) + EdgeMaxHalfWidth - Network._GridOrigin.Y) / Network._ChunkSize), 0, Network._ChunkCountY - 1);

                for (auto CellY = MinY; CellY <= MaxY; ++CellY)
                {
                    for (auto CellX = MinX; CellX <= MaxX; ++CellX)
                    { TouchedCells.Add(CellY * Network._ChunkCountX + CellX); }
                }
            }

            for (const auto CellIndex : TouchedCells)
            { Network._ChunkEdgeIds[CellIndex].Add(EdgeId); }
        }

        pathnetwork::Verbose(TEXT("Build_NetworkFromRibbons: [{}] ribbons -> [{}] nodes, [{}] edges, [{}]x[{}] chunks"),
            InRibbons.Num(), Network._Nodes.Num(), Network._Edges.Num(), Network._ChunkCountX, Network._ChunkCountY);

        return Network;
    }
}

// --------------------------------------------------------------------------------------------------------------------
