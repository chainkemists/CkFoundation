#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

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
     * What one entry has published since the last publish that could have moved GROUND, for a reader
     * that has to narrow past the bounds the neutral rebuild queue carries.
     *
     * A GroundNav-side sidecar rather than a widening of that queue: a changed-link-id channel is a
     * concept exactly one provider has, and every other provider would have to carry it to nowhere.
     * It rides the world-keyed entry, so it is dropped with the entry and nothing here is
     * process-wide.
     *
     * It describes a RUN rather than one publish, because two publishes can land between a reader's
     * snapshot and its read: a repair and a link derive in the same tick leave the queue holding the
     * repair's box under the note the derive wrote, and two toggles leave the second one's ids naming
     * half of what moved. Carrying the epoch the last geometry publish went out under, and
     * accumulating every link-only publish since it, lets a reader decide from its OWN epoch whether
     * this note accounts for everything it has missed.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PublishNote
    {
    public:
        // The epoch the field carried at the newest publish, so a reader can tell whether the note
        // still describes the field it is holding.
        FCk_GroundNav_Epoch _Epoch;

        // The epoch of the last publish that was not link-only. A reader whose own snapshot is older
        // than this has missed ground moving, which no list of link ids describes.
        FCk_GroundNav_Epoch _LastGeometryEpoch;

        // Authored, volume-scoped link ids, accumulated over every link-only publish since that
        // geometry publish and sorted. Emptied by the next one.
        TArray<int32> _ChangedLinkIdsSinceGeometry;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Records (or replaces) everything one volume publishes: its untagged DEFAULT field, and the field
     * it baked for each authored profile variant, keyed by that variant's tag. GAME THREAD: called from
     * the volume's build processor at the moment it swaps its published pointers.
     *
     * One call under ONE write lock, which is the whole of the atomicity a cross-thread reader gets: a
     * default and the variants baked beside it describe one world, and a second entry point for the
     * variants would let a reader see one of them without the other.
     *
     * The variant map REPLACES what was published rather than merging into it, so a volume that dropped
     * a variant - or never had one - leaves nothing behind under a tag nobody authors any more. A tag
     * that goes, and a tag whose field is swapped, retire their tile-epoch sums the way an unpublished
     * volume's do, so the surface revision never falls across the change.
     *
     * Saying NOTHING for the changed ids is a geometry publish - a build, a repair, the empty
     * registration a volume enters the world on - and restarts the note's run: the ids accumulated so
     * far go, because no list of them describes ground that moved. Naming ids is the link derive's claim
     * that its publish moved nothing else, and they are merged into the run rather than replacing it.
     * The two states are an unset and a set optional rather than a flag beside a list, so "link-only,
     * and this is what moved" is the only shape a claim to narrowing can be made in.
     *
     * The epochs the note carries are the PUBLISHED DEFAULT FIELD'S own, never a caller's and never a
     * variant's: a note stamped with an epoch the field beside it does not carry is a note no reader
     * could account for.
     */
    CKGROUNDNAV_API auto
    Publish(
        UWorld*                                           InWorld,
        const FCk_Handle&                                 InVolumeEntity,
        FCk_GroundNav_FieldPtr                            InField,
        const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InVariantFields,
        const TOptional<TArray<int32>>&                   InLinkOnlyChangedLinkIds = {}) -> void;

    /**
     * The note left by the last publish on whichever entry TryGet_Field would answer from for the same
     * location, or unset when the world has no field there.
     *
 * Two separate read locks rather than one call answering both, because the pointer handoff is all
 * this lock is entitled to cover, and a combined accessor would tempt a caller into holding it
 * across a query. A publish landing between the two reads leaves the caller with a note that
 * postdates its field, which is why a reader compares the two epochs before trusting one.
     */
    CKGROUNDNAV_API auto
    TryGet_PublishNote(
        UWorld*        InWorld,
        const FVector& InLocation) -> TOptional<FCk_GroundNav_PublishNote>;

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
     * The tile-epoch sums of every PROFILE VARIANT field the world currently holds, added up.
     *
     * Separate from Get_Fields rather than folded into it, because that view answers "the fields a
     * location could be projected onto" - a count, a bounds union - and a variant field covers the very
     * same ground its default does. It is only the surface REVISION that has to see them: a change that
     * moved a variant and left the default alone is still a change, and a revision blind to it would
     * tell a watcher the surface stood still.
     */
    CKGROUNDNAV_API auto
    Get_VariantRevision(
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

    /**
     * The same field for an EMPTY profile tag, and the entry's variant field for a tag it holds one for.
     *
     * NEVER falls back. A tag the containing volume authored no variant for answers null, so a query
     * naming a profile is answered by that profile's ground or by nothing - silently handing back the
     * default would let an agent that cannot climb a step walk one, which is the whole failure a
     * variant exists to prevent.
     *
     * The ENTRY is chosen by the same rule the two-argument read uses - the volume whose default field
     * contains the location, or the first there is - so a location resolves to one volume and the tag
     * then selects within it. A variant field's own bounds are the default's by construction.
     *
     * Callable from any thread, on the same terms.
     */
    CKGROUNDNAV_API auto
    TryGet_Field(
        UWorld*             InWorld,
        const FVector&      InLocation,
        const FGameplayTag& InProfileTag) -> FCk_GroundNav_FieldPtr;

    /** Every non-null DEFAULT field registered for the world, copied out. Profile-variant fields are
     *  not in here: they cover the ground their default does, and this view answers where ground is.
     *  Their epochs reach a watcher through Get_VariantRevision. */
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
