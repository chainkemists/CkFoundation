#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldStreaming.h"

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
    struct FCk_GroundNav_StreamRegistryAccess;

    /** World-local lifecycle authority for a streamed data source. It is deliberately opaque and is
     *  never serialized into a field or a tile blob. */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamSourceHandle
    {
    public:
        FCk_GroundNav_StreamSourceHandle() = default;
        auto Get_IsValid() const -> bool { return _Value != 0; }
        auto operator==(const FCk_GroundNav_StreamSourceHandle&) const -> bool = default;

    private:
        friend struct FCk_GroundNav_StreamRegistryAccess;
        explicit FCk_GroundNav_StreamSourceHandle(uint64 InValue) : _Value(InValue) {}
        uint64 _Value = 0;
    };

    enum class ECk_GroundNav_StreamRegistryStatus : uint8
    {
        Published,
        NoChange,
        InvalidWorld,
        InvalidVolumeId,
        InvalidOwner,
        OwnerAlreadyRegistered,
        OwnerNotRegistered,
        OwnerEntryMissing,
        InvalidSource,
        SourceNotFound,
        CoordinateAlreadyOwned,
        CoordinateNotOwned,
        InvalidBundle,
        CompositionRefused,
        ConcurrentChange
    };

    /** Data returned to the lifecycle layer. The registry deliberately performs no engine callbacks. */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamRegistryResult
    {
    public:
        ECk_GroundNav_StreamRegistryStatus _Status = ECk_GroundNav_StreamRegistryStatus::NoChange;
        FCk_GroundNav_StreamSourceHandle _Source;
        FBox _ChangedBounds = FBox{ForceInit};
        FCk_GroundNav_Epoch _Epoch;
        ECk_GroundNav_StreamCompositionStatus _CompositionStatus = ECk_GroundNav_StreamCompositionStatus::Composed;
        ECk_GroundNav_LoadStatus _LoadStatus = ECk_GroundNav_LoadStatus::Loaded;
        FGameplayTag _ProfileTag;

    public:
        auto Get_Succeeded() const -> bool { return _Status == ECk_GroundNav_StreamRegistryStatus::Published; }
        auto Get_HasChanges() const -> bool { return Get_Succeeded(); }
    };

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
     *
     * A COST-ONLY publish moves _Epoch and leaves _LastGeometryEpoch and the accumulated link history
     * alone. A reader still checks whether its saved filter now denies a plate in its corridor.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PublishNote
    {
    public:
        // The epoch the field carried at the newest publish, so a reader can tell whether the note
        // still describes the field it is holding.
        FCk_GroundNav_Epoch _Epoch;

        // The epoch of the last publish that could have moved GROUND. A reader whose own snapshot is
        // older than this has missed ground moving, which no list of link ids describes. A link-only
        // and a cost-only publish both leave it exactly where it is.
        FCk_GroundNav_Epoch _LastGeometryEpoch;

        // Authored, volume-scoped link ids and the publish epoch that changed each one, accumulated
        // since geometry moved. A corridor only owes ids newer than its own epoch.
        TMap<int32, FCk_GroundNav_Epoch> _ChangedLinkEpochsSinceGeometry;
    };

    /** A one-lock copy of a logical streamed owner's current immutable publication and notes. */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamOwnerSnapshot
    {
    public:
        FCk_GroundNav_FieldPtr _DefaultField;
        TMap<FGameplayTag, FCk_GroundNav_FieldPtr> _VariantFields;
        FCk_GroundNav_PublishNote _DefaultPublishNote;
        TMap<FGameplayTag, FCk_GroundNav_PublishNote> _VariantPublishNotes;
        FCk_GroundNav_Epoch _Epoch;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** What a publish CLAIMS it changed. Three kinds, because there are three things a publisher can
     *  honestly say, and the note above records each of them differently. */
    enum class ECk_GroundNav_PublishKind : uint8
    {
        // Ground may have moved: a build, a repair, a cook, the empty registration a volume enters the
        // world on. Nothing narrower can be claimed about it, so it restarts the note's run.
        Geometry,

        // Links were re-resolved and the ids beside this name every one that moved. Extends the run.
        LinkOnly,

        // The same ground at a new price. Moves the epoch and nothing else in the note: a re-priced
        // square is a square that can still be stood on, and a corridor is invalidated by ground
        // going away, not by ground getting dearer.
        CostOnly
    };

    /**
     * One publisher's claim: the kind, and the ids a link-only kind carries.
     *
     * Made through the named constructors rather than assembled field by field, so a claim is always
     * one of the three states and never a kind holding a payload that contradicts it.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PublishClaim
    {
    public:
        static auto Geometry() -> FCk_GroundNav_PublishClaim
        { return FCk_GroundNav_PublishClaim{ECk_GroundNav_PublishKind::Geometry}; }

        static auto CostOnly() -> FCk_GroundNav_PublishClaim
        { return FCk_GroundNav_PublishClaim{ECk_GroundNav_PublishKind::CostOnly}; }

        static auto LinkOnly(TArray<int32> InChangedLinkIds) -> FCk_GroundNav_PublishClaim
        { return FCk_GroundNav_PublishClaim{MoveTemp(InChangedLinkIds)}; }

    public:
        auto Get_Kind() const -> ECk_GroundNav_PublishKind
        { return _Kind; }

        const TArray<int32>& Get_ChangedLinkIds() const
        { return _ChangedLinkIds; }

    private:
        explicit FCk_GroundNav_PublishClaim(ECk_GroundNav_PublishKind InKind)
            : _Kind(InKind)
        {
        }

        explicit FCk_GroundNav_PublishClaim(TArray<int32> InChangedLinkIds)
            : _Kind(ECk_GroundNav_PublishKind::LinkOnly)
            , _ChangedLinkIds(MoveTemp(InChangedLinkIds))
        {
        }

        ECk_GroundNav_PublishKind _Kind;
        TArray<int32> _ChangedLinkIds;
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
     * The CLAIM decides what the note records. A GEOMETRY claim - a build, a repair, a cook, the empty
     * registration a volume enters the world on - restarts the note's run: the ids accumulated so far
     * go, because no list of them describes ground that moved. A LINK-ONLY claim is the link derive
     * saying its publish moved nothing but the links it names, and they are merged into the run rather
     * than replacing it. A COST-ONLY claim is the markup derive saying it re-priced published ground
     * and moved none of it: the epoch advances and the run stands, so a reader that has missed only
     * this is told nothing was taken away from it. Named rather than inferred from whether a list came
     * with the call, so a publisher that changed nothing but price cannot claim a rebuild by saying
     * nothing - which is exactly what the cost derive used to do.
     *
      * Each default or variant field receives a note stamped with its own epoch. A reader compares the
      * note for the same profile it used to plan.
     */
    CKGROUNDNAV_API auto
    Publish(
        UWorld*                                           InWorld,
        const FCk_Handle&                                 InVolumeEntity,
        FCk_GroundNav_FieldPtr                            InField,
        const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InVariantFields,
        const FCk_GroundNav_PublishClaim&                 InClaim) -> void;

    CKGROUNDNAV_API auto
    Publish(
        UWorld*                                           InWorld,
        const FCk_Handle&                                 InVolumeEntity,
        FCk_GroundNav_FieldPtr                            InField,
        const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InVariantFields) -> void;

    // Streaming owns a separate positive-VolumeId sidecar. The legacy handle-keyed methods above
    // intentionally do not consult it, so INDEX_NONE callers retain their existing behavior.
    CKGROUNDNAV_API auto Register_StreamOwner(
        UWorld*                                  InWorld,
        const FCk_Handle&                        InOwnerEntity,
        FCk_GroundNav_VolumeId                   InVolumeId,
        const FCk_GroundNav_StreamFieldBundle&   InInitialBundle) -> FCk_GroundNav_StreamRegistryResult;

    CKGROUNDNAV_API auto Load_StreamSource(
        UWorld*                                             InWorld,
        FCk_GroundNav_VolumeId                              InVolumeId,
        TConstArrayView<FCk_GroundNav_StreamTileTransition> InReplacements) -> FCk_GroundNav_StreamRegistryResult;

    CKGROUNDNAV_API auto Replace_StreamSourceTiles(
        UWorld*                                             InWorld,
        FCk_GroundNav_VolumeId                              InVolumeId,
        FCk_GroundNav_StreamSourceHandle                    InSource,
        TConstArrayView<FCk_GroundNav_StreamTileTransition> InReplacements) -> FCk_GroundNav_StreamRegistryResult;

    /**
     * Atomically reconciles one already-reserved source. Replace transitions may claim coordinates
     * that are unowned or already owned by InSource; remove transitions purge only coordinates that
     * InSource owns. InEnabledCoords is the source's complete desired publication mask after those
     * changes. A coordinate omitted from the mask retains its decoded blob for a later zero-probe
     * re-enable, while a remove transition drops that retained blob and leaves its tile unbuilt.
     *
     * Every replacement is validated as an exact default-plus-variants tile before the transaction
     * can publish. A refusal leaves the sidecar, all immutable field pointers, and their epoch intact.
     */
    CKGROUNDNAV_API auto Upsert_StreamSourceTiles(
        UWorld*                                             InWorld,
        FCk_GroundNav_VolumeId                              InVolumeId,
        FCk_GroundNav_StreamSourceHandle                    InSource,
        TConstArrayView<FCk_GroundNav_StreamTileTransition> InTransitions,
        TConstArrayView<FCk_GroundNav_TileCoord>            InEnabledCoords) -> FCk_GroundNav_StreamRegistryResult;

    CKGROUNDNAV_API auto Disable_StreamSource(
        UWorld*                          InWorld,
        FCk_GroundNav_VolumeId           InVolumeId,
        FCk_GroundNav_StreamSourceHandle InSource) -> FCk_GroundNav_StreamRegistryResult;

    CKGROUNDNAV_API auto Enable_StreamSource(
        UWorld*                          InWorld,
        FCk_GroundNav_VolumeId           InVolumeId,
        FCk_GroundNav_StreamSourceHandle InSource) -> FCk_GroundNav_StreamRegistryResult;

    CKGROUNDNAV_API auto Unload_StreamSource(
        UWorld*                          InWorld,
        FCk_GroundNav_VolumeId           InVolumeId,
        FCk_GroundNav_StreamSourceHandle InSource) -> FCk_GroundNav_StreamRegistryResult;

    /** Drops the sidecar and its owner entry. The caller remains responsible for path cancellation. */
    CKGROUNDNAV_API auto Unregister_StreamOwner(
        UWorld*                InWorld,
        const FCk_Handle&      InOwnerEntity,
        FCk_GroundNav_VolumeId InVolumeId) -> FCk_GroundNav_StreamRegistryResult;

    /** Complete, newly-derived bundle refresh. Every retained source tile is reserialized from the
     *  refreshed bundle; only coordinates owned by an enabled source remain published. */
    CKGROUNDNAV_API auto Refresh_StreamOwnerBundle(
        UWorld*                                InWorld,
        FCk_GroundNav_VolumeId                 InVolumeId,
        const FCk_GroundNav_StreamFieldBundle& InRefreshedBundle,
        const FCk_GroundNav_PublishClaim&      InClaim) -> FCk_GroundNav_StreamRegistryResult;

    CKGROUNDNAV_API auto TryGet_StreamOwnerSnapshot(
        UWorld*                InWorld,
        FCk_GroundNav_VolumeId InVolumeId) -> TOptional<FCk_GroundNav_StreamOwnerSnapshot>;

    /**
      * The note left by the last publish on whichever field TryGet_Field would answer for the same
      * location and profile, or unset when the world has no such field.
     *
      * This is a separate read from TryGet_Field. A publish between the two can leave a note that
      * postdates the field, so callers must compare their epochs before trusting the note.
     */
    CKGROUNDNAV_API auto
    TryGet_PublishNote(
        UWorld*             InWorld,
        const FVector&      InLocation,
        const FGameplayTag& InProfileTag) -> TOptional<FCk_GroundNav_PublishNote>;

    struct CKGROUNDNAV_API FCk_GroundNav_FieldSnapshot
    {
        FCk_GroundNav_FieldPtr _Field;
        FCk_GroundNav_PublishNote _PublishNote;
    };

    CKGROUNDNAV_API auto TryGet_FieldSnapshot(
        UWorld* InWorld, const FVector& InLocation, const FGameplayTag& InProfileTag)
        -> TOptional<FCk_GroundNav_FieldSnapshot>;

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
