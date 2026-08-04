#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Chunk identity, the partition math that produces chunks, and the adjacency table that connects them.
//
// Engine-light in the same sense as Octree/ - no UObject, no entity, no world. A chunk is a sub-box of an
// authored volume that bakes its own octree; the only thing tying the pieces together is an integer id.
//
// STABLE IDS, NOT REFERENCES. Nothing in this file names an actor, an entity or a handle. Upstream keyed
// its adjacency edges on TWeakObjectPtr<ANav3DDataChunkActor> and needed a PostLoad repair heuristic that
// re-linked a stale pointer to the nearest spatially-adjacent chunk actor it could find
// (Nav3DDataChunkActor.cpp:81-133) - a bug-shaped workaround for identity that was never stable. A chunk's
// identity here is its parent volume's id plus its index in the partition lattice, both of which are a pure
// function of the authored params.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** Stable identity of one chunk of a partitioned volume: the parent volume's id plus the chunk's index
     *  in the partition lattice. Reproducible from the authored params alone, so it survives anything that
     *  rebuilds the entities - which is exactly what an adjacency edge needs and what an entity id is not. */
    struct CKVOXELNAV_API FChunkId
    {
    public:
        static constexpr int32 InvalidIndex = -1;

    public:
        FChunkId() = default;

        FChunkId(
            FVolumeId InVolume,
            int32 InIndex)
            : _Volume(InVolume)
            , _Index(InIndex)
        {}

    public:
        auto Get_Volume() const -> FVolumeId { return _Volume; }
        auto Get_Index()  const -> int32     { return _Index; }

        auto Get_IsValid() const -> bool { return _Volume.Get_IsValid() && _Index >= 0; }

    public:
        auto operator==(const FChunkId&) const -> bool = default;

    private:
        FVolumeId _Volume;
        int32 _Index = InvalidIndex;
    };

    CKVOXELNAV_API auto
    GetTypeHash(
        const FChunkId& InChunkId) -> uint32;

    // ----------------------------------------------------------------------------------------------------------------

    /** What the partition needs to decide how - and whether - to split a volume. */
    struct CKVOXELNAV_API FChunkPartitionParams
    {
        FBox _VolumeBounds = FBox{ForceInit};

        // The longest edge one chunk may have before the volume splits along that axis.
        float _MaxChunkSizeUu = 0.0f;

        // Hard cap on divisions per axis, so a volume authored far larger than the chunk size produces a
        // bounded number of chunks rather than thousands of them.
        int32 _MaxChunksPerAxis = 8;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The result of partitioning. `_ChunkBounds` is in LATTICE ORDER - index = (x * YDiv + y) * ZDiv + z -
     *  and that order IS the chunk index, so a chunk id is derivable from the params without consulting
     *  anything the runtime built. */
    struct CKVOXELNAV_API FChunkPartition
    {
    public:
        auto Get_Divisions()   const -> const FIntVector&     { return _Divisions; }
        auto Get_ChunkBounds() const -> const TArray<FBox>&   { return _ChunkBounds; }
        auto Get_ChunkCount()  const -> int32                 { return _ChunkBounds.Num(); }

        /** True only when the volume actually SPLIT. A volume that fits inside the chunk size yields one
         *  chunk whose bounds are the volume's own, and a caller must treat that as the unpartitioned case
         *  rather than build a one-chunk hierarchy around it. */
        auto Get_IsPartitioned() const -> bool { return _ChunkBounds.Num() > 1; }

        auto
        Get_ChunkBounds(
            int32 InChunkIndex) const -> FBox;

    public:
        FIntVector _Divisions = FIntVector{1, 1, 1};
        TArray<FBox> _ChunkBounds;
    };

    // ----------------------------------------------------------------------------------------------------------------

    CKVOXELNAV_API auto
    Get_ChunkIndexFromCoord(
        const FIntVector& InDivisions,
        const FIntVector& InCoord) -> int32;

    CKVOXELNAV_API auto
    Get_ChunkCoordFromIndex(
        const FIntVector& InDivisions,
        int32 InChunkIndex) -> FIntVector;

    /** Divisions per axis are ceil(size / max), clamped, and the last chunk on every axis is snapped back
     *  onto the authored bound so floating-point division can never leave a sliver of the volume outside
     *  every chunk. Degenerate or non-positive input yields ONE chunk equal to the input bounds: refusing
     *  to partition is always a legal answer, and it is the one that leaves existing behavior alone. */
    CKVOXELNAV_API auto
    Get_ChunkPartition(
        const FChunkPartitionParams& InParams) -> FChunkPartition;

    // ----------------------------------------------------------------------------------------------------------------

    /** One traversable crossing between two chunks: the boundary cell on each side, and the point on the
     *  shared face a route passes through.
     *
     *  Cells ride as PACKED node addresses rather than FCellIds because an FCellId carries a volume
     *  qualifier, and a chunk's qualifier is a live runtime value. Pairing a stable chunk id with a packed
     *  address keeps the whole table free of anything that does not survive a reload; a caller that wants an
     *  FCellId reconstitutes one against the chunk's current qualifier. */
    struct CKVOXELNAV_API FChunkPortal
    {
        FChunkId _FromChunk;
        FChunkId _ToChunk;

        uint32 _FromCellPacked = FNodeAddress::InvalidPacked;
        uint32 _ToCellPacked = FNodeAddress::InvalidPacked;

        FVector _ConnectionPoint = FVector::ZeroVector;

        auto Get_FromCellAddress() const -> FNodeAddress { return FNodeAddress::Make_FromPacked(_FromCellPacked); }
        auto Get_ToCellAddress()   const -> FNodeAddress { return FNodeAddress::Make_FromPacked(_ToCellPacked); }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Every crossing between every pair of chunks of one volume.
     *
     *  Keyed by the ORDERED pair of chunk indices packed into a uint64. Upstream keyed its connection
     *  interfaces on a TMap<FVector, ...> - a float-keyed map, where two positions that differ in the last
     *  mantissa bit are different keys and the same portal can be stored twice or looked up never. Integer
     *  indices make the key exact.
     *
     *  Portals are stored in BOTH directions, so a search never has to know which side of the pair baked
     *  them. */
    struct CKVOXELNAV_API FChunkAdjacencyTable
    {
    public:
        auto
        Request_AddPortal(
            const FChunkPortal& InPortal) -> void;

        auto
        Request_Reset() -> void;

    public:
        auto
        Get_Portals(
            int32 InFromChunkIndex,
            int32 InToChunkIndex) const -> TArrayView<const FChunkPortal>;

        auto
        Get_NeighborIndices(
            int32 InChunkIndex) const -> TArrayView<const int32>;

        auto
        Get_AreChunksAdjacent(
            int32 InFromChunkIndex,
            int32 InToChunkIndex) const -> bool;

        auto
        Get_PortalCount() const -> int32;

        auto Get_ConnectedChunkCount() const -> int32 { return _NeighborsByChunk.Num(); }
        auto Get_IsEmpty()             const -> bool  { return _PortalsByPair.IsEmpty(); }

    public:
        static auto
        Get_PairKey(
            int32 InFromChunkIndex,
            int32 InToChunkIndex) -> uint64;

    private:
        TMap<uint64, TArray<FChunkPortal>> _PortalsByPair;
        TMap<int32, TArray<int32>> _NeighborsByChunk;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One chunk as a query sees it: its stable id, the qualifier its own FCellIds carry, its authored
     *  sub-box, and a share of its published octree.
     *
     *  `_CellVolumeId` is distinct per chunk, so a cell of one chunk can never compare equal to a cell of
     *  another - which is what lets a cross-chunk route address cells from several bakes in one list. The
     *  shared pointer is a COPY of the chunk's published one, held for the whole query: a chunk that
     *  rebuilds mid-query swaps its own pointer and this copy keeps the structure being walked whole. */
    struct CKVOXELNAV_API FChunkSearchInput
    {
        FChunkId _ChunkId;
        FVolumeId _CellVolumeId;
        FBox _Bounds = FBox{ForceInit};
        TSharedPtr<const FOctree> _Octree;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** What one chunk contributes to its volume's aggregated build state. Ids and counters only - the
     *  aggregation decision must be answerable without reaching into any octree. */
    struct CKVOXELNAV_API FChunkPublishState
    {
        FChunkId _ChunkId;
        bool _IsBuilt = false;
        int32 _Epoch = 0;

        // A build is queued or running on this chunk. It separates "the fan-out has not reached this chunk
        // yet" from "this chunk tried and could not", which is the difference between waiting and reporting.
        bool _IsBuilding = false;
        bool _HasFailed = false;
    };

    /** Epochs only ever increase, so their sum is a monotone fingerprint of "any chunk republished" - which
     *  is the whole question a partitioned volume's own epoch has to answer, and it answers it without
     *  storing a per-chunk table or subscribing to a per-chunk signal. */
    CKVOXELNAV_API auto
    Get_ChunkEpochSum(
        const TArray<FChunkPublishState>& InChunks) -> int64;

    CKVOXELNAV_API auto
    Get_AllChunksArePublished(
        const TArray<FChunkPublishState>& InChunks) -> bool;

    /** A volume re-aggregates once every chunk has published AND the combined epoch has moved since the
     *  last aggregation. Both halves matter: the first stops a half-baked volume publishing adjacency over
     *  chunks that are still rasterizing, the second stops it re-baking adjacency every single tick. */
    CKVOXELNAV_API auto
    Get_ChunksNeedAggregation(
        const TArray<FChunkPublishState>& InChunks,
        int64 InLastAggregatedEpochSum) -> bool;

    /** Every chunk has stopped working and at least one ended in failure - the one state a volume can never
     *  aggregate out of, and therefore the one in which a caller waiting on a fan-out build would otherwise
     *  wait forever. */
    CKVOXELNAV_API auto
    Get_ChunkBuildFailed(
        const TArray<FChunkPublishState>& InChunks) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
