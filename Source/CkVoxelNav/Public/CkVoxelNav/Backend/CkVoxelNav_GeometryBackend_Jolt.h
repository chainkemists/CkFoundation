#pragma once

#include "CkJolt/Query/CkJoltOccupancy_Session.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * The shipped backend: CkJolt's static-body occupancy surface behind the geometry interface. The
 * Jolt world is resolved ONCE at construction and one box shape is kept per distinct half-extent,
 * because a bake issues tens of thousands of identically-sized queries and both the subsystem
 * lookup and the shape build would otherwise be paid per query.
 *
 * An INVALID backend answers every query "unoccupied". That is a legitimate state — a world with no
 * Jolt subsystem is not an error — so whoever owns the bake MUST gate on Get_IsValid() and fail its
 * build loudly, rather than accepting an empty world as free space.
 *
 * Move-only, and not safe for concurrent const calls: the shape cache is filled on demand.
 * Game thread only, whole type (off-thread only under a future step-barrier contract — not yet
 * provided).
 */
struct CKVOXELNAV_API FCk_VoxelNav_GeometryBackend_Jolt final : public ICk_VoxelNav_GeometryBackend
{
public:
    /** A null world context is a programming error and ensures; an unresolvable Jolt subsystem is
     *  not, and yields an INVALID backend. */
    explicit FCk_VoxelNav_GeometryBackend_Jolt(
        const UObject* InWorldContextObject);

    auto
    Get_IsValid() const -> bool;

    auto
    Get_IsBoxOccupied(
        const FVector& InCenter,
        const FVector& InHalfExtents) const -> bool override;

    auto
    Get_BodiesInBox(
        const FBox& InWorldBounds,
        TArray<FCk_VoxelNav_BodyId>& OutBodies) const -> void override;

private:
    auto
    DoGet_Probe(
        const FVector& InHalfExtents) const -> const ck::jolt::FCk_Jolt_BoxProbe&;

private:
    ck::jolt::FCk_Jolt_QuerySession _Session;

    // Filled on first use per size and never evicted: a bake queries a handful of distinct cell
    // sizes and reuses each for thousands of queries, so the cache stays at a few entries and a
    // linear scan beats hashing.
    mutable TArray<ck::jolt::FCk_Jolt_BoxProbe> _ProbesByHalfExtent;
};

// --------------------------------------------------------------------------------------------------------------------
