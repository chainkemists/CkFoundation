#include "CkVoxelNav_Chunk_Adjacency.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_chunk_adjacency
{
    constexpr auto SubNodesPerLeaf = 64;

    // Upstream's figures, kept: a face plane is recognised within 1.5 leaf sizes, and two cells meet when
    // each reaches within half a leaf size of it.
    constexpr auto FaceToleranceInLeafSizes = 1.5;
    constexpr auto BoundaryGapInLeafSizes = 0.5;

    // A cell counts as reaching a face when its own box touches the plane. The tolerance scales with the
    // cell, because a coarse cell's arithmetic carries proportionally coarser error - with a floor, so a
    // sub-node-sized cell still gets a usable one.
    auto
        Get_FaceEpsilon(
            double InExtent)
        -> double
    {
        constexpr auto MinEpsilonUu = 1.0;
        constexpr auto EpsilonInExtents = 0.1;

        return FMath::Max(MinEpsilonUu, InExtent * EpsilonInExtents);
    }

    /** Upstream's face test, verbatim in intent: the cell's near face reaches the plane, and its centre has
     *  not run more than one extent past it. The second half is what stops a cell deep in the octree's
     *  power-of-two padding - which extends well beyond the authored chunk box - from claiming the face. */
    auto
        Get_ReachesFace(
            double InCenterOnAxis,
            double InExtent,
            double InPlaneValue,
            bool InIsMaxFace)
        -> bool
    {
        const auto Epsilon = Get_FaceEpsilon(InExtent);

        if (InIsMaxFace)
        {
            return (InCenterOnAxis + InExtent) >= (InPlaneValue - Epsilon) &&
                    InCenterOnAxis <= (InPlaneValue + InExtent);
        }

        return (InCenterOnAxis - InExtent) <= (InPlaneValue + Epsilon) &&
                InCenterOnAxis >= (InPlaneValue - InExtent);
    }

    auto
        Get_FaceMask(
            const FVector& InCenter,
            double InExtent,
            const FBox& InChunkBounds)
        -> uint8
    {
        using namespace ck::voxelnav;

        auto Mask = uint8{0};

        if (Get_ReachesFace(InCenter.X, InExtent, InChunkBounds.Max.X, true))  { Mask |= static_cast<uint8>(EChunkFace::MaxX); }
        if (Get_ReachesFace(InCenter.X, InExtent, InChunkBounds.Min.X, false)) { Mask |= static_cast<uint8>(EChunkFace::MinX); }
        if (Get_ReachesFace(InCenter.Y, InExtent, InChunkBounds.Max.Y, true))  { Mask |= static_cast<uint8>(EChunkFace::MaxY); }
        if (Get_ReachesFace(InCenter.Y, InExtent, InChunkBounds.Min.Y, false)) { Mask |= static_cast<uint8>(EChunkFace::MinY); }
        if (Get_ReachesFace(InCenter.Z, InExtent, InChunkBounds.Max.Z, true))  { Mask |= static_cast<uint8>(EChunkFace::MaxZ); }
        if (Get_ReachesFace(InCenter.Z, InExtent, InChunkBounds.Min.Z, false)) { Mask |= static_cast<uint8>(EChunkFace::MinZ); }

        return Mask;
    }

    auto
        Get_ProjectionsOverlap(
            double InA1, double InA2, double InExtentA,
            double InB1, double InB2, double InExtentB)
        -> bool
    {
        const auto OverlapsOnFirst = (InA1 + InExtentA) >= (InB1 - InExtentB) && (InA1 - InExtentA) <= (InB1 + InExtentB);
        const auto OverlapsOnSecond = (InA2 + InExtentA) >= (InB2 - InExtentB) && (InA2 - InExtentA) <= (InB2 + InExtentB);

        return OverlapsOnFirst && OverlapsOnSecond;
    }

    auto
        Get_DistanceToPlane(
            double InCenterOnAxis,
            double InExtent,
            double InPlaneValue)
        -> double
    {
        return FMath::Min(
            FMath::Abs((InCenterOnAxis - InExtent) - InPlaneValue),
            FMath::Abs((InCenterOnAxis + InExtent) - InPlaneValue));
    }

    /** Where the crossing sits on ONE of the two in-plane axes: the midpoint of the two cell centres,
     *  CLAMPED into the span both cells actually cover.
     *
     *  Upstream took the raw midpoint. Two cells of different sizes overlap on a face without their centres'
     *  midpoint lying inside either of them - a 200uu cell abutting a 50uu one puts the midpoint 37uu
     *  outside the small cell - and the resulting connection point can therefore sit in a cell that is
     *  neither, which is to say in space nothing proved free. The overlap test already guarantees the
     *  clamp range is non-empty. */
    auto
        Get_ClampedCrossing(
            const ck::voxelnav::FChunkBoundaryCell& InCellA,
            const ck::voxelnav::FChunkBoundaryCell& InCellB,
            int32 InAxis)
        -> double
    {
        const auto Low = FMath::Max(
            InCellA._Center[InAxis] - InCellA._Extent,
            InCellB._Center[InAxis] - InCellB._Extent);

        const auto High = FMath::Min(
            InCellA._Center[InAxis] + InCellA._Extent,
            InCellB._Center[InAxis] + InCellB._Extent);

        const auto Midpoint = (InCellA._Center[InAxis] + InCellB._Center[InAxis]) * 0.5;

        return FMath::Clamp(Midpoint, Low, High);
    }

    auto
        Try_AddBoundaryCell(
            const ck::voxelnav::FNodeAddress& InAddress,
            const FVector& InCenter,
            double InExtent,
            const FBox& InChunkBounds,
            TArray<ck::voxelnav::FChunkBoundaryCell>& OutCells)
        -> void
    {
        const auto FaceMask = Get_FaceMask(InCenter, InExtent, InChunkBounds);

        if (FaceMask == 0)
        { return; }

        auto Cell = ck::voxelnav::FChunkBoundaryCell{};

        Cell._Address = InAddress;
        Cell._Center = InCenter;
        Cell._Extent = static_cast<float>(InExtent);
        Cell._FaceMask = FaceMask;

        OutCells.Emplace(Cell);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Get_ChunkBoundaryCells(
            const FOctree& InOctree,
            const FBox& InChunkBounds,
            TArray<FChunkBoundaryCell>& OutCells)
        -> void
    {
        using namespace ck_voxelnav_chunk_adjacency;

        OutCells.Reset();

        if (NOT InOctree.Get_IsValid() || InChunkBounds.IsValid == 0)
        { return; }

        const auto& LeafNodes = InOctree.Get_LeafNodes();
        const auto LeafExtent = static_cast<double>(LeafNodes.Get_LeafNodeExtent());
        const auto SubNodeExtent = static_cast<double>(LeafNodes.Get_LeafSubNodeExtent());

        const auto& LeafLayer = InOctree.Get_Layer(0);

        for (auto NodeIdx = 0; NodeIdx < LeafLayer.Get_NodeCount(); ++NodeIdx)
        {
            if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx))
            { continue; }

            const auto& Leaf = LeafNodes.Get_LeafNode(NodeIdx);

            if (Leaf.Get_IsFullyOccluded())
            { continue; }

            const auto LeafAddress = FNodeAddress::Make(0, NodeIdx, 0);
            const auto LeafCenter = Get_NodePositionFromAddress(
                InOctree, LeafAddress, ECk_VoxelNav_SubNodePrecision::NodeCenter);

            // The whole leaf is nowhere near a face, so none of its 64 sub-nodes can be either.
            if (Get_FaceMask(LeafCenter, LeafExtent, InChunkBounds) == 0)
            { continue; }

            if (Leaf.Get_IsFullyFree())
            {
                // A fully-free leaf IS one cell to everything downstream: the cell seam reports the leaf's
                // own centre and extent for it regardless of sub-node index.
                Try_AddBoundaryCell(LeafAddress, LeafCenter, LeafExtent, InChunkBounds, OutCells);
                continue;
            }

            for (auto SubIdx = 0; SubIdx < SubNodesPerLeaf; ++SubIdx)
            {
                const auto SubNodeIdx = static_cast<SubNodeIndex>(SubIdx);

                if (Leaf.Get_IsSubNodeOccluded(SubNodeIdx))
                { continue; }

                const auto SubAddress = FNodeAddress::Make(0, NodeIdx, SubNodeIdx);

                Try_AddBoundaryCell(SubAddress,
                    Get_NodePositionFromAddress(InOctree, SubAddress, ECk_VoxelNav_SubNodePrecision::SubNodeCenter),
                    SubNodeExtent, InChunkBounds, OutCells);
            }
        }

        for (auto LayerIdx = 1; LayerIdx < InOctree.Get_LayerCount(); ++LayerIdx)
        {
            const auto Layer = static_cast<LayerIndex>(LayerIdx);
            const auto& Nodes = InOctree.Get_Layer(Layer).Get_Nodes();
            const auto NodeExtent = static_cast<double>(InOctree.Get_Layer(Layer).Get_NodeExtent());

            for (auto NodeIdx = 0; NodeIdx < Nodes.Num(); ++NodeIdx)
            {
                // A subdivided node is not a cell - its children are, and this walk reaches them at their
                // own layer.
                if (Nodes[NodeIdx].Get_HasChildren())
                { continue; }

                Try_AddBoundaryCell(FNodeAddress::Make(Layer, NodeIdx, 0),
                    Get_NodePositionFromLayerAndMorton(InOctree, Layer, Nodes[NodeIdx].Get_MortonCode()),
                    NodeExtent, InChunkBounds, OutCells);
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_SharedFace(
            const FBox& InBoundsA,
            const FBox& InBoundsB,
            float InLeafNodeSizeUu)
        -> uint8
    {
        using namespace ck_voxelnav_chunk_adjacency;

        if (InBoundsA.IsValid == 0 || InBoundsB.IsValid == 0 || InLeafNodeSizeUu <= 0.0f)
        { return 0; }

        const auto FaceTolerance = InLeafNodeSizeUu * FaceToleranceInLeafSizes;

        if (FMath::IsNearlyEqual(InBoundsA.Max.X, InBoundsB.Min.X, FaceTolerance)) { return static_cast<uint8>(EChunkFace::MaxX); }
        if (FMath::IsNearlyEqual(InBoundsB.Max.X, InBoundsA.Min.X, FaceTolerance)) { return static_cast<uint8>(EChunkFace::MinX); }
        if (FMath::IsNearlyEqual(InBoundsA.Max.Y, InBoundsB.Min.Y, FaceTolerance)) { return static_cast<uint8>(EChunkFace::MaxY); }
        if (FMath::IsNearlyEqual(InBoundsB.Max.Y, InBoundsA.Min.Y, FaceTolerance)) { return static_cast<uint8>(EChunkFace::MinY); }
        if (FMath::IsNearlyEqual(InBoundsA.Max.Z, InBoundsB.Min.Z, FaceTolerance)) { return static_cast<uint8>(EChunkFace::MaxZ); }
        if (FMath::IsNearlyEqual(InBoundsB.Max.Z, InBoundsA.Min.Z, FaceTolerance)) { return static_cast<uint8>(EChunkFace::MinZ); }

        return 0;
    }

    auto
        Get_FaceAxis(
            uint8 InFace)
        -> int32
    {
        switch (static_cast<EChunkFace>(InFace))
        {
            case EChunkFace::MaxX:
            case EChunkFace::MinX: return 0;
            case EChunkFace::MaxY:
            case EChunkFace::MinY: return 1;
            case EChunkFace::MaxZ:
            case EChunkFace::MinZ: return 2;
            default: break;
        }

        return INDEX_NONE;
    }

    auto
        Get_FacePlaneValue(
            const FBox& InBounds,
            uint8 InFace)
        -> double
    {
        const auto Axis = Get_FaceAxis(InFace);

        if (Axis == INDEX_NONE)
        { return 0.0; }

        const auto IsMaxFace =
            InFace == static_cast<uint8>(EChunkFace::MaxX) ||
            InFace == static_cast<uint8>(EChunkFace::MaxY) ||
            InFace == static_cast<uint8>(EChunkFace::MaxZ);

        return IsMaxFace ? InBounds.Max[Axis] : InBounds.Min[Axis];
    }

    auto
        Get_OppositeFace(
            uint8 InFace)
        -> uint8
    {
        switch (static_cast<EChunkFace>(InFace))
        {
            case EChunkFace::MaxX: return static_cast<uint8>(EChunkFace::MinX);
            case EChunkFace::MinX: return static_cast<uint8>(EChunkFace::MaxX);
            case EChunkFace::MaxY: return static_cast<uint8>(EChunkFace::MinY);
            case EChunkFace::MinY: return static_cast<uint8>(EChunkFace::MaxY);
            case EChunkFace::MaxZ: return static_cast<uint8>(EChunkFace::MinZ);
            case EChunkFace::MinZ: return static_cast<uint8>(EChunkFace::MaxZ);
            default: break;
        }

        return 0;
    }

    auto
        Get_CellsConnectAcrossPlane(
            const FChunkBoundaryCell& InCellA,
            const FChunkBoundaryCell& InCellB,
            double InPlaneValue,
            int32 InPlaneAxis,
            double InMaxGapUu)
        -> bool
    {
        using namespace ck_voxelnav_chunk_adjacency;

        if (InPlaneAxis < 0 || InPlaneAxis > 2)
        { return false; }

        const auto FirstAxis = (InPlaneAxis + 1) % 3;
        const auto SecondAxis = (InPlaneAxis + 2) % 3;

        if (NOT Get_ProjectionsOverlap(
            InCellA._Center[FirstAxis], InCellA._Center[SecondAxis], InCellA._Extent,
            InCellB._Center[FirstAxis], InCellB._Center[SecondAxis], InCellB._Extent))
        { return false; }

        const auto DistanceA = Get_DistanceToPlane(InCellA._Center[InPlaneAxis], InCellA._Extent, InPlaneValue);
        const auto DistanceB = Get_DistanceToPlane(InCellB._Center[InPlaneAxis], InCellB._Extent, InPlaneValue);

        return DistanceA <= InMaxGapUu && DistanceB <= InMaxGapUu;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Bake_ChunkAdjacency(
            const TArray<FChunkAdjacencyInput>& InChunks)
        -> FChunkAdjacencyTable
    {
        using namespace ck_voxelnav_chunk_adjacency;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Bake_ChunkAdjacency);

        auto Table = FChunkAdjacencyTable{};

        auto BoundaryCellsByChunk = TArray<TArray<FChunkBoundaryCell>>{};
        auto LeafNodeSizeByChunk = TArray<float>{};

        BoundaryCellsByChunk.SetNum(InChunks.Num());
        LeafNodeSizeByChunk.SetNumZeroed(InChunks.Num());

        for (auto ChunkIdx = 0; ChunkIdx < InChunks.Num(); ++ChunkIdx)
        {
            const auto& Chunk = InChunks[ChunkIdx];

            if (ck::Is_NOT_Valid(Chunk._Octree, ck::IsValid_Policy_NullptrOnly{}) || NOT Chunk._Octree->Get_IsValid())
            { continue; }

            LeafNodeSizeByChunk[ChunkIdx] = Chunk._Octree->Get_LeafNodes().Get_LeafNodeSize();

            Get_ChunkBoundaryCells(*Chunk._Octree, Chunk._Bounds, BoundaryCellsByChunk[ChunkIdx]);
        }

        for (auto FirstIdx = 0; FirstIdx < InChunks.Num(); ++FirstIdx)
        {
            for (auto SecondIdx = FirstIdx + 1; SecondIdx < InChunks.Num(); ++SecondIdx)
            {
                const auto& ChunkA = InChunks[FirstIdx];
                const auto& ChunkB = InChunks[SecondIdx];

                const auto LeafNodeSize = LeafNodeSizeByChunk[FirstIdx];

                if (LeafNodeSize <= 0.0f || LeafNodeSizeByChunk[SecondIdx] <= 0.0f)
                { continue; }

                // The prefilter: two chunks that do not touch even after growing one by a leaf cannot
                // share a face, and skipping them here is what keeps the pair loop's O(n^2) from mattering.
                if (NOT ChunkA._Bounds.ExpandBy(LeafNodeSize).Intersect(ChunkB._Bounds))
                { continue; }

                const auto FaceOnA = Get_SharedFace(ChunkA._Bounds, ChunkB._Bounds, LeafNodeSize);

                if (FaceOnA == 0)
                { continue; }

                const auto FaceOnB = Get_OppositeFace(FaceOnA);
                const auto PlaneAxis = Get_FaceAxis(FaceOnA);
                const auto PlaneValue = Get_FacePlaneValue(ChunkA._Bounds, FaceOnA);
                const auto MaxGap = LeafNodeSize * BoundaryGapInLeafSizes;

                const auto FirstAxis = (PlaneAxis + 1) % 3;
                const auto SecondAxis = (PlaneAxis + 2) % 3;

                for (const auto& CellA : BoundaryCellsByChunk[FirstIdx])
                {
                    if ((CellA._FaceMask & FaceOnA) == 0)
                    { continue; }

                    for (const auto& CellB : BoundaryCellsByChunk[SecondIdx])
                    {
                        if ((CellB._FaceMask & FaceOnB) == 0)
                        { continue; }

                        if (NOT Get_CellsConnectAcrossPlane(CellA, CellB, PlaneValue, PlaneAxis, MaxGap))
                        { continue; }

                        auto ConnectionPoint = FVector::ZeroVector;

                        ConnectionPoint[PlaneAxis] = PlaneValue;
                        ConnectionPoint[FirstAxis] = Get_ClampedCrossing(CellA, CellB, FirstAxis);
                        ConnectionPoint[SecondAxis] = Get_ClampedCrossing(CellA, CellB, SecondAxis);

                        auto Forward = FChunkPortal{};

                        Forward._FromChunk = ChunkA._ChunkId;
                        Forward._ToChunk = ChunkB._ChunkId;
                        Forward._FromCellPacked = CellA._Address.Get_Packed();
                        Forward._ToCellPacked = CellB._Address.Get_Packed();
                        Forward._ConnectionPoint = ConnectionPoint;

                        auto Reverse = FChunkPortal{};

                        Reverse._FromChunk = ChunkB._ChunkId;
                        Reverse._ToChunk = ChunkA._ChunkId;
                        Reverse._FromCellPacked = CellB._Address.Get_Packed();
                        Reverse._ToCellPacked = CellA._Address.Get_Packed();
                        Reverse._ConnectionPoint = ConnectionPoint;

                        Table.Request_AddPortal(Forward);
                        Table.Request_AddPortal(Reverse);
                    }
                }
            }
        }

        return Table;
    }
}

// --------------------------------------------------------------------------------------------------------------------
