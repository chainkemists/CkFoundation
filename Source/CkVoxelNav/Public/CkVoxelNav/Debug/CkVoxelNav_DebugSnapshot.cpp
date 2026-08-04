#include "CkVoxelNav/Debug/CkVoxelNav_DebugSnapshot.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_debug_snapshot
{
    using namespace ck::voxelnav;

    auto
        Get_HasLayer(
            EDebugSnapshotLayer InRequestedLayers,
            EDebugSnapshotLayer InLayer) -> bool
    {
        return (InRequestedLayers & InLayer) != EDebugSnapshotLayer::None;
    }

    auto
        Get_BoxIsEqual(
            const FBox& InLeft,
            const FBox& InRight) -> bool
    {
        return InLeft.IsValid == InRight.IsValid &&
               (InLeft.IsValid == 0 || (InLeft.Min == InRight.Min && InLeft.Max == InRight.Max));
    }

    auto
        Get_PassesClip(
            const FBox& InBounds,
            const FBox& InClipBounds) -> bool
    {
        return InBounds.IsValid != 0 && (InClipBounds.IsValid == 0 || InBounds.Intersect(InClipBounds));
    }

    auto
        Get_PassesOctreeLayerRange(
            int32 InLayer,
            const FDebugSnapshotBuildParams& InParams) -> bool
    {
        return InLayer >= InParams._MinOctreeLayer && InLayer <= InParams._MaxOctreeLayer;
    }

    auto
        Append_Cell(
            const FBox& InBounds,
            int32 InChunkIndex,
            int32 InOctreeLayer,
            const FDebugSnapshotBuildParams& InParams,
            FDebugSnapshotLayerOutput& InOutOutput) -> void
    {
        if (NOT Get_PassesClip(InBounds, InParams._ClipBounds))
        { return; }

        ++InOutOutput._FilteredTotal;

        const auto CellLimit = FMath::Max(0, InParams._MaxCellsPerLayer);

        if (InOutOutput._Cells.Num() >= CellLimit)
        {
            InOutOutput._Truncated = true;
            return;
        }

        InOutOutput._Cells.Emplace(FDebugSnapshotCell{InBounds, InChunkIndex, InOctreeLayer});
    }

    auto
        Append_RawCells(
            const FOctree& InOctree,
            int32 InChunkIndex,
            const FDebugSnapshotBuildParams& InParams,
            FDebugSnapshotLayerOutput& InOutOutput) -> void
    {
        auto Addresses = TArray<FNodeAddress>{};
        Get_FreeCells(InOctree, Addresses);

        for (const auto& Address : Addresses)
        {
            const auto LayerIndex = static_cast<int32>(Address.Get_LayerIndex());

            if (NOT Get_PassesOctreeLayerRange(LayerIndex, InParams))
            { continue; }

            Append_Cell(Get_NodeBoundsFromAddress(InOctree, Address), InChunkIndex, LayerIndex, InParams, InOutOutput);
        }
    }

    auto
        Append_OccupiedCells(
            const FOctree& InOctree,
            int32 InChunkIndex,
            const FDebugSnapshotBuildParams& InParams,
            FDebugSnapshotLayerOutput& InOutOutput) -> void
    {
        if (NOT Get_PassesOctreeLayerRange(0, InParams))
        { return; }

        auto Addresses = TArray<FNodeAddress>{};
        Get_OccupiedCells(InOctree, Addresses);

        for (const auto& Address : Addresses)
        {
            Append_Cell(Get_NodeBoundsFromAddress(InOctree, Address), InChunkIndex, 0, InParams, InOutOutput);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        FDebugSnapshotCache::
        TryGet_SnapshotForKey(
            const FDebugSnapshotCacheKey& InKey,
            FDebugSnapshot& OutSnapshot) const -> bool
    {
        FScopeLock Lock{&_Lock};

        if (NOT _Key.IsSet() || NOT _Snapshot.IsSet() || NOT _Key->Get_IsEqual(InKey))
        { return false; }

        OutSnapshot = *_Snapshot;
        return true;
    }

    auto
        FDebugSnapshotCacheKey::
        Get_IsEqual(
            const FDebugSnapshotCacheKey& InOther) const -> bool
    {
        const auto& LeftParams = _BuildParams;
        const auto& RightParams = InOther._BuildParams;

        return _Source == InOther._Source && _Status == InOther._Status && _Identity == InOther._Identity &&
               _Epoch == InOther._Epoch && _Fingerprint == InOther._Fingerprint &&
               LeftParams._RequestedLayers == RightParams._RequestedLayers &&
               LeftParams._MaxCellsPerLayer == RightParams._MaxCellsPerLayer &&
               LeftParams._MinOctreeLayer == RightParams._MinOctreeLayer &&
               LeftParams._MaxOctreeLayer == RightParams._MaxOctreeLayer &&
               ck_voxelnav_debug_snapshot::Get_BoxIsEqual(LeftParams._ClipBounds, RightParams._ClipBounds);
    }

    auto
        FDebugSnapshotCache::
        Publish(
            const FDebugSnapshotCacheKey& InKey,
            FDebugSnapshot InSnapshot) -> bool
    {
        FScopeLock Lock{&_Lock};

        if (_Key.IsSet() && _Key->Get_IsEqual(InKey))
        { return false; }

        InSnapshot._Generation = _NextGeneration++;
        _Key = InKey;
        _Snapshot = MoveTemp(InSnapshot);
        return true;
    }

    auto
        FDebugSnapshotCache::
        Get_SnapshotCopy() const -> TOptional<FDebugSnapshot>
    {
        FScopeLock Lock{&_Lock};
        return _Snapshot;
    }

    auto
        FDebugSnapshotCache::
        Clear() -> void
    {
        FScopeLock Lock{&_Lock};
        _Key.Reset();
        _Snapshot.Reset();
    }

    auto
        Append_OctreeDebugSnapshot(
            const FOctree& InOctree,
            int32 InChunkIndex,
            const FDebugSnapshotBuildParams& InParams,
            FDebugSnapshot& InOutSnapshot) -> void
    {
        if (NOT InOctree.Get_IsValid())
        { return; }

        const auto& VolumeBounds = InOctree.Get_VolumeBounds();
        const auto& NavigationBounds = InOctree.Get_NavigationBounds();

        if (InOutSnapshot._AuthoredBounds.IsValid != 0)
        { InOutSnapshot._AuthoredBounds += VolumeBounds; }
        else
        { InOutSnapshot._AuthoredBounds = VolumeBounds; }

        if (InOutSnapshot._NavigationBounds.IsValid != 0)
        { InOutSnapshot._NavigationBounds += NavigationBounds; }
        else
        { InOutSnapshot._NavigationBounds = NavigationBounds; }

        if (ck_voxelnav_debug_snapshot::Get_HasLayer(InParams._RequestedLayers, EDebugSnapshotLayer::MergedFree))
        {
            for (const auto& Cell : InOctree.Get_MergedCells().Get_Cells())
            {
                ck_voxelnav_debug_snapshot::Append_Cell(
                    Cell.Get_Bounds(), InChunkIndex, INDEX_NONE, InParams, InOutSnapshot._MergedFree);
            }
        }

        if (ck_voxelnav_debug_snapshot::Get_HasLayer(InParams._RequestedLayers, EDebugSnapshotLayer::RawFree))
        {
            ck_voxelnav_debug_snapshot::Append_RawCells(
                InOctree, InChunkIndex, InParams, InOutSnapshot._RawFree);
        }

        if (ck_voxelnav_debug_snapshot::Get_HasLayer(InParams._RequestedLayers, EDebugSnapshotLayer::Occupied))
        {
            ck_voxelnav_debug_snapshot::Append_OccupiedCells(
                InOctree, InChunkIndex, InParams, InOutSnapshot._Occupied);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
