#include "CkVoxelNav_Chunk_Types.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_chunk_types
{
    constexpr auto MinChunksPerAxis = 1;

    // A chunk finer than this is smaller than the coarsest cell most bakes carry, so splitting further buys
    // nothing and costs one whole octree per chunk.
    constexpr auto MinChunkSizeUu = 1.0;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        GetTypeHash(
            const FChunkId& InChunkId)
        -> uint32
    {
        return HashCombine(GetTypeHash(InChunkId.Get_Volume()), ::GetTypeHash(InChunkId.Get_Index()));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FChunkPartition::
        Get_ChunkBounds(
            int32 InChunkIndex) const
        -> FBox
    {
        const auto IndexIsInRange = _ChunkBounds.IsValidIndex(InChunkIndex);

        CK_ENSURE_IF_NOT(IndexIsInRange,
            TEXT("VoxelNav chunk index [{}] is outside the partition's [{}] chunks"),
            InChunkIndex, _ChunkBounds.Num())
        { return FBox{ForceInit}; }

        return _ChunkBounds[InChunkIndex];
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ChunkIndexFromCoord(
            const FIntVector& InDivisions,
            const FIntVector& InCoord)
        -> int32
    {
        return (InCoord.X * InDivisions.Y + InCoord.Y) * InDivisions.Z + InCoord.Z;
    }

    auto
        Get_ChunkCoordFromIndex(
            const FIntVector& InDivisions,
            int32 InChunkIndex)
        -> FIntVector
    {
        const auto DivisionsAreUsable = InDivisions.X > 0 && InDivisions.Y > 0 && InDivisions.Z > 0;

        CK_ENSURE_IF_NOT(DivisionsAreUsable,
            TEXT("VoxelNav chunk divisions [{}] must be positive on every axis"), InDivisions)
        { return FIntVector::ZeroValue; }

        const auto Z = InChunkIndex % InDivisions.Z;
        const auto Remainder = InChunkIndex / InDivisions.Z;

        return FIntVector{Remainder / InDivisions.Y, Remainder % InDivisions.Y, Z};
    }

    auto
        Get_ChunkPartition(
            const FChunkPartitionParams& InParams)
        -> FChunkPartition
    {
        using namespace ck_voxelnav_chunk_types;

        auto Partition = FChunkPartition{};

        const auto DoMakeSingleChunk = [&]() -> FChunkPartition
        {
            Partition._Divisions = FIntVector{1, 1, 1};
            Partition._ChunkBounds.Emplace(InParams._VolumeBounds);

            return Partition;
        };

        if (InParams._VolumeBounds.IsValid == 0 ||
            InParams._MaxChunkSizeUu < MinChunkSizeUu ||
            InParams._MaxChunksPerAxis < MinChunksPerAxis)
        { return DoMakeSingleChunk(); }

        const auto VolumeSize = InParams._VolumeBounds.GetSize();
        const auto MaxChunkSize = static_cast<double>(InParams._MaxChunkSizeUu);

        const auto Get_DivisionsOnAxis = [&](double InAxisSize) -> int32
        {
            return FMath::Clamp(
                FMath::CeilToInt32(InAxisSize / MaxChunkSize), MinChunksPerAxis, InParams._MaxChunksPerAxis);
        };

        const auto Divisions = FIntVector
        {
            Get_DivisionsOnAxis(VolumeSize.X),
            Get_DivisionsOnAxis(VolumeSize.Y),
            Get_DivisionsOnAxis(VolumeSize.Z)
        };

        if (Divisions == FIntVector{1, 1, 1})
        { return DoMakeSingleChunk(); }

        Partition._Divisions = Divisions;
        Partition._ChunkBounds.Reserve(Divisions.X * Divisions.Y * Divisions.Z);

        const auto ChunkSize = FVector
        {
            VolumeSize.X / Divisions.X,
            VolumeSize.Y / Divisions.Y,
            VolumeSize.Z / Divisions.Z
        };

        for (auto X = 0; X < Divisions.X; ++X)
        {
            for (auto Y = 0; Y < Divisions.Y; ++Y)
            {
                for (auto Z = 0; Z < Divisions.Z; ++Z)
                {
                    const auto ChunkMin = InParams._VolumeBounds.Min + FVector{X * ChunkSize.X, Y * ChunkSize.Y, Z * ChunkSize.Z};

                    auto ChunkMax = ChunkMin + ChunkSize;

                    // Snapped rather than accumulated: repeated addition of a divided size leaves the last
                    // chunk short of the authored bound by a float's worth, and that sliver is space no
                    // chunk would ever bake.
                    if (X == Divisions.X - 1)
                    { ChunkMax.X = InParams._VolumeBounds.Max.X; }
                    if (Y == Divisions.Y - 1)
                    { ChunkMax.Y = InParams._VolumeBounds.Max.Y; }
                    if (Z == Divisions.Z - 1)
                    { ChunkMax.Z = InParams._VolumeBounds.Max.Z; }

                    Partition._ChunkBounds.Emplace(FBox{ChunkMin, ChunkMax});
                }
            }
        }

        return Partition;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FChunkAdjacencyTable::
        Request_AddPortal(
            const FChunkPortal& InPortal)
        -> void
    {
        const auto FromIndex = InPortal._FromChunk.Get_Index();
        const auto ToIndex = InPortal._ToChunk.Get_Index();

        const auto PortalIsUsable =
            InPortal._FromChunk.Get_IsValid() &&
            InPortal._ToChunk.Get_IsValid() &&
            FromIndex != ToIndex;

        CK_ENSURE_IF_NOT(PortalIsUsable,
            TEXT("VoxelNav chunk portal [{}] -> [{}] does not name two distinct valid chunks"),
            FromIndex, ToIndex)
        { return; }

        _PortalsByPair.FindOrAdd(Get_PairKey(FromIndex, ToIndex)).Emplace(InPortal);

        auto& Neighbors = _NeighborsByChunk.FindOrAdd(FromIndex);

        if (NOT Neighbors.Contains(ToIndex))
        { Neighbors.Emplace(ToIndex); }
    }

    auto
        FChunkAdjacencyTable::
        Request_Reset()
        -> void
    {
        _PortalsByPair.Reset();
        _NeighborsByChunk.Reset();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FChunkAdjacencyTable::
        Get_Portals(
            int32 InFromChunkIndex,
            int32 InToChunkIndex) const
        -> TArrayView<const FChunkPortal>
    {
        const auto* Portals = _PortalsByPair.Find(Get_PairKey(InFromChunkIndex, InToChunkIndex));

        if (Portals == nullptr)
        { return {}; }

        return TArrayView<const FChunkPortal>{*Portals};
    }

    auto
        FChunkAdjacencyTable::
        Get_NeighborIndices(
            int32 InChunkIndex) const
        -> TArrayView<const int32>
    {
        const auto* Neighbors = _NeighborsByChunk.Find(InChunkIndex);

        if (Neighbors == nullptr)
        { return {}; }

        return TArrayView<const int32>{*Neighbors};
    }

    auto
        FChunkAdjacencyTable::
        Get_AreChunksAdjacent(
            int32 InFromChunkIndex,
            int32 InToChunkIndex) const
        -> bool
    {
        return NOT Get_Portals(InFromChunkIndex, InToChunkIndex).IsEmpty();
    }

    auto
        FChunkAdjacencyTable::
        Get_PortalCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& Kvp : _PortalsByPair)
        { Count += Kvp.Value.Num(); }

        return Count;
    }

    auto
        FChunkAdjacencyTable::
        Get_PairKey(
            int32 InFromChunkIndex,
            int32 InToChunkIndex)
        -> uint64
    {
        return (static_cast<uint64>(static_cast<uint32>(InFromChunkIndex)) << 32) |
                static_cast<uint64>(static_cast<uint32>(InToChunkIndex));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ChunkEpochSum(
            const TArray<FChunkPublishState>& InChunks)
        -> int64
    {
        auto Sum = int64{0};

        for (const auto& Chunk : InChunks)
        { Sum += Chunk._Epoch; }

        return Sum;
    }

    auto
        Get_AllChunksArePublished(
            const TArray<FChunkPublishState>& InChunks)
        -> bool
    {
        if (InChunks.IsEmpty())
        { return false; }

        for (const auto& Chunk : InChunks)
        {
            if (NOT Chunk._IsBuilt)
            { return false; }
        }

        return true;
    }

    auto
        Get_ChunksNeedAggregation(
            const TArray<FChunkPublishState>& InChunks,
            int64 InLastAggregatedEpochSum)
        -> bool
    {
        return Get_AllChunksArePublished(InChunks) &&
               Get_ChunkEpochSum(InChunks) != InLastAggregatedEpochSum;
    }

    auto
        Get_ChunkBuildFailed(
            const TArray<FChunkPublishState>& InChunks)
        -> bool
    {
        auto AnyChunkFailed = false;

        for (const auto& Chunk : InChunks)
        {
            if (Chunk._IsBuilding)
            { return false; }

            AnyChunkFailed |= Chunk._HasFailed;
        }

        return AnyChunkFailed;
    }
}

// --------------------------------------------------------------------------------------------------------------------
