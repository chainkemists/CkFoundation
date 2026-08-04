#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Engine-light Sparse Voxel Octree data. No UObject, no reflection, no physics: an FOctree is a plain
// value that a fragment owns and publishes as TSharedPtr<const FOctree> once its build completes, which
// is what makes post-bake reads lock-free.
//
// Layer numbering is INVERTED relative to intuition and is load-bearing everywhere below: layer 0 holds
// the FINEST cells (the leaf nodes, each subdivided into 4x4x4 sub-nodes), and higher layer indices are
// progressively coarser. A node's parent therefore lives at a HIGHER layer index.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    // Pinned widths. uint_fast64_t (upstream) is platform-variable and would make the packed node-ref
    // wire form depend on the toolchain.
    using MortonCode         = uint64;
    using LayerIndex         = uint8;
    using NodeIndex          = int32;
    using LeafIndex          = int32;
    using SubNodeIndex       = uint8;
    using NeighbourDirection = uint8;

    // The six orthogonal face directions, indexed by NeighbourDirection. The order is load-bearing: the
    // per-face child-offset tables in the neighbour walk are indexed by the same value.
    inline const FIntVector GNeighbourDirections[6] =
    {
        FIntVector{ 1,  0,  0},
        FIntVector{-1,  0,  0},
        FIntVector{ 0,  1,  0},
        FIntVector{ 0, -1,  0},
        FIntVector{ 0,  0,  1},
        FIntVector{ 0,  0, -1}
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** A node's address inside one octree, as a single packed uint32:
     *    bits [31..28] LayerIndex   (0..13 addressable; 14 reserved by FCellId; 15 == invalid)
     *    bits [27..6]  NodeIndex    (0..4'194'303)
     *    bits [5..0]   SubNodeIndex (0..63)
     *  The packed uint32 IS the serialization form. Never write the struct's raw memory: a bitfield
     *  layout is compiler- and ABI-dependent, so a bake written that way silently rots across toolchains. */
    struct CKVOXELNAV_API FNodeAddress
    {
    public:
        static constexpr uint32       InvalidPacked   = 0xF0000000u;
        static constexpr LayerIndex   InvalidLayer    = 15;
        static constexpr LayerIndex   MergedCellLayer = 14;
        static constexpr LayerIndex   MaxLayerIndex   = 13;
        static constexpr NodeIndex    MaxNodeIndex    = 0x3FFFFF;
        static constexpr SubNodeIndex MaxSubNodeIndex = 63;

    public:
        FNodeAddress() = default;

        static auto
        Make(
            LayerIndex InLayerIndex,
            NodeIndex InNodeIndex,
            SubNodeIndex InSubNodeIndex = 0) -> FNodeAddress;

        static auto
        Make_FromPacked(
            uint32 InPacked) -> FNodeAddress;

    public:
        auto Get_Packed()       const -> uint32       { return _Packed; }
        auto Get_LayerIndex()   const -> LayerIndex   { return static_cast<LayerIndex>(_Packed >> 28); }
        auto Get_NodeIndex()    const -> NodeIndex    { return static_cast<NodeIndex>((_Packed >> 6) & 0x3FFFFFu); }
        auto Get_SubNodeIndex() const -> SubNodeIndex { return static_cast<SubNodeIndex>(_Packed & 0x3Fu); }

        // Neither the merged-cell sentinel nor the invalid sentinel addresses an octree node.
        auto Get_IsValid()      const -> bool         { return Get_LayerIndex() <= MaxLayerIndex; }

    public:
        auto
        Set_NodeIndex(
            NodeIndex InNodeIndex) -> FNodeAddress&;

        auto
        Set_SubNodeIndex(
            SubNodeIndex InSubNodeIndex) -> FNodeAddress&;

        auto Invalidate() -> void { _Packed = InvalidPacked; }

    public:
        auto operator==(const FNodeAddress&) const -> bool = default;

    private:
        uint32 _Packed = InvalidPacked;
    };

    CKVOXELNAV_API auto
    GetTypeHash(
        const FNodeAddress& InAddress) -> uint32;

    // ----------------------------------------------------------------------------------------------------------------

    /** Stable integer identity of a voxel volume. Never an actor pointer: a bake outlives the actor that
     *  authored it, and cross-volume paths need an id that survives streaming and serialization. */
    struct CKVOXELNAV_API FVolumeId
    {
    public:
        static constexpr uint32 InvalidValue = 0u;

    public:
        FVolumeId() = default;

        explicit FVolumeId(uint32 InValue)
            : _Value(InValue)
        {}

    public:
        auto Get_Value()   const -> uint32 { return _Value; }
        auto Get_IsValid() const -> bool   { return _Value != InvalidValue; }

    public:
        auto operator==(const FVolumeId&) const -> bool = default;

    private:
        uint32 _Value = InvalidValue;
    };

    CKVOXELNAV_API auto
    GetTypeHash(
        const FVolumeId& InVolumeId) -> uint32;

    // ----------------------------------------------------------------------------------------------------------------

    enum class ECellKind : uint8
    {
        OctreeNode,
        Merged
    };

    /** Volume-qualified cell identity - 8 bytes, copyable, equality-comparable, hashable. This is what
     *  search addresses; it never addresses FNodeAddress directly, so a coarser cell representation can
     *  be introduced behind the kind-dispatching free functions without reworking search or neighbours.
     *
     *  The merged kind is encoded by reserving layer-nibble value 14 (15 is already the invalid
     *  sentinel), which keeps the id at 8 bytes - it lands in a TSet/TMap for every expansion of every
     *  search, so its width is a real cost. */
    struct CKVOXELNAV_API FCellId
    {
    public:
        static constexpr int32 MaxMergedIndex = 0x0FFFFFFF;

    public:
        FCellId() = default;

        static auto
        Make_FromNodeAddress(
            FVolumeId InVolume,
            FNodeAddress InAddress) -> FCellId;

        static auto
        Make_FromMergedIndex(
            FVolumeId InVolume,
            int32 InMergedIndex) -> FCellId;

    public:
        auto Get_Volume() const -> FVolumeId { return FVolumeId{_Volume}; }

        /** The cell's kind-tagged payload word. This is the serialization form and the hash input; it is
         *  NOT a node address - read it through Get_Cell* or Get_NodeAddress(). */
        auto Get_Packed() const -> uint32 { return _Packed; }

        auto
        Get_Kind() const -> ECellKind;

        auto
        Get_IsValid() const -> bool;

        /** Meaningful only when Get_Kind() == OctreeNode; ensures and returns an invalid address
         *  otherwise. Search-facing code must go through Get_Cell* instead of calling this. */
        auto
        Get_NodeAddress() const -> FNodeAddress;

        /** Meaningful only when Get_Kind() == Merged; ensures and returns INDEX_NONE otherwise. */
        auto
        Get_MergedIndex() const -> int32;

    public:
        auto operator==(const FCellId&) const -> bool = default;

    private:
        uint32 _Volume = FVolumeId::InvalidValue;
        uint32 _Packed = FNodeAddress::InvalidPacked;
    };

    CKVOXELNAV_API auto
    GetTypeHash(
        const FCellId& InCell) -> uint32;

    // ----------------------------------------------------------------------------------------------------------------

    /** One merged cell: an axis-aligned box of free space assembled from WHOLE octree cells, plus the
     *  merged cells sharing a face with it. Neighbours are stored as indices into the table's own array,
     *  so the graph a search walks is an integer adjacency list rather than a per-expansion octree walk. */
    struct CKVOXELNAV_API FMergedCell
    {
    public:
        FMergedCell() = default;

        explicit FMergedCell(const FBox& InBounds)
            : _Bounds(InBounds)
        {}

    public:
        auto Get_Bounds()    const -> const FBox&          { return _Bounds; }
        auto Get_Center()    const -> FVector              { return _Bounds.GetCenter(); }
        auto Get_Neighbors() const -> const TArray<int32>& { return _Neighbors; }
        auto Get_CellCount() const -> int32                { return _CellCount; }

        /** The box's SMALLEST half-extent. A merged cell is free everywhere inside it, so its narrowest
         *  axis is the only one an agent-size filter can honestly answer with. */
        auto
        Get_MinExtent() const -> float;

    public:
        auto
        Set_Bounds(
            const FBox& InBounds) -> FMergedCell&;

        auto
        Set_CellCount(
            int32 InCellCount) -> FMergedCell&;

        auto
        Set_Neighbors(
            TArray<int32> InNeighbors) -> FMergedCell&;

    private:
        FBox _Bounds = FBox{ForceInit};
        TArray<int32> _Neighbors;
        int32 _CellCount = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The merged-cell table: the coarser cell representation the kind-dispatching seam hands search when
     *  merging is on, plus the octree-cell to merged-cell lookup that resolves an endpoint into it.
     *
     *  Keyed by the PACKED node address rather than by an FCellId: the table belongs to one octree, so the
     *  volume qualifier an FCellId carries would be a runtime value stored in baked data. */
    struct CKVOXELNAV_API FMergedCellTable
    {
    public:
        auto Get_Cells()           const -> const TArray<FMergedCell>& { return _Cells; }
        auto Get_CellCount()       const -> int32 { return _Cells.Num(); }
        auto Get_IsMerged()        const -> bool  { return _IsMerged; }
        auto Get_SourceCellCount() const -> int32 { return _SourceCellCount; }
        auto Get_CarriedOverCount()const -> int32 { return _CarriedOverCount; }

        /** nullptr when the index names no merged cell - never a shared placeholder, for the same reason
         *  TryGet_NodeFromAddress refuses one. */
        auto
        TryGet_Cell(
            int32 InMergedIndex) const -> const FMergedCell*;

        /** INDEX_NONE when this octree cell belongs to no merged cell, which is what an unmerged octree
         *  and an occluded cell both answer. */
        auto
        Get_MergedIndexForCell(
            const FNodeAddress& InAddress) const -> int32;

        auto
        Get_AllocatedSize() const -> int32;

    public:
        auto
        Request_Reset() -> void;

        auto
        Request_AddCell(
            const FMergedCell& InCell) -> int32;

        auto
        Request_MapCellToMerged(
            const FNodeAddress& InAddress,
            int32 InMergedIndex) -> void;

        auto
        Request_SetBounds(
            int32 InMergedIndex,
            const FBox& InBounds) -> void;

        auto
        Request_SetNeighbors(
            int32 InMergedIndex,
            TArray<int32> InNeighbors) -> void;

        auto
        Request_SetCellCount(
            int32 InMergedIndex,
            int32 InCellCount) -> void;

        /** Flips the table to readable, exactly as Request_MarkOctreeBuilt does for the octree: a table
         *  mid-assembly must never answer a query. */
        auto
        Request_MarkMerged(
            int32 InSourceCellCount,
            int32 InCarriedOverCount) -> void;

    private:
        TArray<FMergedCell> _Cells;
        TMap<uint32, int32> _CellToMerged;
        int32 _SourceCellCount = 0;
        int32 _CarriedOverCount = 0;
        bool _IsMerged = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One leaf node's 4x4x4 occupancy, one bit per sub-node, indexed by the sub-node's Morton code. */
    struct CKVOXELNAV_API FLeafNode
    {
    public:
        auto Get_SubNodes() const -> uint64       { return _SubNodes; }
        auto Get_Parent()   const -> FNodeAddress { return _Parent; }

        auto
        Get_IsSubNodeOccluded(
            SubNodeIndex InSubNodeIndex) const -> bool;

        auto Get_IsAnySubNodeOccluded() const -> bool { return _SubNodes != 0; }
        auto Get_IsFullyOccluded()      const -> bool { return _SubNodes == TNumericLimits<uint64>::Max(); }
        auto Get_IsFullyFree()          const -> bool { return _SubNodes == 0; }

    public:
        auto
        Request_MarkSubNodeOccluded(
            SubNodeIndex InSubNodeIndex) -> void;

        /** Writes the whole 4x4x4 occupancy word at once, REPLACING what was there. A local repair
         *  re-derives a dirty leaf's occupancy from scratch and must be able to clear bits an obstacle
         *  vacated; accumulating into the old word instead is how upstream's dynamic occlusion left
         *  permanent occupied trails behind a moving obstacle. */
        auto
        Set_SubNodes(
            uint64 InSubNodes) -> FLeafNode&;

        auto
        Set_Parent(
            FNodeAddress InParent) -> FLeafNode&;

    private:
        uint64 _SubNodes = 0;
        FNodeAddress _Parent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One octree node. A node exists only where the volume is subdivided, so "has no children" reads as
     *  "this whole cell is navigable free space". */
    struct CKVOXELNAV_API FNode
    {
    public:
        FNode() = default;

        explicit FNode(MortonCode InMortonCode)
            : _MortonCode(InMortonCode)
        {}

    public:
        auto Get_MortonCode()  const -> MortonCode   { return _MortonCode; }
        auto Get_Parent()      const -> FNodeAddress { return _Parent; }
        auto Get_FirstChild()  const -> FNodeAddress { return _FirstChild; }
        auto Get_HasChildren() const -> bool         { return _FirstChild.Get_IsValid(); }

        auto
        Get_Neighbour(
            NeighbourDirection InDirection) const -> FNodeAddress;

    public:
        auto
        Set_MortonCode(
            MortonCode InMortonCode) -> FNode&;

        auto
        Set_Parent(
            FNodeAddress InParent) -> FNode&;

        auto
        Set_FirstChild(
            FNodeAddress InFirstChild) -> FNode&;

        auto
        Set_Neighbour(
            NeighbourDirection InDirection,
            FNodeAddress InNeighbour) -> FNode&;

    private:
        MortonCode _MortonCode = 0;
        FNodeAddress _Parent;
        FNodeAddress _FirstChild;
        FNodeAddress _Neighbours[6];
    };

    // Morton ordering is the layer invariant every binary search over a layer's nodes depends on.
    inline auto
    operator<(
        const FNode& InLeft,
        const FNode& InRight) -> bool
    {
        return InLeft.Get_MortonCode() < InRight.Get_MortonCode();
    }

    // ----------------------------------------------------------------------------------------------------------------

    /** One layer of the octree. `_MaxNodeCount` is the CUBE capacity (edge^3) and `_EdgeNodeCount` is the
     *  per-axis capacity - comparing a per-axis coordinate against the cube count is a bounds check that
     *  is edge^2 too permissive, so coordinate bounds ALWAYS use Get_EdgeNodeCount(). */
    struct CKVOXELNAV_API FLayer
    {
    public:
        FLayer() = default;

        FLayer(
            int32 InEdgeNodeCount,
            int32 InMaxNodeCount,
            float InNodeSize)
            : _EdgeNodeCount(InEdgeNodeCount)
            , _MaxNodeCount(InMaxNodeCount)
            , _NodeSize(InNodeSize)
        {}

    public:
        auto Get_Nodes() const -> const TArray<FNode>& { return _Nodes; }
        auto Get_Nodes()       -> TArray<FNode>&       { return _Nodes; }

        auto
        Get_Node(
            NodeIndex InNodeIndex) const -> const FNode&;

        auto Get_NodeCount()     const -> int32 { return _Nodes.Num(); }
        auto Get_NodeSize()      const -> float { return _NodeSize; }
        auto Get_NodeExtent()    const -> float { return _NodeSize * 0.5f; }
        auto Get_MaxNodeCount()  const -> int32 { return _MaxNodeCount; }
        auto Get_EdgeNodeCount() const -> int32 { return _EdgeNodeCount; }
        auto Get_AllocatedSize() const -> int32 { return _Nodes.Num() * sizeof(FNode); }

    private:
        TArray<FNode> _Nodes;
        int32 _EdgeNodeCount = 0;
        int32 _MaxNodeCount = 0;
        float _NodeSize = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The leaf-node store, parallel to layer 0: leaf i is the sub-node occupancy of layer-0 node i. */
    struct CKVOXELNAV_API FLeafNodes
    {
    public:
        auto Get_LeafNodes() const -> const TArray<FLeafNode>& { return _LeafNodes; }
        auto Get_LeafNodes()       -> TArray<FLeafNode>&       { return _LeafNodes; }

        auto
        Get_LeafNode(
            LeafIndex InLeafIndex) const -> const FLeafNode&;

        auto
        Get_LeafNode(
            LeafIndex InLeafIndex) -> FLeafNode&;

        auto Get_LeafNodeSize()      const -> float { return _LeafNodeSize; }
        auto Get_LeafNodeExtent()    const -> float { return _LeafNodeSize * 0.5f; }
        auto Get_LeafSubNodeSize()   const -> float { return _LeafNodeSize * 0.25f; }
        auto Get_LeafSubNodeExtent() const -> float { return _LeafNodeSize * 0.125f; }
        auto Get_AllocatedSize()     const -> int32 { return _LeafNodes.Num() * sizeof(FLeafNode); }

    public:
        auto
        Request_Initialize(
            float InLeafNodeSize) -> void;

        auto
        Request_Reset() -> void;

        /** Sizes the store outright. The upstream Reserve-then-grow-one-at-a-time shape was correct only
         *  while callers happened to pass strictly increasing indices. */
        auto
        Request_SetLeafCount(
            int32 InLeafCount) -> void;

    private:
        TArray<FLeafNode> _LeafNodes;
        float _LeafNodeSize = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct FOctree;

    CKVOXELNAV_API auto
    Request_InitializeOctree(
        FOctree& InOutOctree,
        float InFinestCellSizeUu,
        const FBox& InVolumeBounds) -> bool;

    CKVOXELNAV_API auto
    Request_ResetOctree(
        FOctree& InOutOctree) -> void;

    /** Flips the octree to readable. Nothing may query an octree before its build publishes it. */
    CKVOXELNAV_API auto
    Request_MarkOctreeBuilt(
        FOctree& InOutOctree) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    /** The whole structure. Build scratch (the blocked-node sets, the leaf-to-parent map) deliberately
     *  lives OUTSIDE this type - a finished octree carries only what queries read, which is what lets it
     *  be shared immutably across threads. */
    struct CKVOXELNAV_API FOctree
    {
    public:
        friend CKVOXELNAV_API auto Request_InitializeOctree(FOctree&, float, const FBox&) -> bool;
        friend CKVOXELNAV_API auto Request_ResetOctree(FOctree&) -> void;
        friend CKVOXELNAV_API auto Request_MarkOctreeBuilt(FOctree&) -> void;

    public:
        auto Get_Layers() const -> const TArray<FLayer>& { return _Layers; }
        auto Get_LayerCount() const -> int32 { return _Layers.Num(); }

        auto
        Get_Layer(
            LayerIndex InLayerIndex) const -> const FLayer&;

        auto
        Get_Layer(
            LayerIndex InLayerIndex) -> FLayer&;

        auto Get_LeafNodes() const -> const FLeafNodes& { return _LeafNodes; }
        auto Get_LeafNodes()       -> FLeafNodes&       { return _LeafNodes; }

        /** Empty and unmerged unless the bake ran the merge pass. Its presence is what makes the cell seam
         *  hand back merged cells, so it IS the merging switch as far as every query is concerned. */
        auto Get_MergedCells() const -> const FMergedCellTable& { return _MergedCells; }
        auto Get_MergedCells()       -> FMergedCellTable&       { return _MergedCells; }

        /** The power-of-two cube the octree actually addresses, centred on the authored volume. It is
         *  usually LARGER than the authored bounds - a volume is snapped up to the next power of two. */
        auto Get_NavigationBounds() const -> const FBox& { return _NavigationBounds; }
        auto Get_VolumeBounds()     const -> const FBox& { return _VolumeBounds; }

        auto Get_IsValid() const -> bool { return _IsValid && Get_LayerCount() > 0; }

        auto
        Get_AllocatedSize() const -> int32;

    private:
        TArray<FLayer> _Layers;
        FLeafNodes _LeafNodes;
        FMergedCellTable _MergedCells;
        FBox _NavigationBounds = FBox{ForceInit};
        FBox _VolumeBounds = FBox{ForceInit};
        bool _IsValid = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------
