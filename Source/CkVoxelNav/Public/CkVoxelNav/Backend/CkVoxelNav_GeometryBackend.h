#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Opaque identity of one piece of world geometry, minted by the backend that returned it. The
 * octree never decodes one — it compares ids and hands them back. Zero is the never-a-body
 * sentinel.
 */
struct CKVOXELNAV_API FCk_VoxelNav_BodyId
{
    uint64 _Value = 0;

    auto Get_IsValid() const -> bool { return _Value != 0; }
    auto operator==(const FCk_VoxelNav_BodyId&) const -> bool = default;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * The ONLY world-geometry surface the voxelizer sees. Everything a bake learns about the world
 * arrives through these two calls, which is what keeps the octree free of any physics backend and
 * lets a hand-authored box list stand in for a whole level in a test.
 *
 * Occupancy is decided against STATIC geometry only: a bake is a statement about immovable world
 * obstacles, and a moving obstacle is a steering problem, not a bake input.
 *
 * Game thread only, whole type (off-thread only under a future step-barrier contract — not yet
 * provided).
 */
class CKVOXELNAV_API ICk_VoxelNav_GeometryBackend
{
public:
    virtual ~ICk_VoxelNav_GeometryBackend() = default;

    /**
     * Does any geometry overlap the axis-aligned box of InHalfExtents centered at InCenter? Any-hit
     * with early-out — this is the bake's hot primitive, issued tens of thousands of times per
     * volume, so it must never collect, sort or attribute its hits.
     *
     * InHalfExtents is ALREADY inflated by whatever agent clearance the caller wants: clearance is
     * the rasterizer's business, never the backend's.
     *
     * Game thread only (off-thread only under a future step-barrier contract — not yet provided).
     */
    virtual auto
    Get_IsBoxOccupied(
        const FVector& InCenter,
        const FVector& InHalfExtents) const -> bool = 0;

    /**
     * Every body whose BOUNDING BOX overlaps InWorldBounds — broadphase only, so the answer is
     * conservative and cheap. A bake spends exactly one of these, on the whole volume: an empty
     * result means there is nothing to rasterize and the volume is free space. OutBodies is reset
     * before filling.
     *
     * Game thread only (off-thread only under a future step-barrier contract — not yet provided).
     */
    virtual auto
    Get_BodiesInBox(
        const FBox& InWorldBounds,
        TArray<FCk_VoxelNav_BodyId>& OutBodies) const -> void = 0;
};

// --------------------------------------------------------------------------------------------------------------------
