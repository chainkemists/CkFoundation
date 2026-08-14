#include "CkVoxelNav_Octree_Types.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_types
{
    // A layer's node capacity is edge^3 and is stored as int32, so 1024^3 is the largest cube that fits.
    constexpr auto MaxVoxelExponent = 10;

    // A leaf node is subdivided 4x4x4 into the finest navigable cells.
    constexpr auto LeafSubdivision = 4.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        FNodeAddress::
        Make(
            LayerIndex InLayerIndex,
            NodeIndex InNodeIndex,
            SubNodeIndex InSubNodeIndex)
        -> FNodeAddress
    {
        const auto FieldsAreInRange =
            InLayerIndex <= MaxLayerIndex &&
            InNodeIndex >= 0 &&
            InNodeIndex <= MaxNodeIndex &&
            InSubNodeIndex <= MaxSubNodeIndex;

        CK_ENSURE_IF_NOT(FieldsAreInRange,
            TEXT("Cannot pack a VoxelNav node address from Layer [{}], Node [{}], SubNode [{}] - the packed "
                 "form allows Layer 0-{}, Node 0-{}, SubNode 0-{}"),
            static_cast<int32>(InLayerIndex), InNodeIndex, static_cast<int32>(InSubNodeIndex),
            static_cast<int32>(MaxLayerIndex), MaxNodeIndex, static_cast<int32>(MaxSubNodeIndex))
        { return FNodeAddress{}; }

        auto Address = FNodeAddress{};
        Address._Packed =
            (static_cast<uint32>(InLayerIndex) << 28) |
            ((static_cast<uint32>(InNodeIndex) & 0x3FFFFFu) << 6) |
            (static_cast<uint32>(InSubNodeIndex) & 0x3Fu);

        return Address;
    }

    auto
        FNodeAddress::
        Make_FromPacked(
            uint32 InPacked)
        -> FNodeAddress
    {
        const auto LayerNibble = static_cast<LayerIndex>(InPacked >> 28);

        // Layer 15 is the invalid encoding, so round-tripping an invalid address is legitimate rather
        // than a diagnostic case; it normalizes back to the canonical invalid value.
        if (LayerNibble == InvalidLayer)
        { return FNodeAddress{}; }

        const auto LayerIsAddressable = LayerNibble <= MaxLayerIndex;

        CK_ENSURE_IF_NOT(LayerIsAddressable,
            TEXT("Packed value [{}] does not encode a VoxelNav node address - its layer nibble is [{}], "
                 "which is reserved"),
            InPacked, static_cast<int32>(LayerNibble))
        { return FNodeAddress{}; }

        auto Address = FNodeAddress{};
        Address._Packed = InPacked;

        return Address;
    }

    auto
        FNodeAddress::
        Set_NodeIndex(
            NodeIndex InNodeIndex)
        -> FNodeAddress&
    {
        const auto NodeIndexIsInRange = InNodeIndex >= 0 && InNodeIndex <= MaxNodeIndex;

        CK_ENSURE_IF_NOT(NodeIndexIsInRange,
            TEXT("Node index [{}] does not fit the 22-bit node field of a VoxelNav node address"),
            InNodeIndex)
        { return *this; }

        _Packed = (_Packed & ~(0x3FFFFFu << 6)) | ((static_cast<uint32>(InNodeIndex) & 0x3FFFFFu) << 6);

        return *this;
    }

    auto
        FNodeAddress::
        Set_SubNodeIndex(
            SubNodeIndex InSubNodeIndex)
        -> FNodeAddress&
    {
        const auto SubNodeIndexIsInRange = InSubNodeIndex <= MaxSubNodeIndex;

        CK_ENSURE_IF_NOT(SubNodeIndexIsInRange,
            TEXT("Sub-node index [{}] does not fit the 6-bit sub-node field of a VoxelNav node address"),
            static_cast<int32>(InSubNodeIndex))
        { return *this; }

        _Packed = (_Packed & ~0x3Fu) | (static_cast<uint32>(InSubNodeIndex) & 0x3Fu);

        return *this;
    }

    auto
        GetTypeHash(
            const FNodeAddress& InAddress)
        -> uint32
    {
        return InAddress.Get_Packed();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        GetTypeHash(
            const FVolumeId& InVolumeId)
        -> uint32
    {
        return InVolumeId.Get_Value();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCellId::
        Make_FromNodeAddress(
            FVolumeId InVolume,
            FNodeAddress InAddress)
        -> FCellId
    {
        const auto InputsAreValid = InVolume.Get_IsValid() && InAddress.Get_IsValid();

        CK_ENSURE_IF_NOT(InputsAreValid,
            TEXT("Cannot make a VoxelNav cell id from Volume [{}] and packed address [{}]"),
            InVolume.Get_Value(), InAddress.Get_Packed())
        { return FCellId{}; }

        auto Cell = FCellId{};
        Cell._Volume = InVolume.Get_Value();
        Cell._Packed = InAddress.Get_Packed();

        return Cell;
    }

    auto
        FCellId::
        Make_FromMergedIndex(
            FVolumeId InVolume,
            int32 InMergedIndex)
        -> FCellId
    {
        const auto InputsAreValid =
            InVolume.Get_IsValid() &&
            InMergedIndex >= 0 &&
            InMergedIndex <= MaxMergedIndex;

        CK_ENSURE_IF_NOT(InputsAreValid,
            TEXT("Cannot make a merged VoxelNav cell id from Volume [{}] and merged index [{}] - the index "
                 "must be within 0-{}"),
            InVolume.Get_Value(), InMergedIndex, MaxMergedIndex)
        { return FCellId{}; }

        auto Cell = FCellId{};
        Cell._Volume = InVolume.Get_Value();
        Cell._Packed =
            (static_cast<uint32>(FNodeAddress::MergedCellLayer) << 28) |
            (static_cast<uint32>(InMergedIndex) & 0x0FFFFFFFu);

        return Cell;
    }

    auto
        FCellId::
        Get_Kind() const
        -> ECellKind
    {
        return static_cast<LayerIndex>(_Packed >> 28) == FNodeAddress::MergedCellLayer
            ? ECellKind::Merged
            : ECellKind::OctreeNode;
    }

    auto
        FCellId::
        Get_IsValid() const
        -> bool
    {
        return Get_Volume().Get_IsValid() &&
               static_cast<LayerIndex>(_Packed >> 28) != FNodeAddress::InvalidLayer;
    }

    auto
        FCellId::
        Get_NodeAddress() const
        -> FNodeAddress
    {
        const auto IsOctreeNodeCell = Get_Kind() == ECellKind::OctreeNode;

        CK_ENSURE_IF_NOT(IsOctreeNodeCell,
            TEXT("VoxelNav cell [{}] on Volume [{}] is not an octree-node cell, so it has no node address"),
            _Packed, _Volume)
        { return FNodeAddress{}; }

        return FNodeAddress::Make_FromPacked(_Packed);
    }

    auto
        FCellId::
        Get_MergedIndex() const
        -> int32
    {
        const auto IsMergedCell = Get_Kind() == ECellKind::Merged;

        CK_ENSURE_IF_NOT(IsMergedCell,
            TEXT("VoxelNav cell [{}] on Volume [{}] is not a merged cell, so it has no merged index"),
            _Packed, _Volume)
        { return INDEX_NONE; }

        return static_cast<int32>(_Packed & 0x0FFFFFFFu);
    }

    auto
        GetTypeHash(
            const FCellId& InCell)
        -> uint32
    {
        return HashCombineFast(GetTypeHash(InCell.Get_Volume()), InCell.Get_Packed());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FMergedCell::
        Get_MinExtent() const
        -> float
    {
        if (_Bounds.IsValid == 0)
        { return 0.0f; }

        return static_cast<float>(_Bounds.GetExtent().GetAbsMin());
    }

    auto
        FMergedCell::
        Set_Bounds(
            const FBox& InBounds)
        -> FMergedCell&
    {
        _Bounds = InBounds;
        return *this;
    }

    auto
        FMergedCell::
        Set_CellCount(
            int32 InCellCount)
        -> FMergedCell&
    {
        _CellCount = InCellCount;
        return *this;
    }

    auto
        FMergedCell::
        Set_Neighbors(
            TArray<int32> InNeighbors)
        -> FMergedCell&
    {
        _Neighbors = MoveTemp(InNeighbors);
        return *this;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FMergedCellTable::
        TryGet_Cell(
            int32 InMergedIndex) const
        -> const FMergedCell*
    {
        if (NOT _Cells.IsValidIndex(InMergedIndex))
        { return nullptr; }

        return &_Cells[InMergedIndex];
    }

    auto
        FMergedCellTable::
        Get_MergedIndexForCell(
            const FNodeAddress& InAddress) const
        -> int32
    {
        const auto* MergedIndex = _CellToMerged.Find(InAddress.Get_Packed());

        return MergedIndex != nullptr ? *MergedIndex : INDEX_NONE;
    }

    auto
        FMergedCellTable::
        Get_AllocatedSize() const
        -> int32
    {
        auto AllocatedSize = _Cells.Num() * static_cast<int32>(sizeof(FMergedCell));

        for (const auto& Cell : _Cells)
        { AllocatedSize += Cell.Get_Neighbors().Num() * static_cast<int32>(sizeof(int32)); }

        return AllocatedSize + static_cast<int32>(_CellToMerged.GetAllocatedSize());
    }

    auto
        FMergedCellTable::
        Request_Reset()
        -> void
    {
        _Cells.Reset();
        _CellToMerged.Reset();
        _SourceCellCount = 0;
        _CarriedOverCount = 0;
        _IsMerged = false;
    }

    auto
        FMergedCellTable::
        Request_AddCell(
            const FMergedCell& InCell)
        -> int32
    {
        return _Cells.Emplace(InCell);
    }

    auto
        FMergedCellTable::
        Request_MapCellToMerged(
            const FNodeAddress& InAddress,
            int32 InMergedIndex)
        -> void
    {
        const auto MappingIsInRange = InAddress.Get_IsValid() && _Cells.IsValidIndex(InMergedIndex);

        CK_ENSURE_IF_NOT(MappingIsInRange,
            TEXT("Cannot map VoxelNav cell [{}] onto merged cell [{}] - the table holds [{}] merged cells"),
            InAddress.Get_Packed(), InMergedIndex, _Cells.Num())
        { return; }

        _CellToMerged.Add(InAddress.Get_Packed(), InMergedIndex);
    }

    auto
        FMergedCellTable::
        Request_SetBounds(
            int32 InMergedIndex,
            const FBox& InBounds)
        -> void
    {
        if (NOT _Cells.IsValidIndex(InMergedIndex))
        { return; }

        _Cells[InMergedIndex].Set_Bounds(InBounds);
    }

    auto
        FMergedCellTable::
        Request_SetNeighbors(
            int32 InMergedIndex,
            TArray<int32> InNeighbors)
        -> void
    {
        if (NOT _Cells.IsValidIndex(InMergedIndex))
        { return; }

        _Cells[InMergedIndex].Set_Neighbors(MoveTemp(InNeighbors));
    }

    auto
        FMergedCellTable::
        Request_SetCellCount(
            int32 InMergedIndex,
            int32 InCellCount)
        -> void
    {
        if (NOT _Cells.IsValidIndex(InMergedIndex))
        { return; }

        _Cells[InMergedIndex].Set_CellCount(InCellCount);
    }

    auto
        FMergedCellTable::
        Request_MarkMerged(
            int32 InSourceCellCount,
            int32 InCarriedOverCount)
        -> void
    {
        _SourceCellCount = InSourceCellCount;
        _CarriedOverCount = InCarriedOverCount;
        _IsMerged = true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FLeafNode::
        Get_IsSubNodeOccluded(
            SubNodeIndex InSubNodeIndex) const
        -> bool
    {
        const auto SubNodeIndexIsInRange = InSubNodeIndex <= FNodeAddress::MaxSubNodeIndex;

        CK_ENSURE_IF_NOT(SubNodeIndexIsInRange,
            TEXT("Sub-node index [{}] is outside a leaf node's 4x4x4 occupancy"),
            static_cast<int32>(InSubNodeIndex))
        {}

        // An unaddressable sub-node reports as occluded: the conservative answer keeps a malformed query
        // from opening a path through space nothing ever rasterized.
        if (NOT SubNodeIndexIsInRange)
        { return true; }

        return (_SubNodes & (1ULL << InSubNodeIndex)) != 0;
    }

    auto
        FLeafNode::
        Request_MarkSubNodeOccluded(
            SubNodeIndex InSubNodeIndex)
        -> void
    {
        const auto SubNodeIndexIsInRange = InSubNodeIndex <= FNodeAddress::MaxSubNodeIndex;

        CK_ENSURE_IF_NOT(SubNodeIndexIsInRange,
            TEXT("Cannot mark sub-node [{}] occluded - it is outside a leaf node's 4x4x4 occupancy"),
            static_cast<int32>(InSubNodeIndex))
        { return; }

        _SubNodes |= 1ULL << InSubNodeIndex;
    }

    auto
        FLeafNode::
        Set_SubNodes(
            uint64 InSubNodes)
        -> FLeafNode&
    {
        _SubNodes = InSubNodes;
        return *this;
    }

    auto
        FLeafNode::
        Set_Parent(
            FNodeAddress InParent)
        -> FLeafNode&
    {
        _Parent = InParent;
        return *this;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FNode::
        Get_Neighbour(
            NeighbourDirection InDirection) const
        -> FNodeAddress
    {
        const auto DirectionIsInRange = InDirection < 6;

        CK_ENSURE_IF_NOT(DirectionIsInRange,
            TEXT("Neighbour direction [{}] is outside the six orthogonal faces"),
            static_cast<int32>(InDirection))
        { return FNodeAddress{}; }

        return _Neighbours[InDirection];
    }

    auto
        FNode::
        Set_MortonCode(
            MortonCode InMortonCode)
        -> FNode&
    {
        _MortonCode = InMortonCode;
        return *this;
    }

    auto
        FNode::
        Set_Parent(
            FNodeAddress InParent)
        -> FNode&
    {
        _Parent = InParent;
        return *this;
    }

    auto
        FNode::
        Set_FirstChild(
            FNodeAddress InFirstChild)
        -> FNode&
    {
        _FirstChild = InFirstChild;
        return *this;
    }

    auto
        FNode::
        Set_Neighbour(
            NeighbourDirection InDirection,
            FNodeAddress InNeighbour)
        -> FNode&
    {
        const auto DirectionIsInRange = InDirection < 6;

        CK_ENSURE_IF_NOT(DirectionIsInRange,
            TEXT("Cannot set neighbour in direction [{}] - it is outside the six orthogonal faces"),
            static_cast<int32>(InDirection))
        { return *this; }

        _Neighbours[InDirection] = InNeighbour;

        return *this;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FLayer::
        Get_Node(
            NodeIndex InNodeIndex) const
        -> const FNode&
    {
        return _Nodes[InNodeIndex];
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FLeafNodes::
        Get_LeafNode(
            LeafIndex InLeafIndex) const
        -> const FLeafNode&
    {
        return _LeafNodes[InLeafIndex];
    }

    auto
        FLeafNodes::
        Get_LeafNode(
            LeafIndex InLeafIndex)
        -> FLeafNode&
    {
        return _LeafNodes[InLeafIndex];
    }

    auto
        FLeafNodes::
        Request_Initialize(
            float InLeafNodeSize)
        -> void
    {
        _LeafNodeSize = InLeafNodeSize;
    }

    auto
        FLeafNodes::
        Request_Reset()
        -> void
    {
        _LeafNodes.Reset();
        _LeafNodeSize = 0.0f;
    }

    auto
        FLeafNodes::
        Request_SetLeafCount(
            int32 InLeafCount)
        -> void
    {
        const auto LeafCountIsInRange = InLeafCount >= 0;

        CK_ENSURE_IF_NOT(LeafCountIsInRange,
            TEXT("Cannot size a VoxelNav leaf store to [{}] leaves"), InLeafCount)
        { return; }

        _LeafNodes.SetNum(InLeafCount);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FOctree::
        Get_Layer(
            LayerIndex InLayerIndex) const
        -> const FLayer&
    {
        return _Layers[InLayerIndex];
    }

    auto
        FOctree::
        Get_Layer(
            LayerIndex InLayerIndex)
        -> FLayer&
    {
        return _Layers[InLayerIndex];
    }

    auto
        FOctree::
        Get_AllocatedSize() const
        -> int32
    {
        auto AllocatedSize = _LeafNodes.Get_AllocatedSize() + _MergedCells.Get_AllocatedSize();

        for (const auto& Layer : _Layers)
        { AllocatedSize += Layer.Get_AllocatedSize(); }

        return AllocatedSize;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_ResetOctree(
            FOctree& InOutOctree)
        -> void
    {
        InOutOctree._Layers.Reset();
        InOutOctree._LeafNodes.Request_Reset();
        InOutOctree._MergedCells.Request_Reset();
        InOutOctree._NavigationBounds = FBox{ForceInit};
        InOutOctree._VolumeBounds = FBox{ForceInit};
        InOutOctree._IsValid = false;
    }

    auto
        Request_MarkOctreeBuilt(
            FOctree& InOutOctree)
        -> void
    {
        const auto HasLayers = InOutOctree.Get_LayerCount() > 0;

        CK_ENSURE_IF_NOT(HasLayers,
            TEXT("Cannot publish a VoxelNav octree that has no layers"))
        { return; }

        InOutOctree._IsValid = true;
    }

    auto
        Request_InitializeOctree(
            FOctree& InOutOctree,
            float InFinestCellSizeUu,
            const FBox& InVolumeBounds)
        -> bool
    {
        using namespace ck_voxelnav_octree_types;

        Request_ResetOctree(InOutOctree);

        const auto CellSizeIsPositive = InFinestCellSizeUu > 0.0f;

        CK_ENSURE_IF_NOT(CellSizeIsPositive,
            TEXT("Cannot initialize a VoxelNav octree with a finest cell size of [{}]uu"),
            InFinestCellSizeUu)
        { return false; }

        const auto VolumeSize = InVolumeBounds.IsValid != 0
            ? InVolumeBounds.GetSize().GetAbsMax()
            : 0.0;

        const auto VolumeIsSized = VolumeSize > 0.0;

        CK_ENSURE_IF_NOT(VolumeIsSized,
            TEXT("Cannot initialize a VoxelNav octree over degenerate bounds [{}]"), InVolumeBounds)
        { return false; }

        const auto LeafNodeSize = InFinestCellSizeUu * LeafSubdivision;
        const auto LeavesAcrossVolume = static_cast<float>(VolumeSize) / LeafNodeSize;
        const auto VoxelExponent = FMath::CeilToInt(FMath::Log2(LeavesAcrossVolume));
        const auto LayerCount = VoxelExponent + 1;

        const auto VolumeHoldsMoreThanOneLeaf = LayerCount >= 2;

        CK_ENSURE_IF_NOT(VolumeHoldsMoreThanOneLeaf,
            TEXT("VoxelNav volume bounds [{}] are smaller than a single leaf node - a finest cell size of "
                 "[{}]uu needs a volume of at least [{}]uu across"),
            InVolumeBounds, InFinestCellSizeUu, LeafNodeSize * 2.0f)
        { return false; }

        const auto VolumeFitsTheAddressableCube = VoxelExponent <= MaxVoxelExponent;

        CK_ENSURE_IF_NOT(VolumeFitsTheAddressableCube,
            TEXT("VoxelNav volume bounds [{}] need [{}] subdivision levels at a finest cell size of [{}]uu, "
                 "which overflows a layer's node capacity - the limit is [{}]"),
            InVolumeBounds, VoxelExponent, InFinestCellSizeUu, MaxVoxelExponent)
        { return false; }

        InOutOctree._VolumeBounds = InVolumeBounds;
        InOutOctree._LeafNodes.Request_Initialize(LeafNodeSize);

        const auto NavigationBoundsSize = static_cast<float>(1 << VoxelExponent) * LeafNodeSize;
        InOutOctree._NavigationBounds = FBox::BuildAABB(
            InVolumeBounds.GetCenter(),
            FVector{NavigationBoundsSize * 0.5f});

        InOutOctree._Layers.Reserve(LayerCount);

        for (auto LayerIdx = 0; LayerIdx < LayerCount; ++LayerIdx)
        {
            const auto EdgeNodeCount = 1 << (VoxelExponent - LayerIdx);
            const auto MaxNodeCount = EdgeNodeCount * EdgeNodeCount * EdgeNodeCount;
            const auto NodeSize = NavigationBoundsSize / static_cast<float>(EdgeNodeCount);

            InOutOctree._Layers.Emplace(EdgeNodeCount, MaxNodeCount, NodeSize);
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------
