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
        InOutScratch._LeafIndexToParentMorton.Reset();
        InOutScratch._Cursor = 0;
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
