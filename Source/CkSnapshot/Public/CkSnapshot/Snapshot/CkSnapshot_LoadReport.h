#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "CkSnapshot_LoadReport.generated.h"

// Answers ONE question — did the operation complete? — because that is what a caller branches on. The two
// Succeeded_* values both mean the world is loaded and playable; the Failed_* values mean it is not. A caller
// that cares about the difference reads the report's buckets, and one that does not calls Get_DidLoadComplete.
UENUM(BlueprintType)
enum class ECk_SnapshotResult : uint8
{
    // Completed; every payload applied.
    Success,
    // Completed; NAMED payloads did not apply (dropped, no handler, unresolved, or released by a quarantine
    // escape) and the world is playable without them. Reusing a Failed_* here would make a lossy-but-fine load
    // indistinguishable from one that never loaded at all.
    Succeeded_WithLoss,
    // There was no load to report on — the honest answer for a promise that fires immediately outside a load,
    // which used to be handed a default-constructed report and therefore said Failed_IO.
    NoLoadInProgress,

    Failed_IO,
    Failed_Corrupt,
    Failed_IncompatibleSave,
    Failed_NotQuiescent,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SnapshotResult);

// Why the loader deliberately did NOT rebuild a saved entity. One bucket per skip site in DoRebuild_Tick.
UENUM(BlueprintType)
enum class ECk_Snapshot_SkipReason : uint8
{
    // The recipe's actor or script class no longer resolves (content drift / deleted asset).
    ClassUnloadable,
    // SpawnActor returned null for a bridged RuntimeSpawned entity.
    SpawnFailed,
    // Owner is the world root/transient (never persisted) and the script did not opt into snapshot respawn —
    // the fresh world's boot owns this entity, so rebuilding it would duplicate it.
    NonPersistedOwnerNotRespawnable,
    // DefinitionBuilt entity carries neither a persisted context owner nor a lifetime owner to build under.
    NoOwnerRecipe,
    // DefinitionBuilt entity's owner was not persisted, so there is nothing to rebuild it under.
    OwnerNotPersisted,
    // Every construction step of a DefinitionBuilt recipe failed to load.
    NoLoadableSteps,
    // The build request returned an invalid handle (host gate / rep-driver rejection).
    BuildFailed,
    // A keyed bridged row resolved to a live entity another saved row had already claimed — a second saved
    // copy of the same logical entity (e.g. a statue player captured by an earlier polluted save). Skipped
    // so distinct saved rows never consolidate onto one live entity.
    DuplicateSaveKey,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_SkipReason);

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_OrphanRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_OrphanRecord);

private:
    UPROPERTY()
    uint32 _SavedId = 0xFFFFFFFFu;

    UPROPERTY()
    ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;

    // label / script-or-actor class path / save-key / player id — whichever the provenance carries.
    UPROPERTY()
    FString _Identity;

    UPROPERTY()
    uint32 _OwnerSavedId = 0xFFFFFFFFu;

    // Reason bucket string — the set is enumerated in CkSnapshot/CLAUDE.md.
    UPROPERTY()
    FString _Reason;

public:
    CK_PROPERTY(_SavedId);
    CK_PROPERTY(_Provenance);
    CK_PROPERTY(_Identity);
    CK_PROPERTY(_OwnerSavedId);
    CK_PROPERTY(_Reason);
};

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_SkipRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_SkipRecord);

private:
    UPROPERTY()
    uint32 _SavedId = 0xFFFFFFFFu;

    UPROPERTY()
    ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;

    // label / script-or-actor class path / save-key / player id — whichever the provenance carries.
    UPROPERTY()
    FString _Identity;

    UPROPERTY()
    uint32 _OwnerSavedId = 0xFFFFFFFFu;

    UPROPERTY()
    ECk_Snapshot_SkipReason _Reason = ECk_Snapshot_SkipReason::ClassUnloadable;

public:
    CK_PROPERTY(_SavedId);
    CK_PROPERTY(_Provenance);
    CK_PROPERTY(_Identity);
    CK_PROPERTY(_OwnerSavedId);
    CK_PROPERTY(_Reason);
};

// One record per entity the loader had to force out of the hydration quarantine before the load's own machinery
// released it. Reaching either escape means the settle could not finish on its own terms, so the entity is named
// rather than counted: "some payloads did not apply" without saying WHICH entity is the silence this exists to break.
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_QuarantineForcedRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_QuarantineForcedRecord);

