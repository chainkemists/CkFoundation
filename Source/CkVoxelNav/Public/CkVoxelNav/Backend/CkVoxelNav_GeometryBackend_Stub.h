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
struct FCk_VoxelNav_GeometryBackend_Stub final : public ICk_VoxelNav_GeometryBackend
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

    auto
    Get_IsSegmentBlocked(
        const FVector& InFrom,
        const FVector& InTo) const -> bool override
    {
        return _Obstacles.ContainsByPredicate([&](const FBox& InObstacle) -> bool
        { return DoGet_SegmentIntersectsBox(InFrom, InTo, InObstacle); });
    }

private:
    /** Slab test in the segment's own parameter space, so [0,1] bounds the answer to the segment rather
     *  than the infinite line. A zero-length segment degenerates to point containment, which is the same
     *  answer Get_IsBoxOccupied gives for a zero half-extent. */
    static auto
    DoGet_SegmentIntersectsBox(
        const FVector& InFrom,
        const FVector& InTo,
        const FBox& InBox) -> bool
    {
        auto TEnter = 0.0;
        auto TExit = 1.0;

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto Origin = InFrom[Axis];
            const auto Delta = InTo[Axis] - InFrom[Axis];

            if (FMath::IsNearlyZero(Delta))
            {
                if (Origin < InBox.Min[Axis] || Origin > InBox.Max[Axis])
                { return false; }

                continue;
            }

            const auto InverseDelta = 1.0 / Delta;

            auto TNear = (InBox.Min[Axis] - Origin) * InverseDelta;
            auto TFar = (InBox.Max[Axis] - Origin) * InverseDelta;

            if (TNear > TFar)
            { Swap(TNear, TFar); }

            TEnter = FMath::Max(TEnter, TNear);
            TExit = FMath::Min(TExit, TFar);

            if (TEnter > TExit)
            { return false; }
        }

        return true;
    }

private:
    TArray<FBox> _Obstacles;

public:
    auto Get_Obstacles() const -> const TArray<FBox>& { return _Obstacles; }
};

// --------------------------------------------------------------------------------------------------------------------
