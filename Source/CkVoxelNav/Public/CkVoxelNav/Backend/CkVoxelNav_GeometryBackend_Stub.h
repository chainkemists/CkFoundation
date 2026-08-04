#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Test double: the geometry interface over a hand-authored list of world-space boxes, so a bake can
 * be driven to completion with no physics world, no level and no subsystems — pure math, and the
 * expected occupancy of every cell is computable by hand.
 *
 * Occupancy is exact AABB-vs-AABB intersection, and FBox::Intersect counts a shared face as an
 * intersection, so a cell placed flush against an obstacle face reads as occupied. Author obstacle
 * layouts that clearly straddle or clear cell boundaries; a layout that touches one exactly is
 * asserting a tie-break rather than the behavior under test.
 *
 * No thread affinity of its own — it reads nothing but its own obstacle list.
 */
struct CKVOXELNAV_API FCk_VoxelNav_GeometryBackend_Stub final : public ICk_VoxelNav_GeometryBackend
{
public:
    FCk_VoxelNav_GeometryBackend_Stub() = default;

    explicit FCk_VoxelNav_GeometryBackend_Stub(
        TArray<FBox> InObstacles)
        : _Obstacles(MoveTemp(InObstacles))
    { }

    auto
    Get_IsBoxOccupied(
        const FVector& InCenter,
        const FVector& InHalfExtents) const -> bool override
    {
        const auto QueryBox = FBox{InCenter - InHalfExtents, InCenter + InHalfExtents};

        return _Obstacles.ContainsByPredicate([&](const FBox& InObstacle) -> bool
        { return InObstacle.Intersect(QueryBox); });
    }

    auto
    Get_BodiesInBox(
        const FBox& InWorldBounds,
        TArray<FCk_VoxelNav_BodyId>& OutBodies) const -> void override
    {
        OutBodies.Reset();

        for (auto ObstacleIndex = 0; ObstacleIndex < _Obstacles.Num(); ++ObstacleIndex)
        {
            if (NOT _Obstacles[ObstacleIndex].Intersect(InWorldBounds))
            { continue; }

            // Ids are the obstacle index offset by one, so that zero stays the never-a-body
            // sentinel and a test can map an id straight back to the box it authored.
            OutBodies.Emplace(FCk_VoxelNav_BodyId{static_cast<uint64>(ObstacleIndex) + 1});
        }
    }

private:
    TArray<FBox> _Obstacles;

public:
    CK_PROPERTY_GET(_Obstacles);
};

// --------------------------------------------------------------------------------------------------------------------
