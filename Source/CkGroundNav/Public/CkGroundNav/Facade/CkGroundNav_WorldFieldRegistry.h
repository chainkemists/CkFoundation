#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------
// Which fields a WORLD has, so a query that knows only a world can find one.
//
// The bake and every query below it are free of world and entity concepts on purpose, and the volume
// that owns a field is reachable only through the ECS. Between the two sits this: a published field
// registered against its world, so the provider adapter — which is handed a UWorld* and nothing else,
// sometimes off the game thread — can resolve one without touching the ECS registry.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::world_fields
{
    /**
     * Records (or replaces) the field one volume publishes. GAME THREAD: called from the volume's
     * build processor at the moment it swaps its published pointer.
     */
    CKGROUNDNAV_API auto
    Publish(
        UWorld*                InWorld,
        const FCk_Handle&      InVolumeEntity,
        FCk_GroundNav_FieldPtr InField) -> void;

    /**
     * Forgets the volume's entry. GAME THREAD: called from the volume's end-play processor, so a
     * field never outlives the volume that published it - a query on a world answers only from
     * fields a live volume still stands behind. The field's tile-epoch sum is kept as RETIRED
     * revision (below), so the world's surface revision never falls when a volume goes away.
     */
    CKGROUNDNAV_API auto
    Unpublish(
        UWorld*           InWorld,
        const FCk_Handle& InVolumeEntity) -> void;

    /**
     * The tile-epoch sums of every field the world has unpublished, added up. A surface revision is
     * the sum over live fields PLUS this, which is what keeps it monotone across a volume's teardown:
     * ground that went away still counts as having moved, and a consumer holding the old number sees
     * a change rather than a fall.
     */
    CKGROUNDNAV_API auto
    Get_RetiredRevision(
        UWorld* InWorld) -> int64;

    /**
     * The field whose bounds contain the location, or the first registered one when none does, or a
     * null pointer when the world has no field at all.
     *
     * Callable from any thread: the caller leaves with its own reference to an immutable field.
     */
    CKGROUNDNAV_API auto
    TryGet_Field(
        UWorld*        InWorld,
        const FVector& InLocation) -> FCk_GroundNav_FieldPtr;

    /** Every non-null field registered for the world, copied out. */
    CKGROUNDNAV_API auto
    Get_Fields(
        UWorld* InWorld) -> TArray<FCk_GroundNav_FieldPtr>;

    CKGROUNDNAV_API auto
    Get_FieldCount(
        UWorld* InWorld) -> int32;

    /**
     * The volume entities that published the world's fields, copied out. GAME THREAD ONLY at the point
     * of USE — the handles are copied out under the lock, but resolving one touches the ECS registry.
     */
    CKGROUNDNAV_API auto
    Get_VolumeEntities(
        UWorld* InWorld) -> TArray<FCk_Handle>;
}

// --------------------------------------------------------------------------------------------------------------------
