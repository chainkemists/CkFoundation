#pragma once

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Build.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Merge.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Repair.h"

#include <CoreMinimal.h>
#include <HAL/CriticalSection.h>
#include <Misc/ScopeLock.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    enum class EDebugSnapshotSource : uint8
    {
        LivePie,
        RetainedSnapshot,
        EditorPreview
    };

    enum class EDebugSnapshotStatus : uint8
    {
        MissingCook,
        StaleCook,
        Building,
        Current,
        Failed,
        RuntimeOnly
    };

    enum class EDebugSnapshotLayer : uint8
    {
        None       = 0,
        MergedFree = 1 << 0,
        RawFree    = 1 << 1,
        Occupied   = 1 << 2
    };

    constexpr auto operator|(EDebugSnapshotLayer InLeft, EDebugSnapshotLayer InRight) -> EDebugSnapshotLayer
    {
        return static_cast<EDebugSnapshotLayer>(static_cast<uint8>(InLeft) | static_cast<uint8>(InRight));
    }

    constexpr auto operator&(EDebugSnapshotLayer InLeft, EDebugSnapshotLayer InRight) -> EDebugSnapshotLayer
    {
        return static_cast<EDebugSnapshotLayer>(static_cast<uint8>(InLeft) & static_cast<uint8>(InRight));
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct CKVOXELNAV_API FDebugSnapshotCell
    {
        FBox _Bounds = FBox{ForceInit};
        int32 _ChunkIndex = INDEX_NONE;
        int32 _OctreeLayer = INDEX_NONE;
    };

    struct CKVOXELNAV_API FDebugSnapshotLayerOutput
    {
        TArray<FDebugSnapshotCell> _Cells;
        int32 _FilteredTotal = 0;
        bool _Truncated = false;
    };

    struct CKVOXELNAV_API FDebugSnapshotChunk
    {
        FBox _Bounds = FBox{ForceInit};
        int32 _ChunkIndex = INDEX_NONE;
        int32 _Epoch = 0;
        ECk_VoxelNav_BuildStage _BuildStage = ECk_VoxelNav_BuildStage::NotStarted;
        bool _IsBuilt = false;
    };

    struct CKVOXELNAV_API FDebugSnapshotPortal
    {
        FVector _From = FVector::ZeroVector;
        FVector _To = FVector::ZeroVector;
        FVector _ConnectionPoint = FVector::ZeroVector;
        int32 _FromChunkIndex = INDEX_NONE;
        int32 _ToChunkIndex = INDEX_NONE;
    };

    /** A renderer-facing value snapshot. It deliberately owns only copied geometry and metadata, never a
     *  live world, entity, Jolt object, or octree reference. */
    struct CKVOXELNAV_API FDebugSnapshot
    {
        FBox _AuthoredBounds = FBox{ForceInit};
        FBox _NavigationBounds = FBox{ForceInit};
        EDebugSnapshotSource _Source = EDebugSnapshotSource::LivePie;
        EDebugSnapshotStatus _Status = EDebugSnapshotStatus::RuntimeOnly;
        FString _SourceIdentity;
        int32 _SourceEpoch = 0;
        uint64 _SourceFingerprint = 0;
        uint64 _Generation = 0;
        ECk_VoxelNav_BuildStage _BuildStage = ECk_VoxelNav_BuildStage::NotStarted;
        ECk_VoxelNav_RepairStage _RepairStage = ECk_VoxelNav_RepairStage::NotStarted;
        FCk_VoxelNav_BuildStats _BuildStats;
        FCk_VoxelNav_RepairStats _RepairStats;
        float _BuildProgress = 0.0f;
        bool _IsBuilt = false;
        bool _IsPartitioned = false;
        TArray<FDebugSnapshotChunk> _Chunks;
        TArray<FDebugSnapshotPortal> _Portals;
        FBox _PendingDirtyBounds = FBox{ForceInit};
        FBox _ActiveDirtyBounds = FBox{ForceInit};
        FString _StatusMessage;
        FDebugSnapshotLayerOutput _MergedFree;
        FDebugSnapshotLayerOutput _RawFree;
        FDebugSnapshotLayerOutput _Occupied;
    };

    struct CKVOXELNAV_API FDebugSnapshotBuildParams
    {
        EDebugSnapshotLayer _RequestedLayers = EDebugSnapshotLayer::MergedFree;
        int32 _MaxCellsPerLayer = 10000;
        int32 _MinOctreeLayer = 0;
        int32 _MaxOctreeLayer = MAX_int32;
        FBox _ClipBounds = FBox{ForceInit};
    };

    /** All fields that change snapshot contents. A cache key contains no live address so a retained snapshot
     *  remains meaningful after the PIE world that produced it is gone. */
    struct CKVOXELNAV_API FDebugSnapshotCacheKey
    {
        EDebugSnapshotSource _Source = EDebugSnapshotSource::LivePie;
        EDebugSnapshotStatus _Status = EDebugSnapshotStatus::RuntimeOnly;
        FString _Identity;
        int32 _Epoch = 0;
        uint64 _Fingerprint = 0;
        FDebugSnapshotBuildParams _BuildParams;

        auto Get_IsEqual(const FDebugSnapshotCacheKey& InOther) const -> bool;
    };

    /** Owns the last complete value snapshot. Publication is a whole-value replacement under one lock, so a
     *  reader never observes geometry from one build paired with metadata from another. */
    class CKVOXELNAV_API FDebugSnapshotCache
    {
    public:
        /** Returns the cached value only when the cheap source/filter key still matches. Call this before
         *  enumerating an octree; Publish is the miss path, not the cache lookup. */
        auto
        TryGet_SnapshotForKey(
            const FDebugSnapshotCacheKey& InKey,
            FDebugSnapshot& OutSnapshot) const -> bool;

        auto
        Publish(
            const FDebugSnapshotCacheKey& InKey,
            FDebugSnapshot InSnapshot) -> bool;

        auto Get_SnapshotCopy() const -> TOptional<FDebugSnapshot>;

        auto Clear() -> void;

    private:
        mutable FCriticalSection _Lock;
        TOptional<FDebugSnapshotCacheKey> _Key;
        TOptional<FDebugSnapshot> _Snapshot;
        uint64 _NextGeneration = 1;
    };

    /** Appends only the requested cell layers from a completed octree. Totals are after filtering but before
     *  the per-layer cap, which makes truncation visible without retaining every cell. */
    CKVOXELNAV_API auto
    Append_OctreeDebugSnapshot(
        const FOctree& InOctree,
        int32 InChunkIndex,
        const FDebugSnapshotBuildParams& InParams,
        FDebugSnapshot& InOutSnapshot) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