private:
    // The entity, formatted the way every other snapshot diagnostic names one.
    UPROPERTY()
    FString _Identity;

    // Payload entries still queued on it when the escape fired — the size of the loss, not just its existence.
    UPROPERTY()
    int32 _PayloadsOutstanding = 0;

    // Which escape released it: "hydrate-frame-cap" or "load-finish".
    UPROPERTY()
    FString _Reason;

public:
    CK_PROPERTY(_Identity);
    CK_PROPERTY(_PayloadsOutstanding);
    CK_PROPERTY(_Reason);
};

// One payload that did not apply, named. A count says the world came back incomplete; only a name says which
// part of it did — and that is the difference between a bug report and a shrug.
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_PayloadLossRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_PayloadLossRecord);

private:
    UPROPERTY()
    FString _PayloadType;

    UPROPERTY()
    FString _OwnerIdentity;

    // "no-handler" | "rejected" | "timed-out" | "destroyed-with-entries"
    UPROPERTY()
    FString _Reason;

public:
    CK_PROPERTY(_PayloadType);
    CK_PROPERTY(_OwnerIdentity);
    CK_PROPERTY(_Reason);
};

// One convergence fact the load waited on and never got, named. Same reason as every other record here: a count
// says the world was handed back before it was coherent; only a name says which part of it had not settled.
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_ConvergenceLossRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_ConvergenceLossRecord);

private:
    // The registered predicate's name — the same name the owning module registered it under.
    UPROPERTY()
    FName _Name;

    // Convergence frames burned before the bounded escape gave up on it.
    UPROPERTY()
    int32 _FramesWaited = 0;

public:
    CK_PROPERTY(_Name);
    CK_PROPERTY(_FramesWaited);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_LoadReport
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_LoadReport);

