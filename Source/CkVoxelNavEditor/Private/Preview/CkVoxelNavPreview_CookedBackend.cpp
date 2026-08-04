#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_CookedBackend.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav_editor
{
    FCookedGeometryBackend::
        FCookedGeometryBackend(
            const ck::jolt::FCk_Jolt_CookedWorldQuery& InQuery)
        : _Query{&InQuery}
    {
    }

    auto FCookedGeometryBackend::Get_IsValid() const -> bool
    {
        return _Query != nullptr && _Query->Get_IsReady();
    }

    auto
        FCookedGeometryBackend::
        Get_IsBoxOccupied(
            const FVector& InCenter,
            const FVector& InHalfExtents) const
        -> bool
    {
        return Get_IsValid() && _Query->Get_IsBoxOccupied(InCenter, InHalfExtents);
    }

    auto
        FCookedGeometryBackend::
        Get_BodiesInBox(
            const FBox& InWorldBounds,
            TArray<FCk_VoxelNav_BodyId>& OutBodies) const
        -> void
    {
        OutBodies.Reset();

        if (NOT Get_IsValid())
        { return; }

        const auto Count = _Query->Get_BroadphaseBodyCount(InWorldBounds);
        OutBodies.Reserve(Count);
        for (auto Index = 0; Index < Count; ++Index)
        { OutBodies.Emplace(FCk_VoxelNav_BodyId{static_cast<uint64>(Index) + 1ULL}); }
    }

    auto
        FCookedGeometryBackend::
        Get_IsSegmentBlocked(
            const FVector& InFrom,
            const FVector& InTo) const
        -> bool
    {
        return Get_IsValid() && _Query->Get_IsSegmentBlocked(InFrom, InTo);
    }
}

// --------------------------------------------------------------------------------------------------------------------
