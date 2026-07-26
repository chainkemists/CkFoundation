#include "CkPathNetwork_RouteGraph.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_routegraph
{
    // Dead-branch sentinel: Cost() is only ever called on pairs Neighbors() produced; any other
    // combination prices as unreachable rather than ensuring in the search hot loop.
    constexpr auto UnreachableCost = TNumericLimits<float>::Max() / 4.0f;

    auto
    Encode_NodeId32(const ck::pathnetwork::FRouteNodeId& InId) -> uint32
    {
        return (static_cast<uint32>(InId._Kind) << 30) | (static_cast<uint32>(InId._Index) & 0x3FFFFFFFu);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    GetTypeHash(const FRouteNodeId& InId) -> uint32
    {
        return ::GetTypeHash(ck_pathnetwork_routegraph::Encode_NodeId32(InId));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        Neighbors(const FRouteNodeId& InNode) const
        -> TArray<FRouteNodeId>
    {
        auto Result = TArray<FRouteNodeId>{};

        switch (InNode._Kind)
        {
            case ERouteNodeKind::Start:
            {
                for (auto Index = 0; Index < _Shared->_OverlayPoints.Num(); ++Index)
                { Result.Add(FRouteNodeId{ERouteNodeKind::OverlayPoint, Index}); }

                Result.Add(FRouteNodeId{ERouteNodeKind::Goal, 0});
                break;
            }
            case ERouteNodeKind::OverlayPoint:
            {
                const auto& Point = _Shared->_OverlayPoints[InNode._Index];
                const auto& Edge = _Network->_Edges[Point._EdgeId];

                Result.Add(FRouteNodeId{ERouteNodeKind::NetNode, Edge._NodeA});

                if (Edge._NodeB != Edge._NodeA)
                { Result.Add(FRouteNodeId{ERouteNodeKind::NetNode, Edge._NodeB}); }

                if (const auto* SameEdgePoints = _Shared->_OverlayPointsByEdge.Find(Point._EdgeId))
                {
                    for (const auto OtherIndex : *SameEdgePoints)
                    {
                        if (OtherIndex != InNode._Index)
                        { Result.Add(FRouteNodeId{ERouteNodeKind::OverlayPoint, OtherIndex}); }
                    }
                }

                Result.Add(FRouteNodeId{ERouteNodeKind::Goal, 0});
                break;
            }
            case ERouteNodeKind::NetNode:
            {
                const auto& Node = _Network->_Nodes[InNode._Index];

                for (const auto EdgeId : Node._EdgeIds)
                {
                    const auto& Edge = _Network->_Edges[EdgeId];
                    const auto OtherNode = Edge._NodeA == InNode._Index ? Edge._NodeB : Edge._NodeA;

                    if (OtherNode != InNode._Index)
                    { Result.Add(FRouteNodeId{ERouteNodeKind::NetNode, OtherNode}); }

                    if (const auto* EdgePoints = _Shared->_OverlayPointsByEdge.Find(EdgeId))
                    {
                        for (const auto PointIndex : *EdgePoints)
                        { Result.Add(FRouteNodeId{ERouteNodeKind::OverlayPoint, PointIndex}); }
                    }
                }
                break;
            }
            case ERouteNodeKind::Goal:
            default:
                break;
        }

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        Cost(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const
        -> float
    {
        using namespace ck_pathnetwork_routegraph;

        if (Get_IsOffPathHop(InFrom, InTo))
        { return DoGet_OffPathCost(InFrom, InTo); }

        if (InFrom._Kind == ERouteNodeKind::OverlayPoint && InTo._Kind == ERouteNodeKind::OverlayPoint)
        {
            const auto& A = _Shared->_OverlayPoints[InFrom._Index];
            const auto& B = _Shared->_OverlayPoints[InTo._Index];

            if (A._EdgeId != B._EdgeId)
            { return UnreachableCost; }

            return FMath::Abs(A._DistAlong - B._DistAlong);
        }

        if (InFrom._Kind == ERouteNodeKind::OverlayPoint && InTo._Kind == ERouteNodeKind::NetNode)
        { return DoGet_AlongEdgeCostToNode(_Shared->_OverlayPoints[InFrom._Index], InTo._Index); }

        if (InFrom._Kind == ERouteNodeKind::NetNode && InTo._Kind == ERouteNodeKind::OverlayPoint)
        { return DoGet_AlongEdgeCostToNode(_Shared->_OverlayPoints[InTo._Index], InFrom._Index); }

        if (InFrom._Kind == ERouteNodeKind::NetNode && InTo._Kind == ERouteNodeKind::NetNode)
        {
            const auto& Node = _Network->_Nodes[InFrom._Index];
            auto BestLength = UnreachableCost;

            for (const auto EdgeId : Node._EdgeIds)
            {
                const auto& Edge = _Network->_Edges[EdgeId];
                const auto OtherNode = Edge._NodeA == InFrom._Index ? Edge._NodeB : Edge._NodeA;

                if (OtherNode == InTo._Index)
                { BestLength = FMath::Min(BestLength, Edge._Length); }
            }

            return BestLength;
        }

        return UnreachableCost;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        Heuristic(const FRouteNodeId& InCurrent, const FRouteNodeId& InGoal) const
        -> float
    {
        // Admissible: nothing travels cheaper than 1x per cm (on-network rate).
        return static_cast<float>(FVector::Dist(Get_NodeLocation(InCurrent), _GoalLocation));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        IsGoal(const FRouteNodeId& InNode) const
        -> bool
    {
        return InNode._Kind == ERouteNodeKind::Goal;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        Get_NodeLocation(const FRouteNodeId& InNode) const
        -> FVector
    {
        switch (InNode._Kind)
        {
            case ERouteNodeKind::Start: return _StartLocation;
            case ERouteNodeKind::Goal: return _GoalLocation;
            case ERouteNodeKind::NetNode: return _Network->_Nodes[InNode._Index]._Location;
            case ERouteNodeKind::OverlayPoint: return _Shared->_OverlayPoints[InNode._Index]._Location;
            default: return FVector::ZeroVector;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        Get_IsOffPathHop(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const
        -> bool
    {
        return InFrom._Kind == ERouteNodeKind::Start || InTo._Kind == ERouteNodeKind::Goal;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        PackOffPathKey(const FRouteNodeId& InFrom, const FRouteNodeId& InTo)
        -> uint64
    {
        using namespace ck_pathnetwork_routegraph;
        return (static_cast<uint64>(Encode_NodeId32(InFrom)) << 32) | static_cast<uint64>(Encode_NodeId32(InTo));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        DoGet_AlongEdgeCostToNode(const FRouteOverlayPoint& InPoint, int32 InNodeId) const
        -> float
    {
        using namespace ck_pathnetwork_routegraph;

        const auto& Edge = _Network->_Edges[InPoint._EdgeId];
        auto BestCost = UnreachableCost;

        if (Edge._NodeA == InNodeId)
        { BestCost = FMath::Min(BestCost, InPoint._DistAlong); }

        if (Edge._NodeB == InNodeId)
        { BestCost = FMath::Min(BestCost, Edge._Length - InPoint._DistAlong); }

        return BestCost;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRouteGraph::
        DoGet_OffPathCost(const FRouteNodeId& InFrom, const FRouteNodeId& InTo) const
        -> float
    {
        if (const auto* Repriced = _Shared->_RepricedOffPathCosts.Find(PackOffPathKey(InFrom, InTo)))
        { return *Repriced; }

        const auto Euclidean = FVector::Dist(Get_NodeLocation(InFrom), Get_NodeLocation(InTo));
        return static_cast<float>(Euclidean) * _OffPathCostMultiplier;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    ExtractLegSpans(
        const FRouteGraph& InGraph,
        const FBuiltNetwork& InNetwork,
        const TArray<FRouteNodeId>& InPath)
        -> TArray<FRouteLegSpan>
    {
        auto Spans = TArray<FRouteLegSpan>{};

        const auto ResolveNodeEndDist = [&](const FBuiltEdge& InEdge, int32 InNodeId, float InOtherDist) -> float
        {
            // A node may match both ends (self-loop edge): pick the end closer to the other span
            // endpoint so the span agrees with the min-cost interpretation the search priced.
            const auto MatchesA = InEdge._NodeA == InNodeId;
            const auto MatchesB = InEdge._NodeB == InNodeId;

            if (MatchesA && MatchesB)
            { return InOtherDist < InEdge._Length - InOtherDist ? 0.0f : InEdge._Length; }

            return MatchesA ? 0.0f : InEdge._Length;
        };

        for (auto Index = 0; Index < InPath.Num() - 1; ++Index)
        {
            const auto& From = InPath[Index];
            const auto& To = InPath[Index + 1];

            auto Span = FRouteLegSpan{};
            Span._FromId = From;
            Span._ToId = To;
            Span._FromLocation = InGraph.Get_NodeLocation(From);
            Span._ToLocation = InGraph.Get_NodeLocation(To);

            if (InGraph.Get_IsOffPathHop(From, To))
            {
                Span._IsOffPath = true;
                Spans.Add(Span);
                continue;
            }

            const auto& Overlay = InGraph.Get_Shared()->_OverlayPoints;

            if (From._Kind == ERouteNodeKind::OverlayPoint && To._Kind == ERouteNodeKind::OverlayPoint)
            {
                Span._EdgeId = Overlay[From._Index]._EdgeId;
                Span._FromDist = Overlay[From._Index]._DistAlong;
                Span._ToDist = Overlay[To._Index]._DistAlong;
            }
            else if (From._Kind == ERouteNodeKind::OverlayPoint && To._Kind == ERouteNodeKind::NetNode)
            {
                const auto& Point = Overlay[From._Index];
                const auto& Edge = InNetwork._Edges[Point._EdgeId];
                Span._EdgeId = Point._EdgeId;
                Span._FromDist = Point._DistAlong;
                Span._ToDist = ResolveNodeEndDist(Edge, To._Index, Point._DistAlong);
            }
            else if (From._Kind == ERouteNodeKind::NetNode && To._Kind == ERouteNodeKind::OverlayPoint)
            {
                const auto& Point = Overlay[To._Index];
                const auto& Edge = InNetwork._Edges[Point._EdgeId];
                Span._EdgeId = Point._EdgeId;
                Span._FromDist = ResolveNodeEndDist(Edge, From._Index, Point._DistAlong);
                Span._ToDist = Point._DistAlong;
            }
            else if (From._Kind == ERouteNodeKind::NetNode && To._Kind == ERouteNodeKind::NetNode)
            {
                auto BestEdgeId = int32{INDEX_NONE};
                auto BestLength = TNumericLimits<float>::Max();

                for (const auto EdgeId : InNetwork._Nodes[From._Index]._EdgeIds)
                {
                    const auto& Edge = InNetwork._Edges[EdgeId];
                    const auto OtherNode = Edge._NodeA == From._Index ? Edge._NodeB : Edge._NodeA;

                    if (OtherNode == To._Index && Edge._Length < BestLength)
                    {
                        BestLength = Edge._Length;
                        BestEdgeId = EdgeId;
                    }
                }

                CK_ENSURE_IF_NOT(BestEdgeId != INDEX_NONE,
                    TEXT("ExtractLegSpans: no edge connects node [{}] to node [{}] on a solved route"),
                    From._Index, To._Index)
                { continue; }

                const auto& Edge = InNetwork._Edges[BestEdgeId];
                Span._EdgeId = BestEdgeId;
                Span._FromDist = Edge._NodeA == From._Index ? 0.0f : Edge._Length;
                Span._ToDist = Edge._NodeA == From._Index ? Edge._Length : 0.0f;
            }

            Spans.Add(Span);
        }

        return Spans;
    }
}

// --------------------------------------------------------------------------------------------------------------------
