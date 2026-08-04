#include "CkVoxelNav_Octree_Rasterize.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Morton.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_rasterize
{
    constexpr auto ChildrenPerNode = 8;

    // Leaf nodes hang off layer 1, so a leaf's parent always lives there.
    constexpr auto LeafParentLayerIndex = ck::voxelnav::LayerIndex{1};

    constexpr auto SubNodesPerLeaf = 64;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Request_ResetScratch(
            FRasterizeScratch& InOutScratch)
        -> void
    {
        InOutScratch._BlockedNodes.Reset();
        InOutScratch._LeafMortonCodes.Reset();
        InOutScratch._OccludedLeafMortons.Reset();
        InOutScratch._LeafIndexToParentMorton.Reset();
        InOutScratch._Cursor = 0;
        InOutScratch._SubCursor = 0;
        InOutScratch._LayerCursor = 0;
    }

    auto
        Request_InitializeScratch(
            const FOctree& InOctree,
            FRasterizeScratch& InOutScratch)
        -> void
    {
        Request_ResetScratch(InOutScratch);

        const auto HasLayers = InOctree.Get_LayerCount() > 0;

        CK_ENSURE_IF_NOT(HasLayers,
            TEXT("Cannot prepare rasterize scratch for a VoxelNav octree that has no layers"))
        {}

        if (NOT HasLayers)
        { return; }

        InOutScratch._BlockedNodes.SetNum(InOctree.Get_LayerCount() + 1);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Stage_ProbeVolumeEmpty(
            const FOctree& InOctree,
            const ICk_VoxelNav_GeometryBackend& InBackend)
        -> FVolumeProbeResult
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Stage_ProbeVolumeEmpty);

        auto Bodies = TArray<FCk_VoxelNav_BodyId>{};
        InBackend.Get_BodiesInBox(InOctree.Get_NavigationBounds(), Bodies);

        auto Result = FVolumeProbeResult{};
        Result._BodyCount = Bodies.Num();
        Result._VolumeIsEmpty = Bodies.IsEmpty();

        return Result;
    }

    auto
        Stage_RasterizeL1(
            const FOctree& InOctree,
            FRasterizeScratch& InOutScratch,
            const ICk_VoxelNav_GeometryBackend& InBackend,
            float InClearanceUu,
            int32 InProbeBudget)
        -> FRasterizeStageResult
    {
        using namespace ck_voxelnav_octree_rasterize;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Stage_RasterizeL1);

        auto Result = FRasterizeStageResult{};

        const auto OctreeCarriesALayerOne =
            InOctree.Get_LayerCount() > LeafParentLayerIndex &&
            InOutScratch._BlockedNodes.Num() > 0;

        CK_ENSURE_IF_NOT(OctreeCarriesALayerOne,
            TEXT("Cannot rasterize layer 1 of a VoxelNav octree with [{}] layers against a scratch holding "
                 "[{}] blocked-node sets"),
            InOctree.Get_LayerCount(), InOutScratch._BlockedNodes.Num())
        {}

        if (NOT OctreeCarriesALayerOne)
        { return Result; }

        const auto& Layer = InOctree.Get_Layer(LeafParentLayerIndex);
        const auto CellCount = Layer.Get_MaxNodeCount();
        const auto ProbeHalfExtents = FVector{Layer.Get_NodeExtent() + InClearanceUu};

        for (; InOutScratch._Cursor < CellCount && Result._ProbesSpent < InProbeBudget; ++InOutScratch._Cursor)
        {
            const auto CellMortonCode = static_cast<MortonCode>(InOutScratch._Cursor);
            const auto CellPosition = Get_NodePositionFromLayerAndMorton(
                InOctree, LeafParentLayerIndex, CellMortonCode);

            ++Result._ProbesSpent;

            if (NOT InBackend.Get_IsBoxOccupied(CellPosition, ProbeHalfExtents))
            { continue; }

            InOutScratch._BlockedNodes[0].Add(CellMortonCode);
        }

        Result._StageComplete = InOutScratch._Cursor >= CellCount;

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Stage_PropagateBlockedUpward(
            const FOctree& InOctree,
            FRasterizeScratch& InOutScratch)
        -> void
    {
        const auto LayerCount = InOctree.Get_LayerCount();
        const auto ScratchIsSized = InOutScratch._BlockedNodes.Num() >= LayerCount + 1;

        CK_ENSURE_IF_NOT(ScratchIsSized,
            TEXT("Rasterize scratch holds [{}] blocked-node sets, which cannot carry a VoxelNav octree of "
                 "[{}] layers"),
            InOutScratch._BlockedNodes.Num(), LayerCount)
        {}

        if (NOT ScratchIsSized)
        { return; }

        for (auto LayerCursor = 1; LayerCursor < LayerCount; ++LayerCursor)
        {
            const auto& ChildCodes = InOutScratch._BlockedNodes[LayerCursor - 1];
            auto& ParentCodes = InOutScratch._BlockedNodes[LayerCursor];

            ParentCodes.Reserve(ChildCodes.Num());

            for (const auto ChildCode : ChildCodes)
            { ParentCodes.Add(Get_ParentMorton(ChildCode)); }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Stage_AllocateLeaves(
            FOctree& InOutOctree,
            FRasterizeScratch& InOutScratch)
        -> void
    {
        using namespace ck_voxelnav_octree_rasterize;

        const auto ScratchIsInitialized = InOutScratch._BlockedNodes.Num() > 0;

        CK_ENSURE_IF_NOT(ScratchIsInitialized,
            TEXT("Cannot allocate VoxelNav leaves from a rasterize scratch that holds no blocked-node sets"))
        {}

        if (NOT ScratchIsInitialized)
        { return; }

        const auto& BlockedParentCodes = InOutScratch._BlockedNodes[0];

        InOutScratch._LeafMortonCodes.Reset();
        InOutScratch._LeafMortonCodes.Reserve(BlockedParentCodes.Num() * ChildrenPerNode);

        // Unlike the interior layers, no child code needs a capacity check here: both lattices are cubes
        // of a power-of-two edge, so the eight children of any in-range layer-1 code are in range on
        // layer 0 by construction. Dropping one would break the leaf store's parallelism with the layer.
        for (const auto ParentMortonCode : BlockedParentCodes)
        {
            const auto FirstChildMortonCode = Get_FirstChildMorton(ParentMortonCode);

            for (auto ChildOffset = 0; ChildOffset < ChildrenPerNode; ++ChildOffset)
            { InOutScratch._LeafMortonCodes.Emplace(FirstChildMortonCode + ChildOffset); }
        }

        InOutOctree.Get_LeafNodes().Request_SetLeafCount(InOutScratch._LeafMortonCodes.Num());
        InOutOctree.Get_Layer(0).Get_Nodes().Reserve(InOutScratch._LeafMortonCodes.Num());
    }

    auto
        Stage_ProbeLeafOccupancy(
            const FOctree& InOctree,
            FRasterizeScratch& InOutScratch,
            const ICk_VoxelNav_GeometryBackend& InBackend,
            float InClearanceUu,
            int32 InProbeBudget)
        -> FRasterizeStageResult
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Stage_ProbeLeafOccupancy);

        auto Result = FRasterizeStageResult{};

        const auto LeafCount = InOutScratch._LeafMortonCodes.Num();
        const auto ProbeHalfExtents = FVector{InOctree.Get_LeafNodes().Get_LeafNodeExtent() + InClearanceUu};

        for (; InOutScratch._Cursor < LeafCount && Result._ProbesSpent < InProbeBudget; ++InOutScratch._Cursor)
        {
            const auto LeafMortonCode = InOutScratch._LeafMortonCodes[InOutScratch._Cursor];
            const auto LeafPosition = Get_LeafNodePositionFromMorton(InOctree, LeafMortonCode);

            ++Result._ProbesSpent;

            if (NOT InBackend.Get_IsBoxOccupied(LeafPosition, ProbeHalfExtents))
            { continue; }

            InOutScratch._OccludedLeafMortons.Add(LeafMortonCode);
        }

        Result._StageComplete = InOutScratch._Cursor >= LeafCount;

        return Result;
    }

    auto
        Stage_SortLeafLayer(
            FOctree& InOutOctree,
            FRasterizeScratch& InOutScratch)
        -> void
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Stage_SortLeafLayer);

        InOutScratch._LeafMortonCodes.Sort();

        auto& LeafLayerNodes = InOutOctree.Get_Layer(0).Get_Nodes();
        LeafLayerNodes.Reset();
        LeafLayerNodes.Reserve(InOutScratch._LeafMortonCodes.Num());

        InOutScratch._LeafIndexToParentMorton.Reset();
        InOutScratch._LeafIndexToParentMorton.Reserve(InOutScratch._LeafMortonCodes.Num());

        for (auto LeafIdx = 0; LeafIdx < InOutScratch._LeafMortonCodes.Num(); ++LeafIdx)
        {
            const auto LeafMortonCode = InOutScratch._LeafMortonCodes[LeafIdx];

            auto LeafLayerNode = FNode{LeafMortonCode};

            // A layer-0 node's child link addresses its LEAF entry, and the leaf store is parallel to the
            // layer, so an occluded leaf points at its own index. A free leaf stays childless, which is
            // how every query reads "this whole cell is navigable".
            if (InOutScratch._OccludedLeafMortons.Contains(LeafMortonCode))
            { LeafLayerNode.Set_FirstChild(FNodeAddress::Make(0, LeafIdx)); }

            LeafLayerNodes.Add(LeafLayerNode);

            InOutScratch._LeafIndexToParentMorton.Add(LeafIdx, Get_ParentMorton(LeafMortonCode));
        }

        InOutOctree.Get_LeafNodes().Request_SetLeafCount(LeafLayerNodes.Num());
    }

    auto
        Stage_RasterizeLeafSubNodes(
            FOctree& InOutOctree,
            FRasterizeScratch& InOutScratch,
            const ICk_VoxelNav_GeometryBackend& InBackend,
            float InClearanceUu,
            int32 InProbeBudget)
        -> FRasterizeStageResult
    {
        using namespace ck_voxelnav_octree_rasterize;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Stage_RasterizeLeafSubNodes);

        auto Result = FRasterizeStageResult{};

        const auto SubNodeSize = InOutOctree.Get_LeafNodes().Get_LeafSubNodeSize();
        const auto SubNodeExtent = InOutOctree.Get_LeafNodes().Get_LeafSubNodeExtent();
        const auto LeafNodeExtent = InOutOctree.Get_LeafNodes().Get_LeafNodeExtent();
        const auto ProbeHalfExtents = FVector{SubNodeExtent + InClearanceUu};

        const auto LeafLayerNodeCount = InOutOctree.Get_Layer(0).Get_NodeCount();

        while (InOutScratch._Cursor < LeafLayerNodeCount && Result._ProbesSpent < InProbeBudget)
        {
            const auto& LeafLayerNode = InOutOctree.Get_Layer(0).Get_Node(InOutScratch._Cursor);

            if (NOT LeafLayerNode.Get_HasChildren())
            {
                ++InOutScratch._Cursor;
                InOutScratch._SubCursor = 0;
                continue;
            }

            const auto LeafOrigin =
                Get_LeafNodePositionFromMorton(InOutOctree, LeafLayerNode.Get_MortonCode()) -
                FVector{LeafNodeExtent};

            for (; InOutScratch._SubCursor < SubNodesPerLeaf && Result._ProbesSpent < InProbeBudget;
                   ++InOutScratch._SubCursor)
            {
                const auto SubNodeIdx = static_cast<SubNodeIndex>(InOutScratch._SubCursor);
                const auto SubNodeCenter = LeafOrigin +
                                           Get_VectorFromMorton(SubNodeIdx) * SubNodeSize +
                                           FVector{SubNodeExtent};

                ++Result._ProbesSpent;

                if (NOT InBackend.Get_IsBoxOccupied(SubNodeCenter, ProbeHalfExtents))
                { continue; }

                InOutOctree.Get_LeafNodes()
                    .Get_LeafNode(InOutScratch._Cursor)
                    .Request_MarkSubNodeOccluded(SubNodeIdx);
            }

            if (InOutScratch._SubCursor < SubNodesPerLeaf)
            { break; }

            ++InOutScratch._Cursor;
            InOutScratch._SubCursor = 0;
        }

        Result._StageComplete = InOutScratch._Cursor >= LeafLayerNodeCount;

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Stage_RasterizeLayer(
            FOctree& InOutOctree,
            FRasterizeScratch& InOutScratch,
            LayerIndex InLayerIndex)
        -> FRasterizeStageResult
    {
        using namespace ck_voxelnav_octree_rasterize;

        auto Result = FRasterizeStageResult{};

        const auto LayerCount = InOutOctree.Get_LayerCount();
        const auto LayerIsRasterizable =
            InLayerIndex > 0 &&
            InLayerIndex < LayerCount &&
            InOutScratch._BlockedNodes.IsValidIndex(InLayerIndex);

        CK_ENSURE_IF_NOT(LayerIsRasterizable,
            TEXT("Cannot rasterize VoxelNav layer [{}] - the octree has [{}] layers and the scratch holds "
                 "[{}] blocked-node sets"),
            static_cast<int32>(InLayerIndex), LayerCount, InOutScratch._BlockedNodes.Num())
        {}

        if (NOT LayerIsRasterizable)
        { return Result; }

        const auto& BlockedParentCodes = InOutScratch._BlockedNodes[InLayerIndex];
        const auto MaxNodeCount = static_cast<MortonCode>(InOutOctree.Get_Layer(InLayerIndex).Get_MaxNodeCount());

        auto NodeMortonCodes = TArray<MortonCode>{};
        NodeMortonCodes.Reserve(BlockedParentCodes.Num() * ChildrenPerNode);

        for (const auto ParentMortonCode : BlockedParentCodes)
        {
            const auto FirstChildMortonCode = Get_FirstChildMorton(ParentMortonCode);

            for (auto ChildOffset = 0; ChildOffset < ChildrenPerNode; ++ChildOffset)
            {
                const auto ChildMortonCode = FirstChildMortonCode + ChildOffset;

                if (ChildMortonCode >= MaxNodeCount)
                { continue; }

                NodeMortonCodes.Emplace(ChildMortonCode);
            }
        }

        // Ascending Morton order is the layer's search invariant, AND the child-to-parent links written
        // below record positions in the node array - so the order has to be settled before any of them
        // are assigned, not by sorting afterwards.
        NodeMortonCodes.Sort();

        const auto ChildLayerIndex = static_cast<LayerIndex>(InLayerIndex - 1);

        InOutOctree.Get_Layer(InLayerIndex).Get_Nodes().Reserve(
            InOutOctree.Get_Layer(InLayerIndex).Get_NodeCount() + NodeMortonCodes.Num());

        for (const auto NodeMortonCode : NodeMortonCodes)
        {
            auto LayerNode = FNode{NodeMortonCode};

            const auto FirstChildIndex = Get_NodeIndexFromMorton(
                InOutOctree, ChildLayerIndex, Get_FirstChildMorton(NodeMortonCode));

            if (FirstChildIndex != INDEX_NONE)
            {
                LayerNode.Set_FirstChild(FNodeAddress::Make(ChildLayerIndex, FirstChildIndex));

                const auto NewNodeIndex = InOutOctree.Get_Layer(InLayerIndex).Get_NodeCount();
                const auto ParentAddress = FNodeAddress::Make(InLayerIndex, NewNodeIndex);

                auto& ChildNodes = InOutOctree.Get_Layer(ChildLayerIndex).Get_Nodes();

                for (auto ChildOffset = 0; ChildOffset < ChildrenPerNode; ++ChildOffset)
                {
                    const auto ChildIndex = FirstChildIndex + ChildOffset;

                    if (NOT ChildNodes.IsValidIndex(ChildIndex))
                    { continue; }

                    ChildNodes[ChildIndex].Set_Parent(ParentAddress);
                }
            }

            InOutOctree.Get_Layer(InLayerIndex).Get_Nodes().Add(LayerNode);
        }

        Result._StageComplete = true;

        return Result;
    }

    auto
        Stage_BuildParentLinks(
            FOctree& InOutOctree,
            const FRasterizeScratch& InScratch)
        -> void
    {
        using namespace ck_voxelnav_octree_rasterize;

        for (const auto& LeafToParentMorton : InScratch._LeafIndexToParentMorton)
        {
            const auto LeafIdx = LeafToParentMorton.Key;
            const auto LeafIsInRange = InOutOctree.Get_LeafNodes().Get_LeafNodes().IsValidIndex(LeafIdx);

            CK_ENSURE_IF_NOT(LeafIsInRange,
                TEXT("Cannot link leaf [{}] to its parent - the leaf store holds [{}] leaves"),
                LeafIdx, InOutOctree.Get_LeafNodes().Get_LeafNodes().Num())
            {}

            if (NOT LeafIsInRange)
            { continue; }

            const auto ParentIndex = Get_NodeIndexFromMorton(
                InOutOctree, LeafParentLayerIndex, LeafToParentMorton.Value);

            CK_ENSURE_IF_NOT(ParentIndex != INDEX_NONE,
                TEXT("Leaf [{}] names parent Morton code [{}], which no layer-1 node carries"),
                LeafIdx, LeafToParentMorton.Value)
            {}

            // The leaf keeps an invalid parent link rather than a wrong one; queries that need the parent
            // reject it loudly instead of resolving against an unrelated node.
            if (ParentIndex == INDEX_NONE)
            { continue; }

            InOutOctree.Get_LeafNodes()
                .Get_LeafNode(LeafIdx)
                .Set_Parent(FNodeAddress::Make(LeafParentLayerIndex, ParentIndex));
        }
    }

    auto
        Stage_BuildNeighbourLinks(
            FOctree& InOutOctree,
            FRasterizeScratch& InOutScratch,
            LayerIndex InLayerIndex)
        -> FRasterizeStageResult
    {
        auto Result = FRasterizeStageResult{};

        const auto LayerCount = InOutOctree.Get_LayerCount();
        const auto LayerIsInRange = InLayerIndex < LayerCount;

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("Cannot build neighbour links for VoxelNav layer [{}] - the octree has [{}] layers"),
            static_cast<int32>(InLayerIndex), LayerCount)
        {}

        if (NOT LayerIsInRange)
        { return Result; }

        // The topmost layer a climb may reach. An int32 cursor, because the upstream loop counted down
        // through a uint8 and relied on wraparound to terminate.
        const auto TopClimbableLayer = LayerCount - 2;

        for (auto NodeIdx = 0; NodeIdx < InOutOctree.Get_Layer(InLayerIndex).Get_NodeCount(); ++NodeIdx)
        {
            const auto NodeMortonCode = InOutOctree.Get_Layer(InLayerIndex).Get_Node(NodeIdx).Get_MortonCode();

            for (auto Direction = NeighbourDirection{0}; Direction < 6; ++Direction)
            {
                auto CurrentLayerIndex = static_cast<int32>(InLayerIndex);
                auto CurrentNodeIndex = NodeIdx;

                for (;;)
                {
                    const auto Search = TryFind_NeighbourInDirection(
                        InOutOctree,
                        static_cast<LayerIndex>(CurrentLayerIndex),
                        CurrentNodeIndex,
                        Direction);

                    if (Search._Resolved)
                    {
                        InOutOctree.Get_Layer(InLayerIndex)
                            .Get_Nodes()[NodeIdx]
                            .Set_Neighbour(Direction, Search._NeighbourAddress);
                        break;
                    }

                    if (CurrentLayerIndex >= TopClimbableLayer)
                    { break; }

                    const auto ParentAddress = InOutOctree
                        .Get_Layer(static_cast<LayerIndex>(CurrentLayerIndex))
                        .Get_Node(CurrentNodeIndex)
                        .Get_Parent();

                    if (ParentAddress.Get_IsValid())
                    {
                        CurrentNodeIndex = ParentAddress.Get_NodeIndex();
                        CurrentLayerIndex = ParentAddress.Get_LayerIndex();
                        continue;
                    }

                    ++CurrentLayerIndex;

                    const auto ParentIndexFromMorton = Get_NodeIndexFromMorton(
                        InOutOctree,
                        static_cast<LayerIndex>(CurrentLayerIndex),
                        Get_ParentMorton(NodeMortonCode));

                    CK_ENSURE_IF_NOT(ParentIndexFromMorton != INDEX_NONE,
                        TEXT("Node [{}] on VoxelNav layer [{}] has no parent link and no layer-[{}] node "
                             "carries its parent Morton code - the neighbour climb cannot continue"),
                        NodeIdx, static_cast<int32>(InLayerIndex), CurrentLayerIndex)
                    {}

                    if (ParentIndexFromMorton == INDEX_NONE)
                    { break; }

                    CurrentNodeIndex = ParentIndexFromMorton;
                }
            }
        }

        Result._StageComplete = true;

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryFind_NeighbourInDirection(
            const FOctree& InOctree,
            LayerIndex InLayerIndex,
            NodeIndex InNodeIndex,
            NeighbourDirection InDirection)
        -> FNeighbourSearchResult
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_TryFind_NeighbourInDirection);

        auto Result = FNeighbourSearchResult{};

        const auto LayerIsInRange = InLayerIndex < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("Layer [{}] is outside the VoxelNav octree's [{}] layers"),
            static_cast<int32>(InLayerIndex), InOctree.Get_LayerCount())
        {}

        if (NOT LayerIsInRange)
        { return Result; }

        const auto& Layer = InOctree.Get_Layer(InLayerIndex);
        const auto& LayerNodes = Layer.Get_Nodes();

        const auto NodeIsInRange = LayerNodes.IsValidIndex(InNodeIndex);

        CK_ENSURE_IF_NOT(NodeIsInRange,
            TEXT("Node [{}] is outside VoxelNav layer [{}]'s [{}] nodes"),
            InNodeIndex, static_cast<int32>(InLayerIndex), LayerNodes.Num())
        {}

        if (NOT NodeIsInRange)
        { return Result; }

        const auto DirectionIsInRange = InDirection < 6;

        CK_ENSURE_IF_NOT(DirectionIsInRange,
            TEXT("Neighbour direction [{}] is outside the six orthogonal faces"),
            static_cast<int32>(InDirection))
        {}

        if (NOT DirectionIsInRange)
        { return Result; }

        const auto& TargetNode = LayerNodes[InNodeIndex];
        const auto TargetMortonCode = TargetNode.Get_MortonCode();

        // A lattice coordinate is bounded by the layer's EDGE node count. Testing it against the cube
        // capacity (edge^3) is edge^2 too permissive: an out-of-lattice coordinate survives, encodes to a
        // Morton code the layer never holds, fails the scan, and silently degrades into a climb to the
        // parent - which then hands back a neighbour on the wrong side of the volume's face.
        const auto MaxCoordinate = Layer.Get_EdgeNodeCount();

        const auto NeighbourCoords = Get_CoordsFromMorton(TargetMortonCode) + GNeighbourDirections[InDirection];

        if (NeighbourCoords.X < 0 || NeighbourCoords.X >= MaxCoordinate ||
            NeighbourCoords.Y < 0 || NeighbourCoords.Y >= MaxCoordinate ||
            NeighbourCoords.Z < 0 || NeighbourCoords.Z >= MaxCoordinate)
        {
            Result._Resolved = true;
            return Result;
        }

        const auto NeighbourMortonCode = Get_MortonFromCoords(NeighbourCoords);

        auto StopIndex = LayerNodes.Num();
        auto Increment = 1;

        if (NeighbourMortonCode < TargetMortonCode)
        {
            Increment = -1;
            StopIndex = -1;
        }

        for (auto Cursor = InNodeIndex + Increment; Cursor != StopIndex; Cursor += Increment)
        {
            if (NOT LayerNodes.IsValidIndex(Cursor))
            { break; }

            const auto& CandidateNode = LayerNodes[Cursor];
            const auto CandidateMortonCode = CandidateNode.Get_MortonCode();

            if (CandidateMortonCode == NeighbourMortonCode)
            {
                const auto& LeafNodes = InOctree.Get_LeafNodes();
                const auto CandidateLeafIndex = CandidateNode.Get_FirstChild().Get_NodeIndex();

                const auto IsSolidLeaf =
                    InLayerIndex == 0 &&
                    CandidateNode.Get_HasChildren() &&
                    LeafNodes.Get_LeafNodes().IsValidIndex(CandidateLeafIndex) &&
                    LeafNodes.Get_LeafNode(CandidateLeafIndex).Get_IsFullyOccluded();

                // A fully occluded leaf is not a neighbour - linking to it would open a path through solid
                // space.
                if (IsSolidLeaf)
                {
                    Result._Resolved = true;
                    return Result;
                }

                Result._Resolved = true;
                Result._NeighbourAddress = FNodeAddress::Make(InLayerIndex, Cursor);

                return Result;
            }

            // The layer is Morton-ordered, so passing the code we want proves it is not on this layer.
            if ((Increment == -1 && CandidateMortonCode < NeighbourMortonCode) ||
                (Increment == 1 && CandidateMortonCode > NeighbourMortonCode))
            { return Result; }
        }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