private:
    UPROPERTY()
    ECk_SnapshotResult _Result = ECk_SnapshotResult::Failed_IO;

    // Rows in the save's entity table. Restored + Skipped + Orphaned partitions it exactly.
    UPROPERTY()
    int32 _EntitiesTotal = 0;

    UPROPERTY()
    int32 _EntitiesRestored = 0;

    UPROPERTY()
    int32 _EntitiesSkipped = 0;

    UPROPERTY()
    int32 _EntitiesOrphaned = 0;

    // Rows in the save's payload table. Applied + Rejected + DroppedNoHandler + DroppedTimeout +
    // DestroyedWithEntries + UnappliedAtFinish + OnSkipped + OnOrphaned + OnUnresolvedOwner + Dropped
    // partitions it exactly — the closure asks what HAPPENED to each row, not how far it got.
    UPROPERTY()
    int32 _PayloadsTotal = 0;

    // Diagnostic, NOT a closure term: how many rows reached the apply queue at all. A row counted here is
    // still counted again by whichever apply bucket it ends in, so it deliberately does not partition.
    UPROPERTY()
    int32 _PayloadsEnqueued = 0;

    // ---- Apply outcomes, folded once from ck::FCtx_HydrationOutcomes when the load finishes -----------------
    UPROPERTY()
    int32 _PayloadsApplied = 0;

    // The handler ran and refused the data.
    UPROPERTY()
    int32 _PayloadsRejected = 0;

    // No handler resolved, or the one that did never declared a HydrationApply: the save recorded state this
    // build cannot apply.
    UPROPERTY()
    int32 _PayloadsDroppedNoHandler = 0;

    // The handler kept answering NotReady until the apply timeout.
    UPROPERTY()
    int32 _PayloadsDroppedTimeout = 0;

    // Queued on an entity that entered destruction before they applied.
    UPROPERTY()
    int32 _PayloadsDestroyedWithEntries = 0;

    // Still queued when the load finished. The dispatcher is not load-gated, so some of these may apply a frame
    // later — the report is a snapshot of the load, and this is the honest name for what was still in flight.
    UPROPERTY()
    int32 _PayloadsUnappliedAtFinish = 0;

    // Payloads whose owner entity the loader deliberately did not rebuild — see _Skips for the per-entity reason.
    UPROPERTY()
    int32 _PayloadsOnSkippedEntities = 0;

    // Payloads whose owner entity never mapped and was not skipped — see _Orphans.
    UPROPERTY()
    int32 _PayloadsOnOrphanedEntities = 0;

    // Payloads whose owner saved-id is neither mapped-to-a-live-handle, skipped, nor orphaned: an owner id absent
    // from the entity table, or one mapped to a handle that has since gone invalid.
    UPROPERTY()
    int32 _PayloadsOnUnresolvedOwner = 0;

    // Payloads that failed to deserialize: empty bytes, or a type absent since the save (content drift).
    UPROPERTY()
    int32 _PayloadsDropped = 0;

    UPROPERTY()
    TArray<FCk_Snapshot_OrphanRecord> _Orphans;

    UPROPERTY()
    TArray<FCk_Snapshot_SkipRecord> _Skips;

    // The rebuild kernel quiesced with unresolved rows and the loader ran full-scope ticks to let
    // multi-stage constructions (and the identity they stamp on completion) finish. Not a failure.
    UPROPERTY()
    bool _UsedEscalatedRebuild = false;

    // Rows that stayed unresolved even after the escalated full scope quiesced — real losses (content
    // drift, or an identity the fresh world never re-creates). Per-row detail is in _Orphans.
    UPROPERTY()
    int32 _UnresolvedAfterEscalation = 0;

    // Entities the hydration quarantine's bounded escape had to release with payloads still outstanding. Empty on
    // a healthy load. It does NOT participate in the accounting closure: those partition the SAVE's rows, and this
    // records a LIVE-side release. Nor does it move _Result — a lossy load still reports the Result it does today.
    UPROPERTY()
    TArray<FCk_Snapshot_QuarantineForcedRecord> _QuarantineForced;

    // The apply-side losses, named. Like _QuarantineForced this does NOT participate in the closure — the buckets
    // above count them; this says which they were. Capped, so a pathological load cannot grow it without bound.
    UPROPERTY()
    TArray<FCk_Snapshot_PayloadLossRecord> _PayloadLosses;

    // Convergence facts still Pending when the convergence phase hit its frame cap and the loader handed the
    // world back anyway. Empty on a healthy load. Like the two above it is outside the accounting closure —
    // those partition the SAVE's rows, and this records a LIVE-side fact — but unlike them it DOES move _Result:
    // a world resumed before its physics, overlaps or queues settled is a world the player watches finish
    // loading, which is the one thing the hold exists to prevent, so it says so instead of passing as Success.
    UPROPERTY()
    TArray<FCk_Snapshot_ConvergenceLossRecord> _ConvergenceUnmet;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY(_EntitiesTotal);
    CK_PROPERTY(_EntitiesRestored);
    CK_PROPERTY(_EntitiesSkipped);
    CK_PROPERTY(_EntitiesOrphaned);
    CK_PROPERTY(_PayloadsTotal);
    CK_PROPERTY(_PayloadsEnqueued);
    CK_PROPERTY(_PayloadsApplied);
    CK_PROPERTY(_PayloadsRejected);
    CK_PROPERTY(_PayloadsDroppedNoHandler);
    CK_PROPERTY(_PayloadsDroppedTimeout);
    CK_PROPERTY(_PayloadsDestroyedWithEntries);
    CK_PROPERTY(_PayloadsUnappliedAtFinish);
    CK_PROPERTY(_PayloadsOnSkippedEntities);
    CK_PROPERTY(_PayloadsOnOrphanedEntities);
    CK_PROPERTY(_PayloadsOnUnresolvedOwner);
    CK_PROPERTY(_PayloadsDropped);
    CK_PROPERTY(_Orphans);
    CK_PROPERTY(_Skips);
    CK_PROPERTY(_UsedEscalatedRebuild);
    CK_PROPERTY(_UnresolvedAfterEscalation);
    CK_PROPERTY(_QuarantineForced);
    CK_PROPERTY(_PayloadLosses);
    CK_PROPERTY(_ConvergenceUnmet);

public:
    /**
     * Did the load COMPLETE? — the question almost every consumer is actually asking, and the one a bare
     * `== Success` comparison answers wrongly the moment a completed load can also report losses. A world that
     * came back playable with named payloads missing is loaded; a world that never came back is not. Branch on
     * this, and read the individual buckets only when the difference matters to the caller.
     */
    auto Get_DidLoadComplete() const -> bool;

    /** True when every saved entity landed in exactly one of restored / skipped / orphaned. */
    auto Get_IsEntityAccountingClosed() const -> bool;

    /** True when every saved payload landed in exactly one TERMINAL bucket: applied, rejected, dropped for want
     *  of a handler or at the apply timeout, destroyed with its entity, still unapplied when the load finished,
     *  or never enqueued at all (owner skipped / orphaned / unresolved, or the blob failed to deserialize).
     *  Enqueued is not a term — it says a row reached the queue, not what became of it. */
    auto Get_IsPayloadAccountingClosed() const -> bool;

    /** Both of the above. A load report that does not close has an unaccounted saved entity or payload. */
    auto Get_IsAccountingClosed() const -> bool;
};
