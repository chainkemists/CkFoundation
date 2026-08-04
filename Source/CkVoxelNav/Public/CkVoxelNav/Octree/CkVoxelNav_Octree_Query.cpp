#include "CkVoxelNav_Octree_Query.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Morton.h"

#include <Algo/BinarySearch.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_query
{
    // A leaf node is 4x4x4 sub-nodes.
    constexpr auto LeafSubNodeEdgeCount = 4;

    // Absorbs floating-point error at the volume's faces when testing containment.
    constexpr auto BoundsToleranceUu = 1.0f;

    /* Morton child ordering, per face. The four children of a node that touch the face pointing in
       direction D, in the order their Morton codes fall:

            Z
            ^          5 --- 7
            |        / |   / |
            |       4 --- 6  |
            |  X    |  1 -|- 3
            | /     | /   | /
            |/      0 --- 2
            +-------------------> Y                                                                     */
    constexpr ck::voxelnav::NodeIndex ChildOffsetsDirections[6][4] =
    {
        {0, 4, 2, 6}, {1, 3, 5, 7}, {0, 1, 4, 5},
        {2, 3, 6, 7}, {0, 1, 2, 3}, {4, 5, 6, 7}
    };

    /* The same idea one level finer: the sixteen sub-nodes of a leaf that touch each face. */
    constexpr ck::voxelnav::SubNodeIndex LeafChildOffsetsDirections[6][16] =
    {
        {0, 2, 16, 18, 4, 6, 20, 22, 32, 34, 48, 50, 36, 38, 52, 54},
        {9, 11, 25, 27, 13, 15, 29, 31, 41, 43, 57, 59, 45, 47, 61, 63},
        {0, 1, 8, 9, 4, 5, 12, 13, 32, 33, 40, 41, 36, 37, 44, 45},
        {18, 19, 26, 27, 22, 23, 30, 31, 50, 51, 58, 59, 54, 55, 62, 63},
        {0, 1, 8, 9, 2, 3, 10, 11, 16, 17, 24, 25, 18, 19, 26, 27},
        {36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63}
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Get_LayerNodeSize(
            const FOctree& InOctree,
            LayerIndex InLayerIndex)
        -> float
    {
        if (InLayerIndex == 0)
        { return InOctree.Get_LeafNodes().Get_LeafNodeSize(); }

        const auto LayerIsInRange = InLayerIndex < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("Layer [{}] is outside the VoxelNav octree's [{}] layers"),
            static_cast<int32>(InLayerIndex), InOctree.Get_LayerCount())
        {}

        if (NOT LayerIsInRange)
        { return 0.0f; }

        return InOctree.Get_Layer(InLayerIndex).Get_NodeSize();
    }

    auto
        Get_LayerNodeExtent(
            const FOctree& InOctree,
            LayerIndex InLayerIndex)
        -> float
    {
        return Get_LayerNodeSize(InOctree, InLayerIndex) * 0.5f;
    }

    auto
        Get_LayerRatio(
            const FOctree& InOctree,
            LayerIndex InLayerIndex)
        -> float
    {
        const auto LayerCount = InOctree.Get_LayerCount();

        const auto HasLayers = LayerCount > 0;

        CK_ENSURE_IF_NOT(HasLayers,
            TEXT("Cannot compute a layer ratio on a VoxelNav octree that has no layers"))
        {}

        if (NOT HasLayers)
        { return 0.0f; }

        return static_cast<float>(InLayerIndex) / static_cast<float>(LayerCount);
    }

    auto
        Get_LayerInverseRatio(
            const FOctree& InOctree,
            LayerIndex InLayerIndex)
        -> float
    {
        return 1.0f - Get_LayerRatio(InOctree, InLayerIndex);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_LeafNodePositionFromMorton(
            const FOctree& InOctree,
            MortonCode InMortonCode)
        -> FVector
    {
        const auto& NavigationBounds = InOctree.Get_NavigationBounds();
        const auto& LeafNodes = InOctree.Get_LeafNodes();

        const auto MinCorner = NavigationBounds.GetCenter() - NavigationBounds.GetExtent();

        return MinCorner +
               Get_VectorFromMorton(InMortonCode) * LeafNodes.Get_LeafNodeSize() +
               FVector{LeafNodes.Get_LeafNodeExtent()};
    }

    auto
        Get_NodePositionFromLayerAndMorton(
            const FOctree& InOctree,
            LayerIndex InLayerIndex,
            MortonCode InMortonCode)
        -> FVector
    {
        if (InLayerIndex == 0)
        { return Get_LeafNodePositionFromMorton(InOctree, InMortonCode); }

        const auto LayerIsInRange = InLayerIndex < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("Layer [{}] is outside the VoxelNav octree's [{}] layers"),
            static_cast<int32>(InLayerIndex), InOctree.Get_LayerCount())
        {}

        if (NOT LayerIsInRange)
        { return FVector::ZeroVector; }

        const auto& Layer = InOctree.Get_Layer(InLayerIndex);
        const auto& NavigationBounds = InOctree.Get_NavigationBounds();

        const auto MinCorner = NavigationBounds.GetCenter() - NavigationBounds.GetExtent();

        return MinCorner +
               Get_VectorFromMorton(InMortonCode) * Layer.Get_NodeSize() +
               FVector{Layer.Get_NodeExtent()};
    }

    auto
        Get_NodePositionFromAddress(
            const FOctree& InOctree,
            const FNodeAddress& InAddress,
            ECk_VoxelNav_SubNodePrecision InPrecision)
        -> FVector
    {
        const auto AddressIsValid = InAddress.Get_IsValid();

        CK_ENSURE_IF_NOT(AddressIsValid,
            TEXT("Cannot resolve a position from the invalid VoxelNav node address [{}]"),
            InAddress.Get_Packed())
        {}

        if (NOT AddressIsValid)
        { return FVector::ZeroVector; }

        const auto LayerIdx = InAddress.Get_LayerIndex();
        const auto LayerIsInRange = LayerIdx < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("VoxelNav node address [{}] names layer [{}], outside the octree's [{}] layers"),
            InAddress.Get_Packed(), static_cast<int32>(LayerIdx), InOctree.Get_LayerCount())
        {}

        if (NOT LayerIsInRange)
        { return FVector::ZeroVector; }

        const auto& Layer = InOctree.Get_Layer(LayerIdx);
        const auto NodeIdx = InAddress.Get_NodeIndex();
        const auto NodeIsInRange = Layer.Get_Nodes().IsValidIndex(NodeIdx);

        CK_ENSURE_IF_NOT(NodeIsInRange,
            TEXT("VoxelNav node address [{}] names node [{}], outside layer [{}]'s [{}] nodes"),
            InAddress.Get_Packed(), NodeIdx, static_cast<int32>(LayerIdx), Layer.Get_NodeCount())
        {}

        if (NOT NodeIsInRange)
        { return FVector::ZeroVector; }

        const auto& Node = Layer.Get_Node(NodeIdx);

        if (LayerIdx != 0)
        { return Get_NodePositionFromLayerAndMorton(InOctree, LayerIdx, Node.Get_MortonCode()); }

        const auto& LeafNodes = InOctree.Get_LeafNodes();
        const auto LeafIsInRange = LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx);

        CK_ENSURE_IF_NOT(LeafIsInRange,
            TEXT("Layer-0 node [{}] has no leaf entry - the leaf store holds [{}] leaves"),
            NodeIdx, LeafNodes.Get_LeafNodes().Num())
        {}

        if (NOT LeafIsInRange)
        { return FVector::ZeroVector; }

        const auto& LeafNode = LeafNodes.Get_LeafNode(NodeIdx);
        const auto LeafParent = LeafNode.Get_Parent();
        const auto LeafParentIsValid = LeafParent.Get_IsValid();

        CK_ENSURE_IF_NOT(LeafParentIsValid,
            TEXT("Leaf node [{}] has no parent link"), NodeIdx)
        {}

        if (NOT LeafParentIsValid)
        { return FVector::ZeroVector; }

        const auto ParentLayerIsInRange = LeafParent.Get_LayerIndex() < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(ParentLayerIsInRange,
            TEXT("Leaf node [{}] names parent layer [{}], outside the octree's [{}] layers"),
            NodeIdx, static_cast<int32>(LeafParent.Get_LayerIndex()), InOctree.Get_LayerCount())
        {}

        if (NOT ParentLayerIsInRange)
        { return FVector::ZeroVector; }

        const auto ParentNodeIsInRange = InOctree
            .Get_Layer(LeafParent.Get_LayerIndex())
            .Get_Nodes()
            .IsValidIndex(LeafParent.Get_NodeIndex());

        CK_ENSURE_IF_NOT(ParentNodeIsInRange,
            TEXT("Leaf node [{}] names parent node [{}], outside its layer"),
            NodeIdx, LeafParent.Get_NodeIndex())
        {}

        if (NOT ParentNodeIsInRange)
        { return FVector::ZeroVector; }

        const auto NodePosition = Get_LeafNodePositionFromMorton(InOctree, Node.Get_MortonCode());

        if (LeafNode.Get_IsFullyFree() || InPrecision == ECk_VoxelNav_SubNodePrecision::NodeCenter)
        { return NodePosition; }

        return NodePosition -
               FVector{LeafNodes.Get_LeafNodeExtent()} +
               Get_VectorFromMorton(InAddress.Get_SubNodeIndex()) * LeafNodes.Get_LeafSubNodeSize() +
               FVector{LeafNodes.Get_LeafSubNodeExtent()};
    }

    auto
        Get_NodeExtentFromAddress(
            const FOctree& InOctree,
            const FNodeAddress& InAddress)
        -> float
    {
        const auto LayerIdx = InAddress.Get_LayerIndex();
        const auto LayerIsInRange = InAddress.Get_IsValid() && LayerIdx < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("VoxelNav node address [{}] does not name a layer of this octree"),
            InAddress.Get_Packed())
        {}

        if (NOT LayerIsInRange)
        { return 0.0f; }

        const auto NodeIdx = InAddress.Get_NodeIndex();

        if (LayerIdx != 0)
        {
            const auto& Layer = InOctree.Get_Layer(LayerIdx);
            const auto NodeIsInRange = Layer.Get_Nodes().IsValidIndex(NodeIdx);

            CK_ENSURE_IF_NOT(NodeIsInRange,
                TEXT("VoxelNav node address [{}] names node [{}], outside its layer"),
                InAddress.Get_Packed(), NodeIdx)
            {}

            if (NOT NodeIsInRange)
            { return 0.0f; }

            return Layer.Get_NodeExtent();
        }

        const auto& LeafNodes = InOctree.Get_LeafNodes();
        const auto LeafIsInRange = LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx);

        CK_ENSURE_IF_NOT(LeafIsInRange,
            TEXT("Layer-0 node [{}] has no leaf entry - the leaf store holds [{}] leaves"),
            NodeIdx, LeafNodes.Get_LeafNodes().Num())
        {}

        if (NOT LeafIsInRange)
        { return 0.0f; }

        return LeafNodes.Get_LeafNode(NodeIdx).Get_IsFullyFree()
            ? LeafNodes.Get_LeafNodeExtent()
            : LeafNodes.Get_LeafSubNodeExtent();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_NodeIndexFromMorton(
            const FOctree& InOctree,
            LayerIndex InLayerIndex,
            MortonCode InMortonCode)
        -> NodeIndex
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Get_NodeIndexFromMorton);

        const auto LayerIsInRange = InLayerIndex < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("Layer [{}] is outside the VoxelNav octree's [{}] layers"),
            static_cast<int32>(InLayerIndex), InOctree.Get_LayerCount())
        {}

        if (NOT LayerIsInRange)
        { return INDEX_NONE; }

        return Algo::BinarySearch(InOctree.Get_Layer(InLayerIndex).Get_Nodes(), FNode{InMortonCode});
    }

    auto
        TryGet_NodeAddressFromMorton(
            const FOctree& InOctree,
            MortonCode InMortonCode,
            LayerIndex InLayerIndex)
        -> FNodeAddress
    {
        if (InLayerIndex >= InOctree.Get_LayerCount())
        { return FNodeAddress{}; }

        const auto NodeIdx = Get_NodeIndexFromMorton(InOctree, InLayerIndex, InMortonCode);

        if (NodeIdx == INDEX_NONE)
        { return FNodeAddress{}; }

        return FNodeAddress::Make(InLayerIndex, NodeIdx);
    }

    auto
        TryGet_NodeFromAddress(
            const FOctree& InOctree,
            const FNodeAddress& InAddress)
        -> const FNode*
    {
        if (NOT InAddress.Get_IsValid())
        { return nullptr; }

        const auto LayerIdx = InAddress.Get_LayerIndex();

        if (LayerIdx >= InOctree.Get_LayerCount())
        { return nullptr; }

        const auto& Layer = InOctree.Get_Layer(LayerIdx);

        if (NOT Layer.Get_Nodes().IsValidIndex(InAddress.Get_NodeIndex()))
        { return nullptr; }

        return &Layer.Get_Node(InAddress.Get_NodeIndex());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryGet_NodeAddressFromPosition(
            const FOctree& InOctree,
            const FVector& InPosition,
            LayerIndex InMinLayerIndex)
        -> FCk_VoxelNav_AddressLookupResult
    {
        using namespace ck_voxelnav_octree_query;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_TryGet_NodeAddressFromPosition);

        const auto LayerCount = InOctree.Get_LayerCount();

        if (LayerCount == 0)
        {
            auto NoLayersResult = FCk_VoxelNav_AddressLookupResult{};
            NoLayersResult._Outcome = ECk_VoxelNav_AddressLookup::NoLayers;
            return NoLayersResult;
        }

        const auto& NavigationBounds = InOctree.Get_NavigationBounds();

        if (NOT NavigationBounds.ExpandBy(BoundsToleranceUu).IsInside(InPosition))
        {
            auto OutOfBoundsResult = FCk_VoxelNav_AddressLookupResult{};
            OutOfBoundsResult._Outcome = ECk_VoxelNav_AddressLookup::OutOfBounds;
            return OutOfBoundsResult;
        }

        const auto MinCorner = NavigationBounds.GetCenter() - NavigationBounds.GetExtent();
        const auto LocalPosition = InPosition - MinCorner;

        for (auto LayerCursor = LayerCount - 1; LayerCursor >= static_cast<int32>(InMinLayerIndex); --LayerCursor)
        {
            const auto LayerIdx = static_cast<LayerIndex>(LayerCursor);
            const auto& Layer = InOctree.Get_Layer(LayerIdx);
            const auto LayerNodeSize = Get_LayerNodeSize(InOctree, LayerIdx);

            const auto LayerIsSized = LayerNodeSize > 0.0f;

            CK_ENSURE_IF_NOT(LayerIsSized,
                TEXT("VoxelNav layer [{}] has a node size of [{}]"),
                static_cast<int32>(LayerIdx), LayerNodeSize)
            {}

            if (NOT LayerIsSized)
            { break; }

            // The bounds test above admits positions up to a tolerance outside the volume, so the lattice
            // coordinate is clamped to the layer's edge cells rather than escaping the lattice.
            const auto MaxCoord = Layer.Get_EdgeNodeCount() - 1;
            const auto LayerCoords = FIntVector
            {
                FMath::Clamp(FMath::FloorToInt32(LocalPosition.X / LayerNodeSize), 0, MaxCoord),
                FMath::Clamp(FMath::FloorToInt32(LocalPosition.Y / LayerNodeSize), 0, MaxCoord),
                FMath::Clamp(FMath::FloorToInt32(LocalPosition.Z / LayerNodeSize), 0, MaxCoord)
            };

            const auto LayerMortonCode = Get_MortonFromCoords(LayerCoords);
            const auto NodeIdx = Get_NodeIndexFromMorton(InOctree, LayerIdx, LayerMortonCode);

            if (NodeIdx == INDEX_NONE)
            {
                auto FreeSpaceResult = FCk_VoxelNav_AddressLookupResult{};
                FreeSpaceResult._Outcome = ECk_VoxelNav_AddressLookup::FreeSpaceAtLayer;
                FreeSpaceResult._Layer = LayerIdx;
                FreeSpaceResult._Morton = LayerMortonCode;
                return FreeSpaceResult;
            }

            const auto& Node = Layer.Get_Node(NodeIdx);

            if (NOT Node.Get_HasChildren())
            {
                auto FoundResult = FCk_VoxelNav_AddressLookupResult{};
                FoundResult._Outcome = ECk_VoxelNav_AddressLookup::Found;
                FoundResult._Address = FNodeAddress::Make(LayerIdx, NodeIdx);
                FoundResult._Layer = LayerIdx;
                FoundResult._Morton = Node.Get_MortonCode();
                return FoundResult;
            }

            if (LayerIdx != 0)
            { continue; }

            const auto& LeafNodes = InOctree.Get_LeafNodes();
            const auto LeafIsInRange = LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx);

            CK_ENSURE_IF_NOT(LeafIsInRange,
                TEXT("Layer-0 node [{}] has no leaf entry - the leaf store holds [{}] leaves"),
                NodeIdx, LeafNodes.Get_LeafNodes().Num())
            {}

            if (NOT LeafIsInRange)
            { break; }

            const auto& LeafNode = LeafNodes.Get_LeafNode(NodeIdx);
            const auto LeafOrigin = Get_LeafNodePositionFromMorton(InOctree, Node.Get_MortonCode()) -
                                    FVector{LeafNodes.Get_LeafNodeExtent()};
            const auto SubNodeSize = LeafNodes.Get_LeafSubNodeSize();
            const auto LocalInLeaf = InPosition - LeafOrigin;

            const auto SubNodeCoords = FIntVector
            {
                FMath::Clamp(FMath::FloorToInt32(LocalInLeaf.X / SubNodeSize), 0, LeafSubNodeEdgeCount - 1),
                FMath::Clamp(FMath::FloorToInt32(LocalInLeaf.Y / SubNodeSize), 0, LeafSubNodeEdgeCount - 1),
                FMath::Clamp(FMath::FloorToInt32(LocalInLeaf.Z / SubNodeSize), 0, LeafSubNodeEdgeCount - 1)
            };

            const auto SubNodeIdx = static_cast<SubNodeIndex>(Get_MortonFromCoords(SubNodeCoords));

            if (NOT LeafNode.Get_IsSubNodeOccluded(SubNodeIdx))
            {
                auto FoundResult = FCk_VoxelNav_AddressLookupResult{};
                FoundResult._Outcome = ECk_VoxelNav_AddressLookup::Found;
                FoundResult._Address = FNodeAddress::Make(0, NodeIdx, SubNodeIdx);
                FoundResult._Morton = Node.Get_MortonCode();
                return FoundResult;
            }

            const auto SubNodeExtent = LeafNodes.Get_LeafSubNodeExtent();

            auto BestDistanceSq = TNumericLimits<double>::Max();
            auto BestSubNodeIdx = SubNodeIndex{0};
            auto FoundFreeSubNode = false;

            for (auto Candidate = 0; Candidate <= FNodeAddress::MaxSubNodeIndex; ++Candidate)
            {
                const auto CandidateIdx = static_cast<SubNodeIndex>(Candidate);

                if (LeafNode.Get_IsSubNodeOccluded(CandidateIdx))
                { continue; }

                const auto CandidateCenter = LeafOrigin +
                                             Get_VectorFromMorton(CandidateIdx) * SubNodeSize +
                                             FVector{SubNodeExtent};

                const auto DistanceSq = FVector::DistSquared(CandidateCenter, InPosition);

                if (DistanceSq >= BestDistanceSq)
                { continue; }

                BestDistanceSq = DistanceSq;
                BestSubNodeIdx = CandidateIdx;
                FoundFreeSubNode = true;
            }

            if (FoundFreeSubNode)
            {
                auto FoundResult = FCk_VoxelNav_AddressLookupResult{};
                FoundResult._Outcome = ECk_VoxelNav_AddressLookup::Found;
                FoundResult._Address = FNodeAddress::Make(0, NodeIdx, BestSubNodeIdx);
                FoundResult._Morton = Node.Get_MortonCode();
                return FoundResult;
            }

            break;
        }

        return TryGet_NearestFreeNodeAddress(InOctree, InPosition, InMinLayerIndex);
    }

    auto
        TryGet_NearestFreeNodeAddress(
            const FOctree& InOctree,
            const FVector& InPosition,
            LayerIndex InMinLayerIndex)
        -> FCk_VoxelNav_AddressLookupResult
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_TryGet_NearestFreeNodeAddress);

        const auto LayerCount = InOctree.Get_LayerCount();

        if (LayerCount == 0)
        {
            auto NoLayersResult = FCk_VoxelNav_AddressLookupResult{};
            NoLayersResult._Outcome = ECk_VoxelNav_AddressLookup::NoLayers;
            return NoLayersResult;
        }

        auto BestDistanceSq = TNumericLimits<double>::Max();
        auto BestAddress = FNodeAddress{};
        auto FoundAny = false;

        for (auto LayerCursor = static_cast<int32>(InMinLayerIndex); LayerCursor < LayerCount; ++LayerCursor)
        {
            const auto LayerIdx = static_cast<LayerIndex>(LayerCursor);
            const auto& Layer = InOctree.Get_Layer(LayerIdx);
            const auto& LeafNodes = InOctree.Get_LeafNodes();

            for (auto NodeIdx = 0; NodeIdx < Layer.Get_NodeCount(); ++NodeIdx)
            {
                const auto& Node = Layer.Get_Node(NodeIdx);

                if (LayerIdx != 0)
                {
                    if (Node.Get_HasChildren())
                    { continue; }

                    const auto NodePosition = Get_NodePositionFromLayerAndMorton(InOctree, LayerIdx, Node.Get_MortonCode());
                    const auto DistanceSq = FVector::DistSquared(NodePosition, InPosition);

                    if (DistanceSq >= BestDistanceSq)
                    { continue; }

                    BestDistanceSq = DistanceSq;
                    BestAddress = FNodeAddress::Make(LayerIdx, NodeIdx);
                    FoundAny = true;

                    continue;
                }

                const auto LeafIsInRange = LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx);

                CK_ENSURE_IF_NOT(LeafIsInRange,
                    TEXT("Layer-0 node [{}] has no leaf entry - the leaf store holds [{}] leaves"),
                    NodeIdx, LeafNodes.Get_LeafNodes().Num())
                {}

                if (NOT LeafIsInRange)
                { continue; }

                const auto& LeafNode = LeafNodes.Get_LeafNode(NodeIdx);

                for (auto Candidate = 0; Candidate <= FNodeAddress::MaxSubNodeIndex; ++Candidate)
                {
                    const auto CandidateIdx = static_cast<SubNodeIndex>(Candidate);

                    if (LeafNode.Get_IsSubNodeOccluded(CandidateIdx))
                    { continue; }

                    const auto CandidateAddress = FNodeAddress::Make(0, NodeIdx, CandidateIdx);
                    const auto CandidatePosition = Get_NodePositionFromAddress(
                        InOctree, CandidateAddress, ECk_VoxelNav_SubNodePrecision::SubNodeCenter);

                    const auto DistanceSq = FVector::DistSquared(CandidatePosition, InPosition);

                    if (DistanceSq >= BestDistanceSq)
                    { continue; }

                    BestDistanceSq = DistanceSq;
                    BestAddress = CandidateAddress;
                    FoundAny = true;
                }
            }
        }

        auto Result = FCk_VoxelNav_AddressLookupResult{};

        if (NOT FoundAny)
        {
            Result._Outcome = ECk_VoxelNav_AddressLookup::OutOfBounds;
            return Result;
        }

        Result._Outcome = ECk_VoxelNav_AddressLookup::Found;
        Result._Address = BestAddress;
        Result._Layer = BestAddress.Get_LayerIndex();

        return Result;
    }

    auto
        Get_MinLayerIndexForAgentRadius(
            const FOctree& InOctree,
            float InAgentRadiusUu)
        -> LayerIndex
    {
        const auto LayerCount = InOctree.Get_LayerCount();

        if (NOT InOctree.Get_IsValid() || LayerCount == 0)
        { return 0; }

        const auto AgentDiameter = InAgentRadiusUu * 2.0f;

        for (auto LayerCursor = 0; LayerCursor < LayerCount; ++LayerCursor)
        {
            const auto LayerIdx = static_cast<LayerIndex>(LayerCursor);
            const auto LayerNodeSize = Get_LayerNodeSize(InOctree, LayerIdx);

            const auto LayerIsSized = NOT FMath::IsNearlyZero(LayerNodeSize);

            CK_ENSURE_IF_NOT(LayerIsSized,
                TEXT("VoxelNav layer [{}] has a node size of [{}]"), LayerCursor, LayerNodeSize)
            {}

            if (NOT LayerIsSized)
            { continue; }

            if (LayerNodeSize >= AgentDiameter)
            { return LayerIdx; }
        }

        return static_cast<LayerIndex>(LayerCount - 1);
    }

    auto
        Get_IsNodeInBounds(
            const FVector& InNodePosition,
            float InNodeExtent,
            const FBox& InBounds)
        -> bool
    {
        return FBox::BuildAABB(InNodePosition, FVector{InNodeExtent}).Intersect(InBounds);
    }

    auto
        Get_IsPositionFree(
            const FOctree& InOctree,
            const FVector& InPosition)
        -> bool
    {
        using namespace ck_voxelnav_octree_query;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Get_IsPositionFree);

        const auto LayerCount = InOctree.Get_LayerCount();

        if (LayerCount == 0)
        { return false; }

        const auto& NavigationBounds = InOctree.Get_NavigationBounds();

        if (NOT NavigationBounds.ExpandBy(BoundsToleranceUu).IsInside(InPosition))
        { return false; }

        const auto MinCorner = NavigationBounds.GetCenter() - NavigationBounds.GetExtent();
        const auto LocalPosition = InPosition - MinCorner;

        for (auto LayerCursor = LayerCount - 1; LayerCursor >= 0; --LayerCursor)
        {
            const auto LayerIdx = static_cast<LayerIndex>(LayerCursor);
            const auto& Layer = InOctree.Get_Layer(LayerIdx);
            const auto LayerNodeSize = Get_LayerNodeSize(InOctree, LayerIdx);

            const auto LayerIsSized = LayerNodeSize > 0.0f;

            CK_ENSURE_IF_NOT(LayerIsSized,
                TEXT("VoxelNav layer [{}] has a node size of [{}]"),
                static_cast<int32>(LayerIdx), LayerNodeSize)
            {}

            if (NOT LayerIsSized)
            { return false; }

            const auto MaxCoord = Layer.Get_EdgeNodeCount() - 1;
            const auto LayerCoords = FIntVector
            {
                FMath::Clamp(FMath::FloorToInt32(LocalPosition.X / LayerNodeSize), 0, MaxCoord),
                FMath::Clamp(FMath::FloorToInt32(LocalPosition.Y / LayerNodeSize), 0, MaxCoord),
                FMath::Clamp(FMath::FloorToInt32(LocalPosition.Z / LayerNodeSize), 0, MaxCoord)
            };

            const auto NodeIdx = Get_NodeIndexFromMorton(InOctree, LayerIdx, Get_MortonFromCoords(LayerCoords));

            // No node at this layer means the cell was never subdivided, which the octree encodes as
            // "nothing in this whole subtree" - the coarsest possible free answer.
            if (NodeIdx == INDEX_NONE)
            { return true; }

            const auto& Node = Layer.Get_Node(NodeIdx);

            if (NOT Node.Get_HasChildren())
            { return true; }

            if (LayerIdx != 0)
            { continue; }

            const auto& LeafNodes = InOctree.Get_LeafNodes();
            const auto LeafIsInRange = LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx);

            CK_ENSURE_IF_NOT(LeafIsInRange,
                TEXT("Layer-0 node [{}] has no leaf entry - the leaf store holds [{}] leaves"),
                NodeIdx, LeafNodes.Get_LeafNodes().Num())
            {}

            if (NOT LeafIsInRange)
            { return false; }

            const auto LeafOrigin = Get_LeafNodePositionFromMorton(InOctree, Node.Get_MortonCode()) -
                                    FVector{LeafNodes.Get_LeafNodeExtent()};
            const auto SubNodeSize = LeafNodes.Get_LeafSubNodeSize();
            const auto LocalInLeaf = InPosition - LeafOrigin;

            const auto SubNodeCoords = FIntVector
            {
                FMath::Clamp(FMath::FloorToInt32(LocalInLeaf.X / SubNodeSize), 0, LeafSubNodeEdgeCount - 1),
                FMath::Clamp(FMath::FloorToInt32(LocalInLeaf.Y / SubNodeSize), 0, LeafSubNodeEdgeCount - 1),
                FMath::Clamp(FMath::FloorToInt32(LocalInLeaf.Z / SubNodeSize), 0, LeafSubNodeEdgeCount - 1)
            };

            const auto SubNodeIdx = static_cast<SubNodeIndex>(Get_MortonFromCoords(SubNodeCoords));

            return NOT LeafNodes.Get_LeafNode(NodeIdx).Get_IsSubNodeOccluded(SubNodeIdx);
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_NodeNeighbours(
            const FOctree& InOctree,
            const FNodeAddress& InAddress,
            TArray<FNodeAddress>& OutNeighbours)
        -> void
    {
        using namespace ck_voxelnav_octree_query;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Get_NodeNeighbours);

        const auto* Node = TryGet_NodeFromAddress(InOctree, InAddress);

        if (ck::Is_NOT_Valid(Node, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        if (InAddress.Get_LayerIndex() == 0 && Node->Get_HasChildren())
        {
            Get_LeafNeighbours(InOctree, InAddress, OutNeighbours);
            return;
        }

        // Six faces, and a face abutting a subdivided neighbour contributes up to sixteen leaf sub-nodes;
        // the typical fan-out is far below that worst case.
        constexpr auto TypicalNeighbourCount = 24;
        OutNeighbours.Reserve(OutNeighbours.Num() + TypicalNeighbourCount);

        for (auto Direction = NeighbourDirection{0}; Direction < 6; ++Direction)
        {
            const auto NeighbourAddress = Node->Get_Neighbour(Direction);

            if (NOT NeighbourAddress.Get_IsValid())
            { continue; }

            const auto* Neighbour = TryGet_NodeFromAddress(InOctree, NeighbourAddress);

            if (ck::Is_NOT_Valid(Neighbour, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            if (NOT Neighbour->Get_HasChildren())
            {
                OutNeighbours.Add(NeighbourAddress);
                continue;
            }

            auto DescentSet = TArray<FNodeAddress>{};
            DescentSet.Push(NeighbourAddress);

            while (DescentSet.Num() > 0)
            {
                const auto ThisAddress = DescentSet.Pop();
                const auto* ThisNode = TryGet_NodeFromAddress(InOctree, ThisAddress);

                if (ck::Is_NOT_Valid(ThisNode, ck::IsValid_Policy_NullptrOnly{}))
                { continue; }

                if (NOT ThisNode->Get_HasChildren())
                {
                    OutNeighbours.Add(ThisAddress);
                    continue;
                }

                const auto FirstChild = ThisNode->Get_FirstChild();

                if (ThisAddress.Get_LayerIndex() > 0)
                {
                    for (const auto ChildOffset : ChildOffsetsDirections[Direction])
                    {
                        const auto ChildAddress = FNodeAddress::Make(
                            FirstChild.Get_LayerIndex(),
                            FirstChild.Get_NodeIndex() + ChildOffset);

                        const auto* ChildNode = TryGet_NodeFromAddress(InOctree, ChildAddress);

                        if (ck::Is_NOT_Valid(ChildNode, ck::IsValid_Policy_NullptrOnly{}))
                        { continue; }

                        if (ChildNode->Get_HasChildren())
                        { DescentSet.Emplace(ChildAddress); }
                        else
                        { OutNeighbours.Emplace(ChildAddress); }
                    }

                    continue;
                }

                const auto& LeafNodes = InOctree.Get_LeafNodes();

                if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(FirstChild.Get_NodeIndex()))
                { continue; }

                const auto& LeafNode = LeafNodes.Get_LeafNode(FirstChild.Get_NodeIndex());

                for (const auto LeafSubNodeIdx : LeafChildOffsetsDirections[Direction])
                {
                    if (LeafNode.Get_IsSubNodeOccluded(LeafSubNodeIdx))
                    { continue; }

                    OutNeighbours.Emplace(FNodeAddress::Make(0, FirstChild.Get_NodeIndex(), LeafSubNodeIdx));
                }
            }
        }
    }

    auto
        Get_LeafNeighbours(
            const FOctree& InOctree,
            const FNodeAddress& InLeafAddress,
            TArray<FNodeAddress>& OutNeighbours)
        -> void
    {
        using namespace ck_voxelnav_octree_query;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Get_LeafNeighbours);

        const auto* Node = TryGet_NodeFromAddress(InOctree, InLeafAddress);

        if (ck::Is_NOT_Valid(Node, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto FirstChild = Node->Get_FirstChild();

        if (NOT FirstChild.Get_IsValid())
        { return; }

        const auto& LeafNodes = InOctree.Get_LeafNodes();

        if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(FirstChild.Get_NodeIndex()))
        { return; }

        const auto& Leaf = LeafNodes.Get_LeafNode(FirstChild.Get_NodeIndex());
        const auto SubNodeCoords = Get_CoordsFromMorton(InLeafAddress.Get_SubNodeIndex());

        for (auto Direction = NeighbourDirection{0}; Direction < 6; ++Direction)
        {
            auto NeighbourCoords = SubNodeCoords + GNeighbourDirections[Direction];

            const auto IsInsideThisLeaf =
                NeighbourCoords.X >= 0 && NeighbourCoords.X < LeafSubNodeEdgeCount &&
                NeighbourCoords.Y >= 0 && NeighbourCoords.Y < LeafSubNodeEdgeCount &&
                NeighbourCoords.Z >= 0 && NeighbourCoords.Z < LeafSubNodeEdgeCount;

            if (IsInsideThisLeaf)
            {
                const auto SubNodeIdx = static_cast<SubNodeIndex>(Get_MortonFromCoords(NeighbourCoords));

                if (Leaf.Get_IsSubNodeOccluded(SubNodeIdx))
                { continue; }

                OutNeighbours.Emplace(FNodeAddress::Make(0, InLeafAddress.Get_NodeIndex(), SubNodeIdx));
                continue;
            }

            const auto NeighbourAddress = Node->Get_Neighbour(Direction);
            const auto* NeighbourNode = TryGet_NodeFromAddress(InOctree, NeighbourAddress);

            if (ck::Is_NOT_Valid(NeighbourNode, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            const auto NeighbourFirstChild = NeighbourNode->Get_FirstChild();

            if (NOT NeighbourFirstChild.Get_IsValid())
            {
                OutNeighbours.Add(NeighbourAddress);
                continue;
            }

            if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(NeighbourFirstChild.Get_NodeIndex()))
            { continue; }

            const auto& NeighbourLeaf = LeafNodes.Get_LeafNode(NeighbourFirstChild.Get_NodeIndex());

            if (NeighbourLeaf.Get_IsFullyOccluded())
            { continue; }

            // The step left this leaf on exactly one axis; wrap that axis onto the abutting leaf's face.
            if (NeighbourCoords.X < 0)                          { NeighbourCoords.X = LeafSubNodeEdgeCount - 1; }
            else if (NeighbourCoords.X >= LeafSubNodeEdgeCount) { NeighbourCoords.X = 0; }
            else if (NeighbourCoords.Y < 0)                     { NeighbourCoords.Y = LeafSubNodeEdgeCount - 1; }
            else if (NeighbourCoords.Y >= LeafSubNodeEdgeCount) { NeighbourCoords.Y = 0; }
            else if (NeighbourCoords.Z < 0)                     { NeighbourCoords.Z = LeafSubNodeEdgeCount - 1; }
            else if (NeighbourCoords.Z >= LeafSubNodeEdgeCount) { NeighbourCoords.Z = 0; }

            const auto SubNodeIdx = static_cast<SubNodeIndex>(Get_MortonFromCoords(NeighbourCoords));

            if (NeighbourLeaf.Get_IsSubNodeOccluded(SubNodeIdx))
            { continue; }

            OutNeighbours.Emplace(FNodeAddress::Make(0, NeighbourFirstChild.Get_NodeIndex(), SubNodeIdx));
        }
    }

    auto
        Get_FreeNodesUnderAddress(
            const FOctree& InOctree,
            const FNodeAddress& InAddress,
            TArray<FNodeAddress>& OutFreeNodes)
        -> void
    {
        const auto* Node = TryGet_NodeFromAddress(InOctree, InAddress);
        const auto NodeResolves = ck::IsValid(Node, ck::IsValid_Policy_NullptrOnly{});

        CK_ENSURE_IF_NOT(NodeResolves,
            TEXT("VoxelNav node address [{}] names no node of this octree"), InAddress.Get_Packed())
        {}

        if (NOT NodeResolves)
        { return; }

        const auto LayerIdx = InAddress.Get_LayerIndex();
        const auto NodeIdx = InAddress.Get_NodeIndex();

        if (LayerIdx != 0)
        {
            if (NOT Node->Get_HasChildren())
            {
                OutFreeNodes.Emplace(InAddress);
                return;
            }

            const auto FirstChild = Node->Get_FirstChild();

            // Descend by the child's NODE INDEX: an octree node's eight children are contiguous, and the
            // child's Morton code is a lattice coordinate, not an index into its layer.
            for (auto ChildOffset = 0; ChildOffset < 8; ++ChildOffset)
            {
                Get_FreeNodesUnderAddress(
                    InOctree,
                    FNodeAddress::Make(FirstChild.Get_LayerIndex(), FirstChild.Get_NodeIndex() + ChildOffset),
                    OutFreeNodes);
            }

            return;
        }

        const auto& LeafNodes = InOctree.Get_LeafNodes();
        const auto LeafIsInRange = LeafNodes.Get_LeafNodes().IsValidIndex(NodeIdx);

        CK_ENSURE_IF_NOT(LeafIsInRange,
            TEXT("Layer-0 node [{}] has no leaf entry - the leaf store holds [{}] leaves"),
            NodeIdx, LeafNodes.Get_LeafNodes().Num())
        {}

        if (NOT LeafIsInRange)
        { return; }

        const auto& LeafNode = LeafNodes.Get_LeafNode(NodeIdx);

        if (LeafNode.Get_IsFullyOccluded())
        { return; }

        if (LeafNode.Get_IsFullyFree())
        {
            OutFreeNodes.Emplace(InAddress);
            return;
        }

        for (auto Candidate = 0; Candidate <= FNodeAddress::MaxSubNodeIndex; ++Candidate)
        {
            const auto CandidateIdx = static_cast<SubNodeIndex>(Candidate);

            if (LeafNode.Get_IsSubNodeOccluded(CandidateIdx))
            { continue; }

            OutFreeNodes.Emplace(FNodeAddress::Make(0, NodeIdx, CandidateIdx));
        }
    }

    auto
        TryGet_RandomFreePoint(
            const FOctree& InOctree,
            FRandomStream& InOutStream)
        -> TOptional<FVector>
    {
        const auto LayerCount = InOctree.Get_LayerCount();
        const auto HasLayers = LayerCount > 0;

        CK_ENSURE_IF_NOT(HasLayers,
            TEXT("Cannot sample a free point from a VoxelNav octree that has no layers"))
        {}

        if (NOT HasLayers)
        { return {}; }

        // The coarsest layer, NOT one past it: an octree with N layers indexes them 0..N-1.
        const auto TopLayerIndex = static_cast<LayerIndex>(LayerCount - 1);

        if (InOctree.Get_Layer(TopLayerIndex).Get_NodeCount() == 0)
        { return {}; }

        auto FreeNodes = TArray<FNodeAddress>{};
        Get_FreeNodesUnderAddress(InOctree, FNodeAddress::Make(TopLayerIndex, 0), FreeNodes);

        if (FreeNodes.IsEmpty())
        { return {}; }

        const auto RandomAddress = FreeNodes[InOutStream.RandRange(0, FreeNodes.Num() - 1)];

        const auto NodePosition = Get_NodePositionFromAddress(
            InOctree, RandomAddress, ECk_VoxelNav_SubNodePrecision::SubNodeCenter);

        const auto NodeBounds = FBox::BuildAABB(
            NodePosition,
            FVector{Get_NodeExtentFromAddress(InOctree, RandomAddress)});

        return FVector
        {
            InOutStream.FRandRange(static_cast<float>(NodeBounds.Min.X), static_cast<float>(NodeBounds.Max.X)),
            InOutStream.FRandRange(static_cast<float>(NodeBounds.Min.Y), static_cast<float>(NodeBounds.Max.Y)),
            InOutStream.FRandRange(static_cast<float>(NodeBounds.Min.Z), static_cast<float>(NodeBounds.Max.Z))
        };
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CellCenter(
            const FOctree& InOctree,
            const FCellId& InCell)
        -> FVector
    {
        if (InCell.Get_Kind() == ECellKind::Merged)
        {
            const auto* MergedCell = InOctree.Get_MergedCells().TryGet_Cell(InCell.Get_MergedIndex());

            if (ck::Is_NOT_Valid(MergedCell, ck::IsValid_Policy_NullptrOnly{}))
            { return FVector::ZeroVector; }

            return MergedCell->Get_Center();
        }

        return Get_NodePositionFromAddress(
            InOctree, InCell.Get_NodeAddress(), ECk_VoxelNav_SubNodePrecision::SubNodeCenter);
    }

    auto
        Get_CellBounds(
            const FOctree& InOctree,
            const FCellId& InCell)
        -> FBox
    {
        if (InCell.Get_Kind() == ECellKind::Merged)
        {
            const auto* MergedCell = InOctree.Get_MergedCells().TryGet_Cell(InCell.Get_MergedIndex());

            if (ck::Is_NOT_Valid(MergedCell, ck::IsValid_Policy_NullptrOnly{}))
            { return FBox{ForceInit}; }

            return MergedCell->Get_Bounds();
        }

        const auto Extent = Get_NodeExtentFromAddress(InOctree, InCell.Get_NodeAddress());

        if (Extent <= 0.0f)
        { return FBox{ForceInit}; }

        return FBox::BuildAABB(Get_CellCenter(InOctree, InCell), FVector{Extent});
    }

    auto
        Get_CellExtent(
            const FOctree& InOctree,
            const FCellId& InCell)
        -> float
    {
        if (InCell.Get_Kind() == ECellKind::Merged)
        {
            const auto* MergedCell = InOctree.Get_MergedCells().TryGet_Cell(InCell.Get_MergedIndex());

            if (ck::Is_NOT_Valid(MergedCell, ck::IsValid_Policy_NullptrOnly{}))
            { return 0.0f; }

            return MergedCell->Get_MinExtent();
        }

        return Get_NodeExtentFromAddress(InOctree, InCell.Get_NodeAddress());
    }

    auto
        Get_CellIsFree(
            const FOctree& InOctree,
            const FCellId& InCell)
        -> bool
    {
        // A merged cell is a box the merge pass assembled out of free cells only, so its existence IS its
        // freedom - there is nothing further to look up.
        if (InCell.Get_Kind() == ECellKind::Merged)
        { return InOctree.Get_MergedCells().TryGet_Cell(InCell.Get_MergedIndex()) != nullptr; }

        const auto Address = InCell.Get_NodeAddress();
        const auto* Node = TryGet_NodeFromAddress(InOctree, Address);

        if (ck::Is_NOT_Valid(Node, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        if (Address.Get_LayerIndex() != 0)
        { return NOT Node->Get_HasChildren(); }

        const auto& LeafNodes = InOctree.Get_LeafNodes();

        if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(Address.Get_NodeIndex()))
        { return NOT Node->Get_HasChildren(); }

        return NOT LeafNodes
            .Get_LeafNode(Address.Get_NodeIndex())
            .Get_IsSubNodeOccluded(Address.Get_SubNodeIndex());
    }

    auto
        Get_CellNeighbors(
            const FOctree& InOctree,
            const FCellId& InCell,
            TArray<FCellId>& OutNeighbors)
        -> void
    {
        const auto& MergedCells = InOctree.Get_MergedCells();

        if (InCell.Get_Kind() == ECellKind::Merged)
        {
            const auto* MergedCell = MergedCells.TryGet_Cell(InCell.Get_MergedIndex());

            if (ck::Is_NOT_Valid(MergedCell, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            OutNeighbors.Reserve(OutNeighbors.Num() + MergedCell->Get_Neighbors().Num());

            for (const auto NeighbourMergedIndex : MergedCell->Get_Neighbors())
            { OutNeighbors.Emplace(FCellId::Make_FromMergedIndex(InCell.Get_Volume(), NeighbourMergedIndex)); }

            return;
        }

        auto NeighbourAddresses = TArray<FNodeAddress>{};
        Get_NodeNeighbours(InOctree, InCell.Get_NodeAddress(), NeighbourAddresses);

        OutNeighbors.Reserve(OutNeighbors.Num() + NeighbourAddresses.Num());

        for (const auto& NeighbourAddress : NeighbourAddresses)
        {
            // A merged octree answers in merged cells even when asked about a node cell, so a node cell
            // handed to a search is an entry point into the merged graph rather than a dead end in it.
            const auto NeighbourMergedIndex = MergedCells.Get_MergedIndexForCell(NeighbourAddress);

            if (NeighbourMergedIndex != INDEX_NONE)
            {
                OutNeighbors.AddUnique(FCellId::Make_FromMergedIndex(InCell.Get_Volume(), NeighbourMergedIndex));
                continue;
            }

            OutNeighbors.Emplace(FCellId::Make_FromNodeAddress(InCell.Get_Volume(), NeighbourAddress));
        }
    }

    auto
        TryGet_CellAtPosition(
            const FOctree& InOctree,
            FVolumeId InVolume,
            const FVector& InPosition,
            LayerIndex InMinLayerIndex)
        -> FCellId
    {
        // Snaps a position buried in geometry to the nearest free cell of its leaf, and falls back to a
        // whole-octree nearest-free scan - which is exactly what resolving a path endpoint wants.
        const auto Lookup = TryGet_NodeAddressFromPosition(InOctree, InPosition, InMinLayerIndex);

        if (Lookup._Outcome != ECk_VoxelNav_AddressLookup::Found)
        { return {}; }

        const auto MergedIndex = InOctree.Get_MergedCells().Get_MergedIndexForCell(Lookup._Address);

        if (MergedIndex != INDEX_NONE)
        { return FCellId::Make_FromMergedIndex(InVolume, MergedIndex); }

        return FCellId::Make_FromNodeAddress(InVolume, Lookup._Address);
    }

    auto
        Get_CellTransitionPoints(
            const FOctree& InOctree,
            const FCellId& InFrom,
            const FCellId& InTo,
            TArray<FVector>& OutPoints)
        -> void
    {
        if (InFrom.Get_Kind() == ECellKind::OctreeNode && InTo.Get_Kind() == ECellKind::OctreeNode)
        { return; }

        const auto FromBounds = Get_CellBounds(InOctree, InFrom);
        const auto ToBounds = Get_CellBounds(InOctree, InTo);

        if (FromBounds.IsValid == 0 || ToBounds.IsValid == 0 || NOT FromBounds.Intersect(ToBounds))
        { return; }

        OutPoints.Emplace(FromBounds.Overlap(ToBounds).GetCenter());
    }
}

// --------------------------------------------------------------------------------------------------------------------
