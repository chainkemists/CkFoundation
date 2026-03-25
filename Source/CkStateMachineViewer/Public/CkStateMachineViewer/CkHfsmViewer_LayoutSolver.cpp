#include "CkHfsmViewer_LayoutSolver.h"
#include "CkHfsmViewer_GraphRenderer.h"

// --------------------------------------------------------------------------------------------------------------------
// Phase 0: DFS Cycle Removal
// --------------------------------------------------------------------------------------------------------------------

namespace
{

enum class EDfsColor : uint8 { White, Grey, Black };

} // anonymous namespace

auto
    FCkHfsmViewer_LayoutSolver::
    RemoveCycles(
        TArray<FCkHfsmViewer_LayoutEdge>& InOutEdges,
        int32 InNodeCount,
        int32 InRootIndex)
    -> void
{
    auto Colors = TArray<EDfsColor>{};
    Colors.SetNum(InNodeCount);
    for (auto& C : Colors) { C = EDfsColor::White; }

    // Build adjacency mapping edge indices to source nodes
    auto AdjEdgeIndices = TArray<TArray<int32>>{};
    AdjEdgeIndices.SetNum(InNodeCount);

    for (auto EdgeIdx = 0; EdgeIdx < InOutEdges.Num(); ++EdgeIdx)
    {
        const auto& Edge = InOutEdges[EdgeIdx];
        if (Edge.Source >= 0 && Edge.Source < InNodeCount)
        {
            AdjEdgeIndices[Edge.Source].Add(EdgeIdx);
        }
    }

    // Iterative DFS using explicit stack (avoids recursion depth issues)
    struct FDfsFrame
    {
        int32 NodeIndex = -1;
        int32 EdgeListPos = 0;
    };

    auto DfsFromNode = [&](int32 InStartNode)
    {
        if (Colors[InStartNode] != EDfsColor::White)
        { return; }

        auto Stack = TArray<FDfsFrame>{};
        Stack.Add({InStartNode, 0});
        Colors[InStartNode] = EDfsColor::Grey;

        while (NOT Stack.IsEmpty())
        {
            auto& Frame = Stack.Last();
            const auto& EdgeList = AdjEdgeIndices[Frame.NodeIndex];

            if (Frame.EdgeListPos >= EdgeList.Num())
            {
                Colors[Frame.NodeIndex] = EDfsColor::Black;
                Stack.Pop();
                continue;
            }

            auto EdgeIdx = EdgeList[Frame.EdgeListPos];
            ++Frame.EdgeListPos;

            auto& Edge = InOutEdges[EdgeIdx];
            auto Target = Edge.Target;

            if (Target < 0 || Target >= InNodeCount)
            { continue; }

            if (Colors[Target] == EDfsColor::Grey)
            {
                // Back-edge: reverse it
                Edge.IsReversed = true;
                auto Temp = Edge.Source;
                Edge.Source = Edge.Target;
                Edge.Target = Temp;
            }
            else if (Colors[Target] == EDfsColor::White)
            {
                Colors[Target] = EDfsColor::Grey;
                Stack.Add({Target, 0});
            }
        }
    };

    // DFS from root first, then any unvisited (disconnected components)
    DfsFromNode(InRootIndex);

    for (auto NodeIdx = 0; NodeIdx < InNodeCount; ++NodeIdx)
    {
        DfsFromNode(NodeIdx);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Phase 1: Longest-Path Ranking
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_LayoutSolver::
    ComputeLongestPathRanking(
        const TArray<FCkHfsmViewer_LayoutEdge>& InEdges,
        int32 InNodeCount,
        int32 InRootIndex)
    -> TArray<int32>
{
    auto Ranks = TArray<int32>{};
    Ranks.SetNum(InNodeCount);
    for (auto& R : Ranks) { R = 0; }

    // Build DAG adjacency (edges already have correct direction after cycle removal)
    auto Successors = TArray<TArray<int32>>{};
    Successors.SetNum(InNodeCount);

    auto InDegree = TArray<int32>{};
    InDegree.SetNum(InNodeCount);
    for (auto& D : InDegree) { D = 0; }

    for (const auto& Edge : InEdges)
    {
        if (Edge.Source < 0 || Edge.Source >= InNodeCount) { continue; }
        if (Edge.Target < 0 || Edge.Target >= InNodeCount) { continue; }

        Successors[Edge.Source].Add(Edge.Target);
        ++InDegree[Edge.Target];
    }

    // Topological processing (Kahn's algorithm)
    auto Queue = TArray<int32>{};

    // Seed with root first (if it has in-degree 0), then other zero-in-degree nodes
    if (InDegree[InRootIndex] == 0)
    {
        Queue.Add(InRootIndex);
    }

    for (auto NodeIdx = 0; NodeIdx < InNodeCount; ++NodeIdx)
    {
        if (InDegree[NodeIdx] == 0 && NodeIdx != InRootIndex)
        {
            Queue.Add(NodeIdx);
        }
    }

    auto Processed = TArray<bool>{};
    Processed.SetNum(InNodeCount);
    for (auto& P : Processed) { P = false; }

    for (auto Head = 0; Head < Queue.Num(); ++Head)
    {
        auto Current = Queue[Head];
        Processed[Current] = true;

        for (auto Successor : Successors[Current])
        {
            Ranks[Successor] = FMath::Max(Ranks[Successor], Ranks[Current] + 1);
            --InDegree[Successor];

            if (InDegree[Successor] == 0)
            {
                Queue.Add(Successor);
            }
        }
    }

    // Assign unprocessed nodes (shouldn't happen after cycle removal, but safety)
    auto CurrentMaxRank = 0;
    for (auto R : Ranks) { CurrentMaxRank = FMath::Max(CurrentMaxRank, R); }

    for (auto NodeIdx = 0; NodeIdx < InNodeCount; ++NodeIdx)
    {
        if (NOT Processed[NodeIdx])
        {
            Ranks[NodeIdx] = CurrentMaxRank + 1;
        }
    }

    return Ranks;
}

// --------------------------------------------------------------------------------------------------------------------
// Phase 1.5: Dummy Node Insertion
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_LayoutSolver::
    InsertDummyNodes(
        const FCkHfsmViewer_GraphRenderer& InRenderer)
    -> void
{
    auto OriginalEdgeCount = Edges.Num();

    for (auto EdgeIdx = OriginalEdgeCount - 1; EdgeIdx >= 0; --EdgeIdx)
    {
        auto Edge = Edges[EdgeIdx];
        auto SrcRank = Nodes[Edge.Source].Rank;
        auto TgtRank = Nodes[Edge.Target].Rank;
        auto Span = TgtRank - SrcRank;

        if (Span <= 1)
        { continue; }

        // Remove original edge, replace with chain
        Edges.RemoveAt(EdgeIdx);

        auto PrevNode = Edge.Source;

        for (auto R = SrcRank + 1; R < TgtRank; ++R)
        {
            auto DummyIndex = Nodes.Num();

            auto DummyRoutingHeight = InRenderer.Get_LayoutDummyNodeHeight();
            auto DummyNode = FCkHfsmViewer_LayoutNode{};
            DummyNode.IsDummy = true;
            DummyNode.Width = 4.0f;
            DummyNode.Height = DummyRoutingHeight;
            DummyNode.Rank = R;
            Nodes.Add(MoveTemp(DummyNode));

            auto ChainEdge = FCkHfsmViewer_LayoutEdge{};
            ChainEdge.Source = PrevNode;
            ChainEdge.Target = DummyIndex;
            ChainEdge.OriginalTransitionIndex = Edge.OriginalTransitionIndex;
            Edges.Add(MoveTemp(ChainEdge));

            PrevNode = DummyIndex;
        }

        // Final edge in chain
        auto FinalEdge = FCkHfsmViewer_LayoutEdge{};
        FinalEdge.Source = PrevNode;
        FinalEdge.Target = Edge.Target;
        FinalEdge.OriginalTransitionIndex = Edge.OriginalTransitionIndex;
        Edges.Add(MoveTemp(FinalEdge));
    }

    // Rebuild rank buckets with dummy nodes
    MaxRank = 0;
    for (const auto& Node : Nodes)
    {
        MaxRank = FMath::Max(MaxRank, Node.Rank);
    }

    RankBuckets.Reset();
    RankBuckets.SetNum(MaxRank + 1);

    for (auto NodeIdx = 0; NodeIdx < Nodes.Num(); ++NodeIdx)
    {
        RankBuckets[Nodes[NodeIdx].Rank].Add(NodeIdx);
        Nodes[NodeIdx].PositionInRank = RankBuckets[Nodes[NodeIdx].Rank].Num() - 1;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Phase 2: Crossing Minimization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_LayoutSolver::
    CountCrossings() const
    -> int32
{
    auto Total = 0;

    for (auto RankIdx = 0; RankIdx < MaxRank; ++RankIdx)
    {
        // Collect edges between this rank and the next
        struct FRankEdge
        {
            int32 UpperPos = 0;
            int32 LowerPos = 0;
        };

        auto RankEdges = TArray<FRankEdge>{};

        for (const auto& Edge : Edges)
        {
            if (Nodes[Edge.Source].Rank == RankIdx && Nodes[Edge.Target].Rank == RankIdx + 1)
            {
                RankEdges.Add({Nodes[Edge.Source].PositionInRank, Nodes[Edge.Target].PositionInRank});
            }
        }

        // Count inversions (crossings)
        for (auto I = 0; I < RankEdges.Num(); ++I)
        {
            for (auto J = I + 1; J < RankEdges.Num(); ++J)
            {
                auto CrossesUpperLower = (RankEdges[I].UpperPos < RankEdges[J].UpperPos && RankEdges[I].LowerPos > RankEdges[J].LowerPos);
                auto CrossesLowerUpper = (RankEdges[I].UpperPos > RankEdges[J].UpperPos && RankEdges[I].LowerPos < RankEdges[J].LowerPos);

                if (CrossesUpperLower || CrossesLowerUpper)
                {
                    ++Total;
                }
            }
        }
    }

    return Total;
}

auto
    FCkHfsmViewer_LayoutSolver::
    UpdatePositionsInRank()
    -> void
{
    for (auto RankIdx = 0; RankIdx <= MaxRank; ++RankIdx)
    {
        const auto& Bucket = RankBuckets[RankIdx];
        for (auto Pos = 0; Pos < Bucket.Num(); ++Pos)
        {
            Nodes[Bucket[Pos]].PositionInRank = Pos;
        }
    }
}

auto
    FCkHfsmViewer_LayoutSolver::
    BarycenterSweepForward()
    -> void
{
    for (auto RankIdx = 1; RankIdx <= MaxRank; ++RankIdx)
    {
        auto& Bucket = RankBuckets[RankIdx];

        auto Barycenters = TArray<TPair<float, int32>>{};

        for (auto NodeIdx : Bucket)
        {
            auto Sum = 0.0f;
            auto Count = 0;

            // Predecessors only: edges where Target == NodeIdx and Source is in rank-1
            for (const auto& Edge : Edges)
            {
                if (Edge.Target == NodeIdx && Nodes[Edge.Source].Rank == RankIdx - 1)
                {
                    Sum += static_cast<float>(Nodes[Edge.Source].PositionInRank);
                    ++Count;
                }
            }

            auto Bc = Count > 0 ? Sum / static_cast<float>(Count) : static_cast<float>(Nodes[NodeIdx].PositionInRank);
            Barycenters.Add({Bc, NodeIdx});
        }

        Barycenters.Sort([](const auto& A, const auto& B)
        {
            if (A.Key != B.Key) { return A.Key < B.Key; }
            return A.Value < B.Value;
        });

        Bucket.Reset();
        for (const auto& Pair : Barycenters)
        {
            Bucket.Add(Pair.Value);
        }
    }

    UpdatePositionsInRank();
}

auto
    FCkHfsmViewer_LayoutSolver::
    BarycenterSweepBackward()
    -> void
{
    for (auto RankIdx = MaxRank - 1; RankIdx >= 0; --RankIdx)
    {
        auto& Bucket = RankBuckets[RankIdx];

        auto Barycenters = TArray<TPair<float, int32>>{};

        for (auto NodeIdx : Bucket)
        {
            auto Sum = 0.0f;
            auto Count = 0;

            // Successors only: edges where Source == NodeIdx and Target is in rank+1
            for (const auto& Edge : Edges)
            {
                if (Edge.Source == NodeIdx && Nodes[Edge.Target].Rank == RankIdx + 1)
                {
                    Sum += static_cast<float>(Nodes[Edge.Target].PositionInRank);
                    ++Count;
                }
            }

            auto Bc = Count > 0 ? Sum / static_cast<float>(Count) : static_cast<float>(Nodes[NodeIdx].PositionInRank);
            Barycenters.Add({Bc, NodeIdx});
        }

        Barycenters.Sort([](const auto& A, const auto& B)
        {
            if (A.Key != B.Key) { return A.Key < B.Key; }
            return A.Value < B.Value;
        });

        Bucket.Reset();
        for (const auto& Pair : Barycenters)
        {
            Bucket.Add(Pair.Value);
        }
    }

    UpdatePositionsInRank();
}

auto
    FCkHfsmViewer_LayoutSolver::
    TransposeStep()
    -> bool
{
    auto Improved = false;

    for (auto RankIdx = 0; RankIdx <= MaxRank; ++RankIdx)
    {
        auto& Bucket = RankBuckets[RankIdx];

        for (auto Pos = 0; Pos < Bucket.Num() - 1; ++Pos)
        {
            auto CrossingsBefore = CountCrossings();

            // Try swap
            Swap(Bucket[Pos], Bucket[Pos + 1]);
            UpdatePositionsInRank();

            auto CrossingsAfter = CountCrossings();

            if (CrossingsAfter < CrossingsBefore)
            {
                Improved = true;
            }
            else
            {
                // Revert swap
                Swap(Bucket[Pos], Bucket[Pos + 1]);
                UpdatePositionsInRank();
            }
        }
    }

    return Improved;
}

auto
    FCkHfsmViewer_LayoutSolver::
    MinimizeCrossings(
        const FCkHfsmViewer_GraphRenderer& InRenderer)
    -> void
{
    auto NumRuns = InRenderer.Get_LayoutCrossingMinRuns();
    auto MaxFails = InRenderer.Get_LayoutCrossingMaxFails();

    auto BestCrossings = TNumericLimits<int32>::Max();
    auto BestBuckets = RankBuckets;

    // Use deterministic seed based on graph structure
    auto Seed = static_cast<uint32>(Nodes.Num() * 31 + Edges.Num() * 17);

    for (auto Run = 0; Run < NumRuns; ++Run)
    {
        if (Run > 0)
        {
            // Random shuffle of each rank bucket
            auto Rng = FRandomStream{static_cast<int32>(Seed + Run)};
            for (auto RankIdx = 0; RankIdx <= MaxRank; ++RankIdx)
            {
                auto& Bucket = RankBuckets[RankIdx];
                for (auto I = Bucket.Num() - 1; I > 0; --I)
                {
                    auto J = Rng.RandRange(0, I);
                    Swap(Bucket[I], Bucket[J]);
                }
            }
            UpdatePositionsInRank();
        }

        auto FailCount = 0;
        auto RunBestCrossings = CountCrossings();
        auto RunBestBuckets = RankBuckets;

        while (FailCount < MaxFails)
        {
            BarycenterSweepForward();

            // Transpose until no improvement
            while (TransposeStep()) {}

            BarycenterSweepBackward();

            while (TransposeStep()) {}

            auto CurrentCrossings = CountCrossings();

            if (CurrentCrossings < RunBestCrossings)
            {
                RunBestCrossings = CurrentCrossings;
                RunBestBuckets = RankBuckets;
                FailCount = 0;
            }
            else
            {
                ++FailCount;
            }

            if (RunBestCrossings == 0)
            { break; }
        }

        if (RunBestCrossings < BestCrossings)
        {
            BestCrossings = RunBestCrossings;
            BestBuckets = RunBestBuckets;
        }

        if (BestCrossings == 0)
        { break; }
    }

    RankBuckets = BestBuckets;
    UpdatePositionsInRank();
}

// --------------------------------------------------------------------------------------------------------------------
// Phase 3: Coordinate Assignment (Iterative Median)
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_LayoutSolver::
    AssignCoordinates(
        const FCkHfsmViewer_GraphRenderer& InRenderer)
    -> void
{
    auto HorizontalGap = InRenderer.Get_LayoutHorizontalGap();
    auto VerticalGap = InRenderer.Get_LayoutVerticalGap();

    // Compute per-rank max widths and X positions
    auto RankX = TArray<float>{};
    RankX.SetNum(MaxRank + 1);

    auto RankMaxWidth = TArray<float>{};
    RankMaxWidth.SetNum(MaxRank + 1);
    for (auto& W : RankMaxWidth) { W = 0.0f; }

    for (const auto& Node : Nodes)
    {
        RankMaxWidth[Node.Rank] = FMath::Max(RankMaxWidth[Node.Rank], Node.Width);
    }

    auto CumulativeX = 0.0f;
    for (auto RankIdx = 0; RankIdx <= MaxRank; ++RankIdx)
    {
        RankX[RankIdx] = CumulativeX;
        CumulativeX += RankMaxWidth[RankIdx] + HorizontalGap;
    }

    // Assign X from rank
    for (auto& Node : Nodes)
    {
        Node.X = RankX[Node.Rank];
    }

    // Initial Y placement: center each rank vertically
    for (auto RankIdx = 0; RankIdx <= MaxRank; ++RankIdx)
    {
        const auto& Bucket = RankBuckets[RankIdx];
        auto TotalHeight = 0.0f;

        for (auto Pos = 0; Pos < Bucket.Num(); ++Pos)
        {
            if (Pos > 0) { TotalHeight += VerticalGap; }
            TotalHeight += Nodes[Bucket[Pos]].Height;
        }

        auto CurrentY = -TotalHeight * 0.5f;

        for (auto Pos = 0; Pos < Bucket.Num(); ++Pos)
        {
            auto NodeIdx = Bucket[Pos];
            Nodes[NodeIdx].Y = CurrentY;
            CurrentY += Nodes[NodeIdx].Height + VerticalGap;
        }
    }

    // Build neighbor lists for barycenter computation
    // Each node tracks all connected neighbors (via edges)
    auto Neighbors = TArray<TArray<int32>>{};
    Neighbors.SetNum(Nodes.Num());

    for (const auto& Edge : Edges)
    {
        Neighbors[Edge.Source].Add(Edge.Target);
        Neighbors[Edge.Target].Add(Edge.Source);
    }

    // Iterative barycenter refinement
    // Use weighted mean of all neighbors (closer ranks = higher weight)
    auto BarycenterIterations = InRenderer.Get_LayoutBarycenterIterations();

    for (auto Iteration = 0; Iteration < BarycenterIterations; ++Iteration)
    {
        // Forward pass: rank 1 → MaxRank
        for (auto RankIdx = 1; RankIdx <= MaxRank; ++RankIdx)
        {
            const auto& Bucket = RankBuckets[RankIdx];

            for (auto NodeIdx : Bucket)
            {
                const auto& Nbrs = Neighbors[NodeIdx];

                if (Nbrs.IsEmpty())
                { continue; }

                auto WeightedSum = 0.0f;
                auto TotalWeight = 0.0f;

                for (auto NbrIdx : Nbrs)
                {
                    auto NbrCenterY = Nodes[NbrIdx].Y + Nodes[NbrIdx].Height * 0.5f;
                    auto RankDist = FMath::Abs(Nodes[NbrIdx].Rank - Nodes[NodeIdx].Rank);
                    auto Weight = 1.0f / FMath::Max(1.0f, static_cast<float>(RankDist));
                    WeightedSum += NbrCenterY * Weight;
                    TotalWeight += Weight;
                }

                if (TotalWeight > 0.0f)
                {
                    auto TargetY = WeightedSum / TotalWeight - Nodes[NodeIdx].Height * 0.5f;
                    Nodes[NodeIdx].Y = TargetY;
                }
            }

            EnforceMinSpacing(RankBuckets[RankIdx], VerticalGap);
        }

        // Backward pass: MaxRank-1 → 0
        for (auto RankIdx = MaxRank - 1; RankIdx >= 0; --RankIdx)
        {
            const auto& Bucket = RankBuckets[RankIdx];

            for (auto NodeIdx : Bucket)
            {
                const auto& Nbrs = Neighbors[NodeIdx];

                if (Nbrs.IsEmpty())
                { continue; }

                auto WeightedSum = 0.0f;
                auto TotalWeight = 0.0f;

                for (auto NbrIdx : Nbrs)
                {
                    auto NbrCenterY = Nodes[NbrIdx].Y + Nodes[NbrIdx].Height * 0.5f;
                    auto RankDist = FMath::Abs(Nodes[NbrIdx].Rank - Nodes[NodeIdx].Rank);
                    auto Weight = 1.0f / FMath::Max(1.0f, static_cast<float>(RankDist));
                    WeightedSum += NbrCenterY * Weight;
                    TotalWeight += Weight;
                }

                if (TotalWeight > 0.0f)
                {
                    auto TargetY = WeightedSum / TotalWeight - Nodes[NodeIdx].Height * 0.5f;
                    Nodes[NodeIdx].Y = TargetY;
                }
            }

            EnforceMinSpacing(RankBuckets[RankIdx], VerticalGap);
        }
    }
}

auto
    FCkHfsmViewer_LayoutSolver::
    EnforceMinSpacing(
        const TArray<int32>& InBucket,
        float InVerticalGap)
    -> void
{
    if (InBucket.Num() <= 1)
    { return; }

    // Forward pass: push nodes down if overlapping
    for (auto Pos = 1; Pos < InBucket.Num(); ++Pos)
    {
        auto PrevIdx = InBucket[Pos - 1];
        auto CurrIdx = InBucket[Pos];
        auto MinY = Nodes[PrevIdx].Y + Nodes[PrevIdx].Height + InVerticalGap;

        if (Nodes[CurrIdx].Y < MinY)
        {
            Nodes[CurrIdx].Y = MinY;
        }
    }

    // Backward pass: push nodes up if overlapping (center the group)
    for (auto Pos = InBucket.Num() - 2; Pos >= 0; --Pos)
    {
        auto NextIdx = InBucket[Pos + 1];
        auto CurrIdx = InBucket[Pos];
        auto MaxY = Nodes[NextIdx].Y - InVerticalGap - Nodes[CurrIdx].Height;

        if (Nodes[CurrIdx].Y > MaxY)
        {
            Nodes[CurrIdx].Y = MaxY;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Write-Back + Edge Route Extraction
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_LayoutSolver::
    WriteBack(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates,
        TArray<FCkHfsmViewer_TransitionInfo>& InOutTransitions)
    -> void
{
    // Center the layout around (0, 0)
    auto MinX = TNumericLimits<float>::Max();
    auto MaxX = TNumericLimits<float>::Lowest();
    auto MinY = TNumericLimits<float>::Max();
    auto MaxY = TNumericLimits<float>::Lowest();

    for (const auto& Node : Nodes)
    {
        if (NOT Node.IsDummy)
        {
            MinX = FMath::Min(MinX, Node.X);
            MaxX = FMath::Max(MaxX, Node.X + Node.Width);
            MinY = FMath::Min(MinY, Node.Y);
            MaxY = FMath::Max(MaxY, Node.Y + Node.Height);
        }
    }

    auto CenterX = (MinX + MaxX) * 0.5f;
    auto CenterY = (MinY + MaxY) * 0.5f;

    // Write node positions
    for (const auto& Node : Nodes)
    {
        if (NOT Node.IsDummy && Node.OriginalStateIndex >= 0)
        {
            InOutStates[Node.OriginalStateIndex].NodePosition = {
                Node.X - CenterX,
                Node.Y - CenterY
            };
        }
    }

    // Extract edge waypoints from dummy node chains
    // Group edges by OriginalTransitionIndex to find chains
    auto TransitionChains = TMap<int32, TArray<int32>>{};

    for (const auto& Edge : Edges)
    {
        if (Edge.OriginalTransitionIndex >= 0)
        {
            // Collect dummy nodes that are part of this transition's chain
            if (Nodes[Edge.Source].IsDummy)
            {
                auto& Chain = TransitionChains.FindOrAdd(Edge.OriginalTransitionIndex);
                Chain.AddUnique(Edge.Source);
            }
            if (Nodes[Edge.Target].IsDummy)
            {
                auto& Chain = TransitionChains.FindOrAdd(Edge.OriginalTransitionIndex);
                Chain.AddUnique(Edge.Target);
            }
        }
    }

    for (auto& [TransIdx, DummyIndices] : TransitionChains)
    {
        if (TransIdx < 0 || TransIdx >= InOutTransitions.Num())
        { continue; }

        // Sort dummy nodes by rank
        DummyIndices.Sort([this](int32 A, int32 B)
        {
            return Nodes[A].Rank < Nodes[B].Rank;
        });

        auto& Waypoints = InOutTransitions[TransIdx].RouteWaypoints;
        Waypoints.Reset();

        for (auto DummyIdx : DummyIndices)
        {
            Waypoints.Add({
                Nodes[DummyIdx].X - CenterX + Nodes[DummyIdx].Width * 0.5f,
                Nodes[DummyIdx].Y - CenterY
            });
        }

        // For back-edges, the dummy chain was built in the reversed direction.
        // Flip waypoints so they go from actual source to actual target.
        if (ReversedTransitionIndices.Contains(TransIdx))
        {
            Algo::Reverse(Waypoints);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Main Entry Point
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_LayoutSolver::
    Solve(
        TArray<FCkHfsmViewer_StateInfo>& InOutStates,
        TArray<FCkHfsmViewer_TransitionInfo>& InOutTransitions,
        int32 InRootIndex,
        const FCkHfsmViewer_GraphRenderer& InRenderer)
    -> void
{
    RealNodeCount = InOutStates.Num();

    if (RealNodeCount == 0)
    { return; }

    // Initialize layout nodes from states
    Nodes.Reset();
    Nodes.Reserve(RealNodeCount * 2);

    for (auto Index = 0; Index < RealNodeCount; ++Index)
    {
        auto LayoutNode = FCkHfsmViewer_LayoutNode{};
        LayoutNode.OriginalStateIndex = Index;
        LayoutNode.Width = InRenderer.ComputeNodeWidth(InOutStates[Index]);
        LayoutNode.Height = InRenderer.ComputeNodeHeight(InOutStates[Index]);
        Nodes.Add(MoveTemp(LayoutNode));
    }

    if (RealNodeCount == 1)
    {
        InOutStates[0].NodePosition = {-Nodes[0].Width * 0.5f, -Nodes[0].Height * 0.5f};
        return;
    }

    // Initialize layout edges from transitions (skip self-loops)
    Edges.Reset();

    for (auto TransIdx = 0; TransIdx < InOutTransitions.Num(); ++TransIdx)
    {
        const auto& Trans = InOutTransitions[TransIdx];
        auto Src = Trans.SourceStateIndex;
        auto Tgt = Trans.TargetStateIndex;

        if (Src < 0 || Src >= RealNodeCount) { continue; }
        if (Tgt < 0 || Tgt >= RealNodeCount) { continue; }
        if (Src == Tgt) { continue; }

        auto LayoutEdge = FCkHfsmViewer_LayoutEdge{};
        LayoutEdge.Source = Src;
        LayoutEdge.Target = Tgt;
        LayoutEdge.OriginalTransitionIndex = TransIdx;
        Edges.Add(MoveTemp(LayoutEdge));
    }

    // Clear any existing route waypoints
    for (auto& Trans : InOutTransitions)
    {
        Trans.RouteWaypoints.Reset();
    }

    // Phase 0: Cycle removal
    RemoveCycles(Edges, RealNodeCount, InRootIndex);

    // Track which transitions had their edges reversed (must be before InsertDummyNodes
    // which replaces original edges with chain edges that lack IsReversed)
    ReversedTransitionIndices.Reset();
    for (const auto& Edge : Edges)
    {
        if (Edge.IsReversed && Edge.OriginalTransitionIndex >= 0)
        {
            ReversedTransitionIndices.Add(Edge.OriginalTransitionIndex);
        }
    }

    // Phase 1: Longest-path ranking
    auto Ranks = ComputeLongestPathRanking(Edges, RealNodeCount, InRootIndex);
    MaxRank = 0;

    for (auto NodeIdx = 0; NodeIdx < RealNodeCount; ++NodeIdx)
    {
        Nodes[NodeIdx].Rank = Ranks[NodeIdx];
        MaxRank = FMath::Max(MaxRank, Ranks[NodeIdx]);
    }

    // Build initial rank buckets
    RankBuckets.Reset();
    RankBuckets.SetNum(MaxRank + 1);

    for (auto NodeIdx = 0; NodeIdx < RealNodeCount; ++NodeIdx)
    {
        RankBuckets[Nodes[NodeIdx].Rank].Add(NodeIdx);
        Nodes[NodeIdx].PositionInRank = RankBuckets[Nodes[NodeIdx].Rank].Num() - 1;
    }

    // Phase 1.5: Dummy node insertion
    InsertDummyNodes(InRenderer);

    // Phase 2: Crossing minimization
    MinimizeCrossings(InRenderer);

    // Phase 3: Coordinate assignment
    AssignCoordinates(InRenderer);

    // Write back results
    WriteBack(InOutStates, InOutTransitions);
}

// --------------------------------------------------------------------------------------------------------------------
