#pragma once

#include "CkJolt/StaticWorld/CkJoltStaticWorld_CookedQuery.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav_editor
{
    /** Exact cooked-Jolt adapter for the engine-free VoxelNav builder. The synthesized body ids are used only
     *  for the builder's one broadphase count; no Jolt identity crosses this boundary. */
    class CKVOXELNAVEDITOR_API FCookedGeometryBackend final : public ICk_VoxelNav_GeometryBackend
    {
    public:
        explicit FCookedGeometryBackend(
            const ck::jolt::FCk_Jolt_CookedWorldQuery& InQuery);

        auto Get_IsValid() const -> bool;

        auto
        Get_IsBoxOccupied(
            const FVector& InCenter,
            const FVector& InHalfExtents) const -> bool override;

        auto
        Get_BodiesInBox(
            const FBox& InWorldBounds,
            TArray<FCk_VoxelNav_BodyId>& OutBodies) const -> void override;

        auto
        Get_IsSegmentBlocked(
            const FVector& InFrom,
            const FVector& InTo) const -> bool override;

    private:
        const ck::jolt::FCk_Jolt_CookedWorldQuery* _Query = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------
