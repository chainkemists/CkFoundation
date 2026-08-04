#include "CkVoxelNav_Chunk_Search.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

#include <Algo/Reverse.h>
#include <Containers/Queue.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_chunk_search
{
    auto
        Get_ChunkIsSearchable(
            const ck::voxelnav::FChunkSearchInput& InChunk)
        -> bool
    {
        return InChunk._Octree.IsValid() && InChunk._Octree->Get_IsValid();
    }

    auto
        Get_CellCenterOfPortalSide(
            const ck::voxelnav::FChunkSearchInput& InChunk,
            uint32 InPackedAddress)
        -> FVector
    {
        using namespace ck::voxelnav;

        return Get_CellCenter(*InChunk._Octree,
            FCellId::Make_FromNodeAddress(InChunk._CellVolumeId, FNodeAddress::Make_FromPacked(InPackedAddress)));
    }

    /** Greedy and deterministic: the crossing that minimises "how far to reach it plus how far it still
     *  leaves to go". Upstream picked purely by proximity to the chunk being left, which happily chooses a
     *  portal on the far side of a face from the goal. */
    auto
        TryGet_BestPortal(
            TArrayView<const ck::voxelnav::FChunkPortal> InPortals,
            const FVector& InFrom,
            const FVector& InGoal)
        -> const ck::voxelnav::FChunkPortal*
    {
        const ck::voxelnav::FChunkPortal* Best = nullptr;
        auto BestScore = TNumericLimits<double>::Max();

        for (const auto& Portal : InPortals)
        {
            const auto Score =
                FVector::Dist(InFrom, Portal._ConnectionPoint) +
                FVector::Dist(Portal._ConnectionPoint, InGoal);

            if (Score >= BestScore)
            { continue; }

            BestScore = Score;
            Best = &Portal;
        }

        return Best;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        TryGet_ChunkIndexContainingPoint(
            const TArray<FChunkSearchInput>& InChunks,
            const FVector& InPoint)
        -> int32
    {
        using namespace ck_voxelnav_chunk_search;

        auto NearestIndex = FChunkId::InvalidIndex;
        auto NearestDistanceSq = TNumericLimits<double>::Max();

        for (auto ChunkIdx = 0; ChunkIdx < InChunks.Num(); ++ChunkIdx)
        {
            const auto& Chunk = InChunks[ChunkIdx];

            if (NOT Get_ChunkIsSearchable(Chunk) || Chunk._Bounds.IsValid == 0)
            { continue; }

            // First containing chunk in lattice order wins, which matters only for a point exactly on a
            // shared face - and there both answers are correct, so a stable one is what a caller needs.
            if (Chunk._Bounds.IsInsideOrOn(InPoint))
            { return ChunkIdx; }

            const auto DistanceSq = Chunk._Bounds.ComputeSquaredDistanceToPoint(InPoint);

            if (DistanceSq >= NearestDistanceSq)
            { continue; }

            NearestDistanceSq = DistanceSq;
            NearestIndex = ChunkIdx;
        }

        return NearestIndex;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ChunkPath(
            const FChunkAdjacencyTable& InAdjacency,
            int32 InFromChunkIndex,
            int32 InToChunkIndex,
            TArray<int32>& OutChunkPath)
        -> bool
    {
        OutChunkPath.Reset();

        if (InFromChunkIndex < 0 || InToChunkIndex < 0)
        { return false; }

        if (InFromChunkIndex == InToChunkIndex)
        {
            OutChunkPath.Emplace(InFromChunkIndex);
            return true;
        }

        auto Frontier = TQueue<int32>{};
        auto CameFrom = TMap<int32, int32>{};
        auto Visited = TSet<int32>{};

        Frontier.Enqueue(InFromChunkIndex);
        Visited.Emplace(InFromChunkIndex);

        while (NOT Frontier.IsEmpty())
        {
            auto Current = FChunkId::InvalidIndex;
            Frontier.Dequeue(Current);

            if (Current == InToChunkIndex)
            {
                for (auto Node = InToChunkIndex; Node != InFromChunkIndex; Node = CameFrom[Node])
                { OutChunkPath.Emplace(Node); }

                OutChunkPath.Emplace(InFromChunkIndex);
                Algo::Reverse(OutChunkPath);

                return true;
            }

            for (const auto Neighbor : InAdjacency.Get_NeighborIndices(Current))
            {
                if (Visited.Contains(Neighbor))
                { continue; }

                Visited.Emplace(Neighbor);
                CameFrom.Emplace(Neighbor, Current);
                Frontier.Enqueue(Neighbor);
            }
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CellAddressesNameSameCell(
            const FOctree& InOctree,
            const FNodeAddress& InLeft,
            const FNodeAddress& InRight)
        -> bool
    {
        if (InLeft == InRight)
        { return true; }

        if (InLeft.Get_LayerIndex() != 0 || InRight.Get_LayerIndex() != 0)
        { return false; }

        if (InLeft.Get_NodeIndex() != InRight.Get_NodeIndex())
        { return false; }

        const auto& LeafNodes = InOctree.Get_LeafNodes();

        if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(InLeft.Get_NodeIndex()))
        { return false; }

        return LeafNodes.Get_LeafNode(InLeft.Get_NodeIndex()).Get_IsFullyFree();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Search_ChunkedPathGraph(
            const TArray<FChunkSearchInput>& InChunks,
            const FChunkAdjacencyTable& InAdjacency,
            const FPathSearchParams& InParams)
        -> FChunkedPathPlan
    {
        using namespace ck_voxelnav_chunk_search;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Search_ChunkedPathGraph);

        auto Plan = FChunkedPathPlan{};
        Plan._PlannedAgainstEpoch = InParams._PlannedAgainstEpoch;

        if (InChunks.IsEmpty())
        {
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::NoBake;
            return Plan;
        }

        const auto StartChunkIndex = TryGet_ChunkIndexContainingPoint(InChunks, InParams._From);
        const auto GoalChunkIndex = TryGet_ChunkIndexContainingPoint(InChunks, InParams._To);

        if (StartChunkIndex == FChunkId::InvalidIndex || GoalChunkIndex == FChunkId::InvalidIndex)
        {
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::NoBake;
            return Plan;
        }

        auto ChunkPath = TArray<int32>{};

        if (NOT Get_ChunkPath(InAdjacency, StartChunkIndex, GoalChunkIndex, ChunkPath))
        {
            // The endpoints resolved but no sequence of shared faces joins their chunks, which is the same
            // answer an unpartitioned volume gives when no sequence of free cells joins two points.
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::Unreachable;
            return Plan;
        }

        // The adjacency's integer keys index straight into the chunk array, so a key naming no chunk means
        // the table and the chunk record were built from different partitions.
        for (const auto ChunkIndex : ChunkPath)
        {
            if (InChunks.IsValidIndex(ChunkIndex) && Get_ChunkIsSearchable(InChunks[ChunkIndex]))
            { continue; }

            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::NoBake;
            return Plan;
        }

        Plan._Segments.Reserve(ChunkPath.Num());
        Plan._Portals.Reserve(FMath::Max(0, ChunkPath.Num() - 1));

        auto SegmentFrom = InParams._From;

        for (auto Leg = 0; Leg < ChunkPath.Num(); ++Leg)
        {
            const auto ChunkIndex = ChunkPath[Leg];
            const auto& Chunk = InChunks[ChunkIndex];

            const auto IsLastLeg = Leg == ChunkPath.Num() - 1;

            const auto* Portal = IsLastLeg
                ? nullptr
                : TryGet_BestPortal(
                    InAdjacency.Get_Portals(ChunkIndex, ChunkPath[Leg + 1]), SegmentFrom, InParams._To);

            if (NOT IsLastLeg && Portal == nullptr)
            {
                // The BFS walked an edge the portal table does not carry, which can only mean the two were
                // built from different bakes.
                Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::Unreachable;
                return Plan;
            }

            // The segment ends at the portal CELL's centre rather than at the connection point: a cell
            // centre resolves back to that exact cell, so the route's last cell in this chunk is provably
            // the crossing the adjacency named.
            const auto SegmentTo = IsLastLeg
                ? InParams._To
                : Get_CellCenterOfPortalSide(Chunk, Portal->_FromCellPacked);

            auto SegmentParams = InParams;
            SegmentParams._From = SegmentFrom;
            SegmentParams._To = SegmentTo;

            const auto SegmentPlan = Search_PathGraph(*Chunk._Octree, Chunk._CellVolumeId, SegmentParams);

            Plan._Iterations += SegmentPlan._Iterations;

            if (SegmentPlan._Outcome != ECk_VoxelNav_PathSearchOutcome::Succeeded)
            {
                Plan._Outcome = SegmentPlan._Outcome;
                Plan._Segments.Reset();
                Plan._Portals.Reset();
                Plan._Waypoints.Reset();

                return Plan;
            }

            Plan._TotalCost += SegmentPlan._TotalCost;

            auto Segment = FChunkedPathSegment{};

            Segment._ChunkId = Chunk._ChunkId;
            Segment._ChunkIndex = ChunkIndex;
            Segment._Cells = SegmentPlan._Cells;
            Segment._Waypoints = SegmentPlan._Waypoints;

            Plan._Segments.Emplace(MoveTemp(Segment));

            if (IsLastLeg)
            { break; }

            Plan._Portals.Emplace(*Portal);

            SegmentFrom = Get_CellCenterOfPortalSide(InChunks[ChunkPath[Leg + 1]], Portal->_ToCellPacked);
        }

        for (auto SegmentIdx = 0; SegmentIdx < Plan._Segments.Num(); ++SegmentIdx)
        {
            if (SegmentIdx > 0)
            { Plan._Waypoints.Emplace(Plan._Portals[SegmentIdx - 1]._ConnectionPoint); }

            Plan._Waypoints.Append(Plan._Segments[SegmentIdx]._Waypoints);
        }

        Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::Succeeded;

        return Plan;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Refine_ChunkedWaypoints(
            const TArray<FChunkSearchInput>& InChunks,
            const FChunkedPathPlan& InPlan,
            const FPathRefineParams& InParams)
        -> FPathRefineResult
    {
        auto Result = FPathRefineResult{};

        Result._RawWaypointCount = InPlan._Waypoints.Num();
        Result._RawLengthUu = Get_PathLength(InPlan._Waypoints);

        if (InPlan._Outcome != ECk_VoxelNav_PathSearchOutcome::Succeeded)
        {
            Result._Waypoints = InPlan._Waypoints;
            Result._RefinedLengthUu = Result._RawLengthUu;

            return Result;
        }

        for (auto SegmentIdx = 0; SegmentIdx < InPlan._Segments.Num(); ++SegmentIdx)
        {
            const auto& Segment = InPlan._Segments[SegmentIdx];

            if (SegmentIdx > 0)
            { Result._Waypoints.Emplace(InPlan._Portals[SegmentIdx - 1]._ConnectionPoint); }

            if (NOT InChunks.IsValidIndex(Segment._ChunkIndex) ||
                NOT InChunks[Segment._ChunkIndex]._Octree.IsValid())
            {
                Result._Waypoints.Append(Segment._Waypoints);
                continue;
            }

            const auto Refined = Refine_Waypoints(
                *InChunks[Segment._ChunkIndex]._Octree, Segment._Waypoints, InParams);

            Result._Waypoints.Append(Refined._Waypoints);
        }

        Result._RefinedLengthUu = Get_PathLength(Result._Waypoints);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
