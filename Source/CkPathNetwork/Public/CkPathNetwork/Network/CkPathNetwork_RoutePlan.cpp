#include "CkPathNetwork_RoutePlan.h"

#include "CkPathNetwork/CkPathNetwork_Stats.h"
#include "CkPathNetwork/Settings/CkPathNetwork_ProjectSettings.h"

#include "CkAStar/Algorithm/CkAStar_Search.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(
    TEXT("PathNetwork::StaticAugmentation"),
    STAT_CkPathNetwork_StaticAugmentation,
    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(
    TEXT("PathNetwork::EndpointOverlay"),
    STAT_CkPathNetwork_EndpointOverlay,
    STATGROUP_CkPathNetwork);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_routeplan
{
    using namespace ck::pathnetwork;

    constexpr auto MaxTransfersPerComponentPairPerCell = 8;
    constexpr auto MaxLocalNetworkShortcutCandidatePairs = 8192;
    constexpr auto MaxLocalNetworkShortcutCandidateSources = 512;
    constexpr auto MaxLocalNetworkShortcutsPerNode = 8;
    constexpr auto UnreachableAuthoredNetworkCost =
        TNumericLimits<float>::Max() / 4.0f;

    struct FAuthoredNetworkDistanceEntry
    {
        int32 _NodeId = INDEX_NONE;
        float _Distance = UnreachableAuthoredNetworkCost;

        auto
        operator<(
            const FAuthoredNetworkDistanceEntry& InOther) const
            -> bool
        {
            return _Distance < InOther._Distance;
        }
    };

    auto
    Get_AuthoredNetworkDistancesWithinCost(
        const FBuiltNetwork& InNetwork,
        const int32 InSourceNode,
        const float InMaxCost)
        -> TArray<float>
    {
        auto Distances = TArray<float>{};
        Distances.Init(
            UnreachableAuthoredNetworkCost,
            InNetwork._Nodes.Num());
        if (NOT InNetwork._Nodes.IsValidIndex(InSourceNode)
            || NOT FMath::IsFinite(InMaxCost)
            || InMaxCost <= 0.0f)
        { return Distances; }

        auto OpenSet = TArray<FAuthoredNetworkDistanceEntry>{};
        Distances[InSourceNode] = 0.0f;
        OpenSet.HeapPush(
            FAuthoredNetworkDistanceEntry{
                InSourceNode,
                0.0f},
            TLess<>{});

        while (NOT OpenSet.IsEmpty())
        {
            auto Current = FAuthoredNetworkDistanceEntry{};
            OpenSet.HeapPop(Current, TLess<>{});
            if (NOT InNetwork._Nodes.IsValidIndex(Current._NodeId)
                || Current._Distance
                    > Distances[Current._NodeId] + KINDA_SMALL_NUMBER)
            { continue; }
            if (Current._Distance > InMaxCost)
            { break; }

            for (const auto EdgeId :
                 InNetwork._Nodes[Current._NodeId]._EdgeIds)
            {
                if (NOT InNetwork._Edges.IsValidIndex(EdgeId))
                { continue; }

                const auto& Edge = InNetwork._Edges[EdgeId];
                int32 OtherNode = INDEX_NONE;
                if (Edge._NodeA == Current._NodeId)
                { OtherNode = Edge._NodeB; }
                else if (Edge._NodeB == Current._NodeId)
                { OtherNode = Edge._NodeA; }

                if (NOT InNetwork._Nodes.IsValidIndex(OtherNode)
                    || OtherNode == Current._NodeId
                    || NOT FMath::IsFinite(Edge._Length)
                    || Edge._Length <= KINDA_SMALL_NUMBER)
                { continue; }

                const auto CandidateDistance =
                    Current._Distance + Edge._Length;
                if (NOT FMath::IsFinite(CandidateDistance)
                    || CandidateDistance > InMaxCost
                    || CandidateDistance
                        >= Distances[OtherNode])
                { continue; }

                Distances[OtherNode] = CandidateDistance;
                OpenSet.HeapPush(
                    FAuthoredNetworkDistanceEntry{
                        OtherNode,
                        CandidateDistance},
                    TLess<>{});
            }
        }
        return Distances;
    }

    auto
    Is_PolicyValid(
        const FRouteCostPolicy& InCostPolicy) -> bool
    {
        return FMath::IsFinite(InCostPolicy._FarOrDirectCostMultiplier)
            && InCostPolicy._FarOrDirectCostMultiplier >= 1.0f
            && FMath::IsFinite(InCostPolicy._NearEndpointCostMultiplier)
            && InCostPolicy._NearEndpointCostMultiplier >= 1.0f
            && FMath::IsFinite(InCostPolicy._NetworkGapCostMultiplier)
            && InCostPolicy._NetworkGapCostMultiplier >= 1.0f
            && FMath::IsFinite(InCostPolicy._EndpointJoinMaxDistance)
            && InCostPolicy._EndpointJoinMaxDistance >= 0.0f
            && FMath::IsFinite(InCostPolicy._ComponentTransferMaxDistance)
            && InCostPolicy._ComponentTransferMaxDistance >= 0.0f
            && FMath::IsFinite(
                InCostPolicy._LocalNetworkShortcutMaxDistance)
            && InCostPolicy._LocalNetworkShortcutMaxDistance >= 0.0f
            && FMath::IsFinite(InCostPolicy._DirectTripGraceDistance)
            && InCostPolicy._DirectTripGraceDistance >= 0.0f
            && FMath::IsFinite(
                InCostPolicy._DirectRouteMinimumSavingsFraction)
            && InCostPolicy._DirectRouteMinimumSavingsFraction >= 0.0f
            && InCostPolicy._DirectRouteMinimumSavingsFraction <= 1.0f;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Build_LocalNetworkShortcuts(
        FRouteGraphStaticData& InOutShared,
        const FBuiltNetwork& InNetwork,
        const FRouteCostPolicy& InCostPolicy) -> void
    {
        const auto MaxDistance =
            InCostPolicy._LocalNetworkShortcutMaxDistance;
        if (MaxDistance <= 0.0f)
        { return; }

        const auto Topology = Analyze_NetworkTopology(InNetwork);
        if (Topology._ComponentCount <= 0)
        { return; }

        const auto AreAuthoredNeighbors =
            [&InNetwork](const int32 InNodeA, const int32 InNodeB)
            {
                for (const auto EdgeId : InNetwork._Nodes[InNodeA]._EdgeIds)
                {
                    if (NOT InNetwork._Edges.IsValidIndex(EdgeId))
                    { continue; }

                    const auto& Edge = InNetwork._Edges[EdgeId];
                    int32 OtherNode = INDEX_NONE;
                    if (Edge._NodeA == InNodeA)
                    { OtherNode = Edge._NodeB; }
                    else if (Edge._NodeB == InNodeA)
                    { OtherNode = Edge._NodeA; }
                    else
                    { continue; }
                    if (OtherNode == InNodeB)
                    { return true; }
                }
                return false;
            };

        struct FShortcutCandidate
        {
            int32 _NodeA = INDEX_NONE;
            int32 _NodeB = INDEX_NONE;
            float _Distance = 0.0f;
            float _BenefitScore = 0.0f;
        };

        // Query-local spatial bucketing keeps candidate generation bounded by
        // geographic density rather than the square of the entire network.
        const auto CellSize = FMath::Max(MaxDistance, 1.0f);
        const auto GetCell =
            [CellSize](const FVector& InLocation)
            {
                return FIntVector{
                    FMath::FloorToInt32(InLocation.X / CellSize),
                    FMath::FloorToInt32(InLocation.Y / CellSize),
                    FMath::FloorToInt32(InLocation.Z / CellSize)};
            };

        auto NodesByCell = TMap<FIntVector, TArray<int32>>{};
        for (auto NodeId = 0; NodeId < InNetwork._Nodes.Num(); ++NodeId)
        {
            if (Topology._LogicalDegreeByNode[NodeId] <= 0)
            { continue; }

            NodesByCell
                .FindOrAdd(GetCell(InNetwork._Nodes[NodeId]._Location))
                .Add(NodeId);
        }

        auto Candidates = TArray<FShortcutCandidate>{};
        auto CandidateSourceCount = 0;
        for (auto NodeA = 0; NodeA < InNetwork._Nodes.Num(); ++NodeA)
        {
            if (Topology._LogicalDegreeByNode[NodeA] <= 0)
            { continue; }

            auto NodeHasCandidate = false;
            const auto Cell = GetCell(
                InNetwork._Nodes[NodeA]._Location);
            for (auto CellZ = Cell.Z - 1; CellZ <= Cell.Z + 1; ++CellZ)
            {
                for (auto CellY = Cell.Y - 1; CellY <= Cell.Y + 1; ++CellY)
                {
                    for (auto CellX = Cell.X - 1;
                         CellX <= Cell.X + 1;
                         ++CellX)
                    {
                        const auto* NearbyNodes = NodesByCell.Find(
                            FIntVector{CellX, CellY, CellZ});
                        if (NearbyNodes == nullptr)
                        { continue; }

                        for (const auto NodeB : *NearbyNodes)
                        {
                            if (NodeB <= NodeA
                                || Topology._ComponentByNode[NodeA]
                                    != Topology._ComponentByNode[NodeB]
                                || AreAuthoredNeighbors(NodeA, NodeB))
                            { continue; }

                            const auto Distance = static_cast<float>(
                                FVector::Dist(
                                    InNetwork._Nodes[NodeA]._Location,
                                    InNetwork._Nodes[NodeB]._Location));
                            if (Distance <= KINDA_SMALL_NUMBER
                                || Distance > MaxDistance)
                            { continue; }

                            if (NOT NodeHasCandidate)
                            {
                                NodeHasCandidate = true;
                                ++CandidateSourceCount;
                                if (CandidateSourceCount
                                    > MaxLocalNetworkShortcutCandidateSources)
                                {
                                    InOutShared
                                        ._LocalNetworkShortcutCandidateCount =
                                            Candidates.Num();
                                    InOutShared
                                        ._LocalNetworkShortcutCandidateSourceCount =
                                            CandidateSourceCount;
                                    InOutShared
                                        ._LocalNetworkShortcutBudgetExceeded =
                                            true;
                                    return;
                                }
                            }
                            if (Candidates.Num()
                                >= MaxLocalNetworkShortcutCandidatePairs)
                            {
                                InOutShared
                                    ._LocalNetworkShortcutCandidateCount =
                                        MaxLocalNetworkShortcutCandidatePairs
                                        + 1;
                                InOutShared
                                    ._LocalNetworkShortcutCandidateSourceCount =
                                        CandidateSourceCount;
                                InOutShared
                                    ._LocalNetworkShortcutBudgetExceeded =
                                        true;
                                return;
                            }

                            Candidates.Add(
                                FShortcutCandidate{
                                    NodeA,
                                    NodeB,
                                    Distance});
                        }
                    }
                }
            }
        }

        const auto MaxAuthoredCostToInspect =
            MaxDistance
            * InCostPolicy._NetworkGapCostMultiplier;
        if (NOT FMath::IsFinite(MaxAuthoredCostToInspect)
            || MaxAuthoredCostToInspect <= 0.0f)
        { return; }

        InOutShared._LocalNetworkShortcutCandidateCount =
            Candidates.Num();
        InOutShared._LocalNetworkShortcutCandidateSourceCount =
            CandidateSourceCount;

        auto BeneficialCandidates = TArray<FShortcutCandidate>{};
        BeneficialCandidates.Reserve(Candidates.Num());
        int32 CurrentSourceNode = INDEX_NONE;
        auto AuthoredDistances = TArray<float>{};
        for (auto Candidate : Candidates)
        {
            if (Candidate._NodeA != CurrentSourceNode)
            {
                AuthoredDistances =
                    Get_AuthoredNetworkDistancesWithinCost(
                        InNetwork,
                        Candidate._NodeA,
                        MaxAuthoredCostToInspect);
                CurrentSourceNode = Candidate._NodeA;
            }
            if (NOT AuthoredDistances.IsValidIndex(
                    Candidate._NodeB))
            { continue; }

            const auto OffPathCostEstimate =
                Candidate._Distance
                * InCostPolicy._NetworkGapCostMultiplier;
            if (NOT FMath::IsFinite(OffPathCostEstimate)
                || AuthoredDistances[Candidate._NodeB]
                    <= OffPathCostEstimate + KINDA_SMALL_NUMBER)
            { continue; }

            Candidate._BenefitScore =
                FMath::Min(
                    AuthoredDistances[Candidate._NodeB],
                    MaxAuthoredCostToInspect)
                - OffPathCostEstimate;
            BeneficialCandidates.Add(Candidate);
        }

        BeneficialCandidates.Sort(
            [](const FShortcutCandidate& InA,
               const FShortcutCandidate& InB)
            {
                if (InA._BenefitScore != InB._BenefitScore)
                { return InA._BenefitScore > InB._BenefitScore; }
                if (InA._Distance != InB._Distance)
                { return InA._Distance < InB._Distance; }
                if (InA._NodeA != InB._NodeA)
                { return InA._NodeA < InB._NodeA; }
                return InA._NodeB < InB._NodeB;
            });

        auto ShortcutCountByNode = TArray<int32>{};
        ShortcutCountByNode.Init(0, InNetwork._Nodes.Num());
        for (const auto& Candidate : BeneficialCandidates)
        {
            if (ShortcutCountByNode[Candidate._NodeA]
                    >= MaxLocalNetworkShortcutsPerNode
                || ShortcutCountByNode[Candidate._NodeB]
                    >= MaxLocalNetworkShortcutsPerNode)
            { continue; }

            InOutShared._LocalNetworkShortcutsByNode
                .FindOrAdd(Candidate._NodeA)
                .Add(Candidate._NodeB);
            InOutShared._LocalNetworkShortcutsByNode
                .FindOrAdd(Candidate._NodeB)
                .Add(Candidate._NodeA);
            ++ShortcutCountByNode[Candidate._NodeA];
            ++ShortcutCountByNode[Candidate._NodeB];
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    FindOrAdd_OverlayPoint(
        FRouteGraphStaticData& InOutShared,
        const FRouteOverlayPoint& InCandidate) -> int32;

    auto
    Build_ComponentTransfers(
        FRouteGraphStaticData& InOutShared,
        const FBuiltNetwork& InNetwork,
        const FRouteCostPolicy& InCostPolicy) -> void
    {
        const auto MaxDistance =
            InCostPolicy._ComponentTransferMaxDistance;
        if (MaxDistance <= 0.0f)
        { return; }

        const auto Topology = Analyze_NetworkTopology(InNetwork);
        if (Topology._ComponentCount <= 1)
        { return; }

        struct FTransferEndpoint
        {
            int32 _NodeId = INDEX_NONE;
            int32 _EdgeId = INDEX_NONE;
            float _DistAlong = 0.0f;
            FVector _Location = FVector::ZeroVector;
        };

        struct FTransferCandidate
        {
            FTransferEndpoint _EndpointA;
            FTransferEndpoint _EndpointB;
            int32 _ComponentA = INDEX_NONE;
            int32 _ComponentB = INDEX_NONE;
            float _Distance = 0.0f;
            bool _UsesEdgeInterior = false;
        };

        struct FCanonicalTransferCandidate
        {
            FTransferCandidate _Candidate;
            FRouteNodeId _EndpointA;
            FRouteNodeId _EndpointB;
        };

        // Keep pathological but valid sub-centimeter tuning total: the actual
        // distance predicate still uses MaxDistance, while the lookup grid never
        // divides world coordinates by a near-zero value.
        const auto CellSize = FMath::Max(MaxDistance, 1.0f);
        const auto GetCell =
            [CellSize](const FVector& InLocation)
            {
                return FIntVector{
                    FMath::FloorToInt32(InLocation.X / CellSize),
                    FMath::FloorToInt32(InLocation.Y / CellSize),
                    FMath::FloorToInt32(InLocation.Z / CellSize)};
            };

        auto NodesByCell = TMap<FIntVector, TArray<int32>>{};
        for (auto NodeId = 0; NodeId < InNetwork._Nodes.Num(); ++NodeId)
        {
            if (Topology._LogicalDegreeByNode[NodeId] <= 0)
            { continue; }

            NodesByCell
                .FindOrAdd(GetCell(InNetwork._Nodes[NodeId]._Location))
                .Add(NodeId);
        }

        auto Candidates = TArray<FTransferCandidate>{};
        for (auto NodeA = 0; NodeA < InNetwork._Nodes.Num(); ++NodeA)
        {
            if (Topology._LogicalDegreeByNode[NodeA] <= 0)
            { continue; }

            const auto Cell = GetCell(
                InNetwork._Nodes[NodeA]._Location);
            for (auto CellZ = Cell.Z - 1; CellZ <= Cell.Z + 1; ++CellZ)
            {
                for (auto CellY = Cell.Y - 1; CellY <= Cell.Y + 1; ++CellY)
                {
                    for (auto CellX = Cell.X - 1;
                         CellX <= Cell.X + 1;
                         ++CellX)
                    {
                        const auto* NearbyNodes = NodesByCell.Find(
                            FIntVector{CellX, CellY, CellZ});
                        if (NearbyNodes == nullptr)
                        { continue; }

                        for (const auto NodeB : *NearbyNodes)
                        {
                            if (NodeB <= NodeA
                                || Topology._ComponentByNode[NodeA]
                                    == Topology._ComponentByNode[NodeB])
                            { continue; }

                            const auto Distance = static_cast<float>(
                                FVector::Dist(
                                    InNetwork._Nodes[NodeA]._Location,
                                    InNetwork._Nodes[NodeB]._Location));
                            if (Distance > MaxDistance)
                            { continue; }

                            Candidates.Add(
                                FTransferCandidate{
                                    FTransferEndpoint{
                                        NodeA,
                                        INDEX_NONE,
                                        0.0f,
                                        InNetwork._Nodes[NodeA]._Location},
                                    FTransferEndpoint{
                                        NodeB,
                                        INDEX_NONE,
                                        0.0f,
                                        InNetwork._Nodes[NodeB]._Location},
                                    Topology._ComponentByNode[NodeA],
                                    Topology._ComponentByNode[NodeB],
                                    Distance,
                                    false});
                        }
                    }
                }
            }
        }

        const auto GetEdgeComponent =
            [&](const int32 InEdgeId) -> int32
            {
                if (NOT InNetwork._Edges.IsValidIndex(InEdgeId))
                { return INDEX_NONE; }

                const auto& Edge = InNetwork._Edges[InEdgeId];
                if (NOT InNetwork._Nodes.IsValidIndex(Edge._NodeA)
                    || NOT InNetwork._Nodes.IsValidIndex(Edge._NodeB)
                    || NOT Topology._ComponentByNode.IsValidIndex(Edge._NodeA)
                    || NOT Topology._ComponentByNode.IsValidIndex(Edge._NodeB))
                { return INDEX_NONE; }

                const auto ComponentA =
                    Topology._ComponentByNode[Edge._NodeA];
                const auto ComponentB =
                    Topology._ComponentByNode[Edge._NodeB];
                return ComponentA >= 0 && ComponentA == ComponentB
                    ? ComponentA
                    : INDEX_NONE;
            };

        // The authored graph nodes usually sit at ribbon ends and intersections,
        // while road crossings frequently face the interior of long ribbon
        // edges. Use the built polyline chunk index to find local cross-component
        // edge pairs, then admit their exact closest points when no endpoint pair
        // can represent the same configured gap.
        auto EdgePairKeys = TSet<uint64>{};
        for (auto EdgeAId = 0;
             EdgeAId < InNetwork._Edges.Num();
             ++EdgeAId)
        {
            if (GetEdgeComponent(EdgeAId) == INDEX_NONE)
            { continue; }

            const auto& EdgeA = InNetwork._Edges[EdgeAId];
            for (auto SegmentA = 0;
                 SegmentA < EdgeA._Points.Num() - 1;
                 ++SegmentA)
            {
                const auto& SegmentStart =
                    EdgeA._Points[SegmentA];
                const auto& SegmentEnd =
                    EdgeA._Points[SegmentA + 1];
                const auto SegmentMidpoint =
                    (SegmentStart + SegmentEnd) * 0.5;
                const auto SearchRadius = static_cast<float>(
                    FVector::Dist(SegmentStart, SegmentEnd) * 0.5
                    + MaxDistance);
                for (const auto EdgeBId :
                     InNetwork.Query_EdgesNear(
                         SegmentMidpoint,
                         SearchRadius))
                {
                    if (EdgeBId <= EdgeAId)
                    { continue; }

                    const auto ComponentB =
                        GetEdgeComponent(EdgeBId);
                    if (ComponentB == INDEX_NONE
                        || ComponentB == GetEdgeComponent(EdgeAId))
                    { continue; }

                    EdgePairKeys.Add(
                        (static_cast<uint64>(
                            static_cast<uint32>(EdgeAId)) << 32)
                        | static_cast<uint64>(
                            static_cast<uint32>(EdgeBId)));
                }
            }
        }

        auto SortedEdgePairKeys = EdgePairKeys.Array();
        SortedEdgePairKeys.Sort();
        for (const auto EdgePairKey : SortedEdgePairKeys)
        {
            const auto EdgeAId = static_cast<int32>(
                static_cast<uint32>(EdgePairKey >> 32));
            const auto EdgeBId = static_cast<int32>(
                static_cast<uint32>(EdgePairKey));
            const auto ComponentA = GetEdgeComponent(EdgeAId);
            const auto ComponentB = GetEdgeComponent(EdgeBId);
            if (ComponentA == INDEX_NONE
                || ComponentB == INDEX_NONE
                || ComponentA == ComponentB)
            { continue; }

            const auto& EdgeA = InNetwork._Edges[EdgeAId];
            const auto& EdgeB = InNetwork._Edges[EdgeBId];
            if (EdgeA._Points.Num() < 2
                || EdgeB._Points.Num() < 2
                || EdgeA._CumulativeLengths.Num()
                    != EdgeA._Points.Num()
                || EdgeB._CumulativeLengths.Num()
                    != EdgeB._Points.Num())
            { continue; }

            auto ClosestA = FVector::ZeroVector;
            auto ClosestB = FVector::ZeroVector;
            auto ClosestDistanceSquared =
                TNumericLimits<double>::Max();
            auto ClosestDistAlongA = 0.0f;
            auto ClosestDistAlongB = 0.0f;
            for (auto SegmentA = 0;
                 SegmentA < EdgeA._Points.Num() - 1;
                 ++SegmentA)
            {
                for (auto SegmentB = 0;
                     SegmentB < EdgeB._Points.Num() - 1;
                     ++SegmentB)
                {
                    auto CandidateA = FVector::ZeroVector;
                    auto CandidateB = FVector::ZeroVector;
                    FMath::SegmentDistToSegmentSafe(
                        EdgeA._Points[SegmentA],
                        EdgeA._Points[SegmentA + 1],
                        EdgeB._Points[SegmentB],
                        EdgeB._Points[SegmentB + 1],
                        CandidateA,
                        CandidateB);
                    const auto CandidateDistanceSquared =
                        FVector::DistSquared(
                            CandidateA,
                            CandidateB);
                    if (CandidateDistanceSquared
                        >= ClosestDistanceSquared)
                    { continue; }

                    ClosestDistanceSquared =
                        CandidateDistanceSquared;
                    ClosestA = CandidateA;
                    ClosestB = CandidateB;
                    ClosestDistAlongA =
                        EdgeA._CumulativeLengths[SegmentA]
                        + static_cast<float>(FVector::Dist(
                            EdgeA._Points[SegmentA],
                            CandidateA));
                    ClosestDistAlongB =
                        EdgeB._CumulativeLengths[SegmentB]
                        + static_cast<float>(FVector::Dist(
                            EdgeB._Points[SegmentB],
                            CandidateB));
                }
            }

            const auto ClosestDistance = static_cast<float>(
                FMath::Sqrt(ClosestDistanceSquared));
            if (ClosestDistance > MaxDistance)
            { continue; }

            const auto& NodeA0 =
                InNetwork._Nodes[EdgeA._NodeA]._Location;
            const auto& NodeA1 =
                InNetwork._Nodes[EdgeA._NodeB]._Location;
            const auto& NodeB0 =
                InNetwork._Nodes[EdgeB._NodeA]._Location;
            const auto& NodeB1 =
                InNetwork._Nodes[EdgeB._NodeB]._Location;
            const auto NearestEndpointPairDistance =
                static_cast<float>(FMath::Min(
                    FMath::Min(
                        FVector::Dist(NodeA0, NodeB0),
                        FVector::Dist(NodeA0, NodeB1)),
                    FMath::Min(
                        FVector::Dist(NodeA1, NodeB0),
                        FVector::Dist(NodeA1, NodeB1))));
            if (NearestEndpointPairDistance <= MaxDistance)
            {
                // A node-pair candidate already represents this local edge pair.
                continue;
            }

            Candidates.Add(
                FTransferCandidate{
                    FTransferEndpoint{
                        INDEX_NONE,
                        EdgeAId,
                        ClosestDistAlongA,
                        ClosestA},
                    FTransferEndpoint{
                        INDEX_NONE,
                        EdgeBId,
                        ClosestDistAlongB,
                        ClosestB},
                    ComponentA,
                    ComponentB,
                    ClosestDistance,
                    true});
        }

        Candidates.Sort(
            [](const FTransferCandidate& InA,
               const FTransferCandidate& InB)
            {
                if (InA._Distance != InB._Distance)
                { return InA._Distance < InB._Distance; }
                if (InA._ComponentA != InB._ComponentA)
                { return InA._ComponentA < InB._ComponentA; }
                if (InA._ComponentB != InB._ComponentB)
                { return InA._ComponentB < InB._ComponentB; }
                if (InA._EndpointA._NodeId
                    != InB._EndpointA._NodeId)
                {
                    return InA._EndpointA._NodeId
                        < InB._EndpointA._NodeId;
                }
                if (InA._EndpointA._EdgeId
                    != InB._EndpointA._EdgeId)
                {
                    return InA._EndpointA._EdgeId
                        < InB._EndpointA._EdgeId;
                }
                if (InA._EndpointA._DistAlong
                    != InB._EndpointA._DistAlong)
                {
                    return InA._EndpointA._DistAlong
                        < InB._EndpointA._DistAlong;
                }
                if (InA._EndpointB._NodeId
                    != InB._EndpointB._NodeId)
                {
                    return InA._EndpointB._NodeId
                        < InB._EndpointB._NodeId;
                }
                if (InA._EndpointB._EdgeId
                    != InB._EndpointB._EdgeId)
                {
                    return InA._EndpointB._EdgeId
                        < InB._EndpointB._EdgeId;
                }
                return InA._EndpointB._DistAlong
                    < InB._EndpointB._DistAlong;
            });
        // Canonicalize candidate endpoints before charging the geographic cap.
        // Multiple raw edge pairs can resolve to the same route-node jump.
        constexpr auto EndpointDedupeDistance = 1.0f;
        auto CanonicalOverlayPoints =
            TArray<FRouteOverlayPoint>{};
        auto CanonicalOverlayPointsByEdge =
            TMap<int32, TArray<int32>>{};
        const auto CanonicalizeEndpoint =
            [&](const FTransferEndpoint& InEndpoint)
                -> FRouteNodeId
            {
                if (InNetwork._Nodes.IsValidIndex(
                        InEndpoint._NodeId))
                {
                    return FRouteNodeId{
                        ERouteNodeKind::NetNode,
                        InEndpoint._NodeId};
                }

                if (NOT InNetwork._Edges.IsValidIndex(
                        InEndpoint._EdgeId))
                {
                    return FRouteNodeId{
                        ERouteNodeKind::NetNode,
                        INDEX_NONE};
                }

                const auto& Edge =
                    InNetwork._Edges[InEndpoint._EdgeId];
                if (InEndpoint._DistAlong
                    <= EndpointDedupeDistance)
                {
                    return FRouteNodeId{
                        ERouteNodeKind::NetNode,
                        Edge._NodeA};
                }
                if (Edge._Length - InEndpoint._DistAlong
                    <= EndpointDedupeDistance)
                {
                    return FRouteNodeId{
                        ERouteNodeKind::NetNode,
                        Edge._NodeB};
                }

                if (const auto* ExistingOnEdge =
                        CanonicalOverlayPointsByEdge.Find(
                            InEndpoint._EdgeId))
                {
                    for (const auto ExistingIndex : *ExistingOnEdge)
                    {
                        if (FMath::Abs(
                                CanonicalOverlayPoints[ExistingIndex]
                                    ._DistAlong
                                - InEndpoint._DistAlong)
                            < EndpointDedupeDistance)
                        {
                            return FRouteNodeId{
                                ERouteNodeKind::OverlayPoint,
                                ExistingIndex};
                        }
                    }
                }

                const auto OverlayIndex =
                    CanonicalOverlayPoints.Add(
                        FRouteOverlayPoint{
                            InEndpoint._EdgeId,
                            InEndpoint._DistAlong,
                            InEndpoint._Location});
                CanonicalOverlayPointsByEdge
                    .FindOrAdd(InEndpoint._EdgeId)
                    .Add(OverlayIndex);
                return FRouteNodeId{
                    ERouteNodeKind::OverlayPoint,
                    OverlayIndex};
            };

        auto CanonicalCandidates =
            TArray<FCanonicalTransferCandidate>{};
        auto CanonicalPairKeys = TSet<uint64>{};
        auto EdgeInteriorCandidateCount = 0;
        for (const auto& Candidate : Candidates)
        {
            const auto EndpointA =
                CanonicalizeEndpoint(Candidate._EndpointA);
            const auto EndpointB =
                CanonicalizeEndpoint(Candidate._EndpointB);
            if (EndpointA._Index == INDEX_NONE
                || EndpointB._Index == INDEX_NONE
                || EndpointA == EndpointB)
            { continue; }

            const auto ForwardPairKey =
                FRouteGraph::PackOffPathKey(
                    EndpointA,
                    EndpointB);
            const auto ReversePairKey =
                FRouteGraph::PackOffPathKey(
                    EndpointB,
                    EndpointA);
            const auto CanonicalPairKey =
                ForwardPairKey < ReversePairKey
                    ? ForwardPairKey
                    : ReversePairKey;
            if (CanonicalPairKeys.Contains(CanonicalPairKey))
            { continue; }

            CanonicalPairKeys.Add(CanonicalPairKey);
            CanonicalCandidates.Add(
                FCanonicalTransferCandidate{
                    Candidate,
                    EndpointA,
                    EndpointB});
            if (Candidate._UsesEdgeInterior)
            { ++EdgeInteriorCandidateCount; }
        }
        InOutShared._ComponentTransferCandidateCount =
            CanonicalCandidates.Num();
        InOutShared._ComponentTransferEdgeInteriorCandidateCount =
            EdgeInteriorCandidateCount;

        const auto MaterializeEndpoint =
            [&](const FRouteNodeId& InCanonicalEndpoint)
                -> FRouteNodeId
            {
                if (InCanonicalEndpoint._Kind
                    == ERouteNodeKind::NetNode)
                { return InCanonicalEndpoint; }

                if (InCanonicalEndpoint._Kind
                        != ERouteNodeKind::OverlayPoint
                    || NOT CanonicalOverlayPoints.IsValidIndex(
                        InCanonicalEndpoint._Index))
                {
                    return FRouteNodeId{
                        ERouteNodeKind::NetNode,
                        INDEX_NONE};
                }

                return FRouteNodeId{
                    ERouteNodeKind::OverlayPoint,
                    FindOrAdd_OverlayPoint(
                        InOutShared,
                        CanonicalOverlayPoints[
                            InCanonicalEndpoint._Index])};
            };

        auto TransferCountsByCell =
            TMap<FIntVector, TMap<uint64, int32>>{};
        for (const auto& CanonicalCandidate :
             CanonicalCandidates)
        {
            const auto& Candidate =
                CanonicalCandidate._Candidate;
            const auto ComponentLow = FMath::Min(
                Candidate._ComponentA,
                Candidate._ComponentB);
            const auto ComponentHigh = FMath::Max(
                Candidate._ComponentA,
                Candidate._ComponentB);
            const auto ComponentPairKey =
                (static_cast<uint64>(
                    static_cast<uint32>(ComponentLow)) << 32)
                | static_cast<uint64>(
                    static_cast<uint32>(ComponentHigh));
            const auto TransferCell = GetCell(
                (Candidate._EndpointA._Location
                    + Candidate._EndpointB._Location)
                * 0.5);
            auto& TransferCountsByComponentPair =
                TransferCountsByCell.FindOrAdd(TransferCell);
            auto& PairTransferCount =
                TransferCountsByComponentPair.FindOrAdd(
                    ComponentPairKey);
            if (PairTransferCount
                >= MaxTransfersPerComponentPairPerCell)
            {
                ++InOutShared._ComponentTransferRejectedByCellCapCount;
                continue;
            }

            const auto EndpointA =
                MaterializeEndpoint(
                    CanonicalCandidate._EndpointA);
            const auto EndpointB =
                MaterializeEndpoint(
                    CanonicalCandidate._EndpointB);
            if (EndpointA._Index == INDEX_NONE
                || EndpointB._Index == INDEX_NONE)
            { continue; }

            InOutShared._ComponentTransfersByRouteNode
                .FindOrAdd(EndpointA)
                .AddUnique(EndpointB);
            InOutShared._ComponentTransfersByRouteNode
                .FindOrAdd(EndpointB)
                .AddUnique(EndpointA);
            if (EndpointA._Kind == ERouteNodeKind::NetNode
                && EndpointB._Kind == ERouteNodeKind::NetNode)
            {
                InOutShared._ComponentTransfersByNode
                    .FindOrAdd(EndpointA._Index)
                    .AddUnique(EndpointB._Index);
                InOutShared._ComponentTransfersByNode
                    .FindOrAdd(EndpointB._Index)
                    .AddUnique(EndpointA._Index);
            }
            ++PairTransferCount;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    FindOrAdd_OverlayPoint(
        FRouteGraphStaticData& InOutShared,
        const FRouteOverlayPoint& InCandidate) -> int32
    {
        constexpr auto DedupeDistAlong = 1.0f;

        if (const auto* ExistingOnEdge =
                InOutShared._OverlayPointsByEdge.Find(
                    InCandidate._EdgeId))
        {
            for (const auto ExistingIndex : *ExistingOnEdge)
            {
                if (FMath::Abs(
                        InOutShared
                            ._OverlayPoints[ExistingIndex]
                            ._DistAlong
                        - InCandidate._DistAlong)
                    < DedupeDistAlong)
                {
                    return ExistingIndex;
                }
            }
        }

        const auto NewIndex =
            InOutShared._OverlayPoints.Add(InCandidate);
        InOutShared._OverlayPointsByEdge
            .FindOrAdd(InCandidate._EdgeId)
            .Add(NewIndex);
        return NewIndex;
    }

    auto
    Merge_CandidatesIntoOverlay(
        FRouteGraphStaticData& InOutShared,
        const TArray<FRouteOverlayPoint>& InCandidates) -> void
    {
        for (const auto& Candidate : InCandidates)
        { FindOrAdd_OverlayPoint(InOutShared, Candidate); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Search_RouteGraphOnce(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy,
        const TSharedPtr<FRouteGraphSharedData>& InShared,
        const int32 InMaxIterations)
        -> FRoutePlanResult
    {
        auto Result = FRoutePlanResult{};
        Result._Shared = InShared;

        const auto Graph = FRouteGraph{
            &InNetwork,
            InStartLocation,
            InGoalLocation,
            InCostPolicy,
            InShared};
        const auto StartId = FRouteNodeId{ERouteNodeKind::Start, 0};
        const auto GoalId = FRouteNodeId{ERouteNodeKind::Goal, 0};
        auto Search =
            ck::astar::TSearchState<FRouteNodeId, FRouteGraph>{
                Graph,
                StartId,
                GoalId};

        auto SearchParams = ck::astar::FSearchParams{};
        SearchParams.MaxIterationsPerTick = InMaxIterations;
        const auto SearchStatus = Search.ContinueSearch(SearchParams);
        switch (SearchStatus)
        {
            case ck::astar::ESearchStatus::Complete:
                Result._SearchOutcome = ERouteSearchOutcome::Complete;
                break;
            case ck::astar::ESearchStatus::Failed:
                Result._SearchOutcome = ERouteSearchOutcome::Failed;
                break;
            case ck::astar::ESearchStatus::InProgress:
                Result._SearchOutcome = ERouteSearchOutcome::InProgress;
                break;
            case ck::astar::ESearchStatus::CostThresholdReached:
                Result._SearchOutcome = ERouteSearchOutcome::CostThresholdReached;
                break;
            default:
                Result._SearchOutcome = ERouteSearchOutcome::Failed;
                break;
        }
        if (SearchStatus != ck::astar::ESearchStatus::Complete)
        { return Result; }

        Result._Succeeded = true;
        Result._Path = Search.GetResultPath();
        Result._Spans = ExtractLegSpans(Graph, InNetwork, Result._Path);
        Result._EstimatedCost = Search.GetResultCost();
        return Result;
    }

    auto
    Is_ExactDirectPlan(
        const FRoutePlanResult& InPlan) -> bool
    {
        return InPlan._Succeeded
            && InPlan._Path.Num() == 2
            && InPlan._Path[0]._Kind == ERouteNodeKind::Start
            && InPlan._Path[1]._Kind == ERouteNodeKind::Goal
            && InPlan._Spans.Num() == 1
            && InPlan._Spans[0]._IsOffPath;
    }

    auto
    Is_DirectTripGraceApplied(
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy) -> bool
    {
        return InCostPolicy._DirectTripGraceDistance > 0.0f
            && FVector::Dist(InStartLocation, InGoalLocation)
                <= InCostPolicy._DirectTripGraceDistance;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    Uses_Network(
        const FRoutePlanResult& InPlan) -> bool
    {
        return InPlan._Succeeded
            && InPlan._Spans.ContainsByPredicate(
                [](const FRouteLegSpan& InSpan)
                {
                    return NOT InSpan._IsOffPath
                        || (InSpan._FromId._Kind == ERouteNodeKind::NetNode
                            && InSpan._ToId._Kind
                                == ERouteNodeKind::NetNode);
                });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Gather_RouteEndpointCandidates(
        const FBuiltNetwork& InNetwork,
        const FVector& InLocation,
        const FRouteCostPolicy& InCostPolicy) -> TArray<FRouteOverlayPoint>
    {
        const bool InputsAreValid =
            NOT InNetwork._Edges.IsEmpty()
            && FMath::IsFinite(InLocation.X)
            && FMath::IsFinite(InLocation.Y)
            && FMath::IsFinite(InLocation.Z)
            && ck_pathnetwork_routeplan::Is_PolicyValid(InCostPolicy);
        if (NOT InputsAreValid)
        { return {}; }

        const auto CandidateCount =
            UCk_Utils_PathNetwork_Settings_UE::Get_GoalCandidateCount();
        const auto MaxDoublings =
            UCk_Utils_PathNetwork_Settings_UE::Get_CandidateSearchMaxDoublings();
        const auto JoinMaxDistance = InCostPolicy._EndpointJoinMaxDistance;

        auto Radius =
            UCk_Utils_PathNetwork_Settings_UE::Get_CandidateSearchRadius();
        if (JoinMaxDistance > 0.0f)
        { Radius = FMath::Min(Radius, JoinMaxDistance); }

        auto EdgeIds = TArray<int32>{};
        for (auto Doubling = 0; Doubling <= MaxDoublings; ++Doubling)
        {
            EdgeIds = InNetwork.Query_EdgesNear(InLocation, Radius);

            if (EdgeIds.Num() >= CandidateCount)
            { break; }

            if (JoinMaxDistance > 0.0f && Radius >= JoinMaxDistance)
            { break; }

            Radius = JoinMaxDistance > 0.0f
                ? FMath::Min(Radius * 2.0f, JoinMaxDistance)
                : Radius * 2.0f;
        }

        struct FScoredCandidate
        {
            FRouteOverlayPoint _Point;
            float _Distance = 0.0f;
        };

        auto Scored = TArray<FScoredCandidate>{};
        Scored.Reserve(EdgeIds.Num());

        for (const auto EdgeId : EdgeIds)
        {
            const auto Projection = InNetwork.Project_OntoEdge(EdgeId, InLocation);
            if (NOT InCostPolicy.Get_IsEndpointJoinPermitted(Projection._Distance))
            { continue; }

            Scored.Add(FScoredCandidate{
                FRouteOverlayPoint{
                    EdgeId,
                    Projection._DistAlong,
                    Projection._Location},
                Projection._Distance});
        }

        Scored.Sort(
            [](const FScoredCandidate& InA, const FScoredCandidate& InB)
            {
                return InA._Distance < InB._Distance;
            });

        auto Result = TArray<FRouteOverlayPoint>{};
        for (auto Index = 0;
             Index < Scored.Num() && Index < CandidateCount;
             ++Index)
        {
            Result.Add(Scored[Index]._Point);
        }
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Resolve_RouteCostPolicy(
        const FCk_PathNetworkFollower_Tuning& InTuning)
        -> FRouteCostPolicy
    {
        const auto FarOrDirectMultiplier =
            InTuning.Get_OffPathCostMultiplier();
        const auto AuthoredNearMultiplier =
            InTuning.Get_NearEndpointCostMultiplier();
        const auto AuthoredNetworkGapMultiplier =
            InTuning.Get_NetworkGapCostMultiplier();

        auto Policy = FRouteCostPolicy{};
        Policy._FarOrDirectCostMultiplier = FarOrDirectMultiplier;
        Policy._NearEndpointCostMultiplier = AuthoredNearMultiplier > 0.0f
            ? AuthoredNearMultiplier
            : FarOrDirectMultiplier;
        Policy._NetworkGapCostMultiplier = AuthoredNetworkGapMultiplier > 0.0f
            ? AuthoredNetworkGapMultiplier
            : FarOrDirectMultiplier;
        Policy._EndpointJoinMaxDistance =
            InTuning.Get_EndpointJoinMaxDistance();
        Policy._ComponentTransferMaxDistance =
            InTuning.Get_ComponentTransferMaxDistance();
        Policy._LocalNetworkShortcutMaxDistance =
            InTuning.Get_LocalNetworkShortcutMaxDistance();
        Policy._DirectTripGraceDistance =
            InTuning.Get_DirectTripGraceDistance();
        Policy._DirectRouteMinimumSavingsFraction =
            InTuning.Get_DirectRouteMinimumSavingsFraction();
        return Policy;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Resolve_RouteCostPolicy(
        const FCk_Fragment_PathNetworkFollower_ParamsData& InParams)
        -> FRouteCostPolicy
    {
        auto Tuning = FCk_PathNetworkFollower_Tuning{};
        Tuning.Set_OffPathCostMultiplier(InParams.Get_OffPathCostMultiplier());
        Tuning.Set_NearEndpointCostMultiplier(
            InParams.Get_NearEndpointCostMultiplier());
        Tuning.Set_NetworkGapCostMultiplier(
            InParams.Get_NetworkGapCostMultiplier());
        Tuning.Set_EndpointJoinMaxDistance(
            InParams.Get_EndpointJoinMaxDistance());
        Tuning.Set_ComponentTransferMaxDistance(
            InParams.Get_ComponentTransferMaxDistance());
        Tuning.Set_LocalNetworkShortcutMaxDistance(
            InParams.Get_LocalNetworkShortcutMaxDistance());
        Tuning.Set_DirectTripGraceDistance(
            InParams.Get_DirectTripGraceDistance());
        Tuning.Set_DirectRouteMinimumSavingsFraction(
            InParams.Get_DirectRouteMinimumSavingsFraction());
        Tuning.Set_SideKeepingFraction(InParams.Get_SideKeepingFraction());
        Tuning.Set_CorridorWaypointSpacing(
            InParams.Get_CorridorWaypointSpacing());
        Tuning.Set_CornerSmoothingDistance(
            InParams.Get_CornerSmoothingDistance());
        Tuning.Set_DesiredNavmeshClearance(
            InParams.Get_DesiredNavmeshClearance());
        Tuning.Set_NavmeshResolvedRibbonTolerance(
            InParams.Get_NavmeshResolvedRibbonTolerance());
        return Resolve_RouteCostPolicy(Tuning);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Build_RouteGraphStaticData(
        const FBuiltNetwork& InNetwork,
        const FRouteCostPolicy& InCostPolicy)
        -> TSharedPtr<const FRouteGraphStaticData>
    {
        using namespace ck_pathnetwork_routeplan;

        SCOPE_CYCLE_COUNTER(
            STAT_CkPathNetwork_StaticAugmentation);

        auto StaticData = MakeShared<FRouteGraphStaticData>();
        if (InNetwork._Edges.IsEmpty()
            || NOT Is_PolicyValid(InCostPolicy))
        { return StaticData; }

        Build_ComponentTransfers(
            *StaticData,
            InNetwork,
            InCostPolicy);
        Build_LocalNetworkShortcuts(
            *StaticData,
            InNetwork,
            InCostPolicy);
        return StaticData;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Build_RouteGraphSharedData(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy,
        const TSharedPtr<const FRouteGraphStaticData>&
            InStaticData)
        -> TSharedPtr<FRouteGraphSharedData>
    {
        using namespace ck_pathnetwork_routeplan;

        auto Shared = MakeShared<FRouteGraphSharedData>();
        const bool InputsAreValid =
            NOT InNetwork._Edges.IsEmpty()
            && NOT InStartLocation.ContainsNaN()
            && NOT InGoalLocation.ContainsNaN()
            && Is_PolicyValid(InCostPolicy);
        if (NOT InputsAreValid)
        { return Shared; }

        auto StaticData = InStaticData;
        if (NOT StaticData.IsValid())
        {
            StaticData = Build_RouteGraphStaticData(
                InNetwork,
                InCostPolicy);
        }
        static_cast<FRouteGraphStaticData&>(*Shared) =
            *StaticData;

        SCOPE_CYCLE_COUNTER(
            STAT_CkPathNetwork_EndpointOverlay);
        Merge_CandidatesIntoOverlay(
            *Shared,
            Gather_RouteEndpointCandidates(
                InNetwork,
                InStartLocation,
                InCostPolicy));
        Merge_CandidatesIntoOverlay(
            *Shared,
            Gather_RouteEndpointCandidates(
                InNetwork,
                InGoalLocation,
                InCostPolicy));
        return Shared;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Search_RouteGraph(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy,
        const TSharedPtr<FRouteGraphSharedData>& InShared,
        const int32 InMaxIterations)
        -> FRoutePlanResult
    {
        auto Result = FRoutePlanResult{};
        Result._Shared = InShared;

        const bool InputsAreValid =
            NOT InNetwork._Edges.IsEmpty()
            && NOT InStartLocation.ContainsNaN()
            && NOT InGoalLocation.ContainsNaN()
            && ck_pathnetwork_routeplan::Is_PolicyValid(InCostPolicy)
            && InShared.IsValid()
            && InMaxIterations > 0;
        if (NOT InputsAreValid)
        { return Result; }

        auto SelectedPlan = ck_pathnetwork_routeplan::Search_RouteGraphOnce(
            InNetwork,
            InStartLocation,
            InGoalLocation,
            InCostPolicy,
            InShared,
            InMaxIterations);
        const auto MinimumSavings =
            InCostPolicy._DirectRouteMinimumSavingsFraction;
        const bool ShouldCompareNetworkAlternative =
            SelectedPlan._Succeeded
            && ck_pathnetwork_routeplan::Is_ExactDirectPlan(SelectedPlan)
            && InShared->_AllowDirectStartToGoal
            && NOT InShared->_OverlayPoints.IsEmpty()
            && MinimumSavings > 0.0f
            && NOT ck_pathnetwork_routeplan::Is_DirectTripGraceApplied(
                InStartLocation,
                InGoalLocation,
                InCostPolicy);
        if (NOT ShouldCompareNetworkAlternative)
        { return SelectedPlan; }

        auto NetworkOnlyShared =
            MakeShared<FRouteGraphSharedData>(*InShared);
        NetworkOnlyShared->_AllowDirectStartToGoal = false;
        auto NetworkPlan = ck_pathnetwork_routeplan::Search_RouteGraphOnce(
            InNetwork,
            InStartLocation,
            InGoalLocation,
            InCostPolicy,
            NetworkOnlyShared,
            InMaxIterations);
        const bool HasNetworkAlternative =
            NetworkPlan._Succeeded
            && Uses_Network(NetworkPlan);
        if (NOT HasNetworkAlternative)
        { return SelectedPlan; }

        const auto MaximumEligibleDirectCost =
            NetworkPlan._EstimatedCost * (1.0f - MinimumSavings);
        if (SelectedPlan._EstimatedCost <= MaximumEligibleDirectCost)
        { return SelectedPlan; }

        NetworkPlan._Shared = InShared;
        return NetworkPlan;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Plan_RouteGraph(
        const FBuiltNetwork& InNetwork,
        const FVector& InStartLocation,
        const FVector& InGoalLocation,
        const FRouteCostPolicy& InCostPolicy)
        -> FRoutePlanResult
    {
        return Search_RouteGraph(
            InNetwork,
            InStartLocation,
            InGoalLocation,
            InCostPolicy,
            Build_RouteGraphSharedData(
                InNetwork,
                InStartLocation,
                InGoalLocation,
                InCostPolicy));
    }
}

// --------------------------------------------------------------------------------------------------------------------
