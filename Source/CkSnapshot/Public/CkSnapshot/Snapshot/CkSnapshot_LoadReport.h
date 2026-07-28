#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "CkSnapshot_LoadReport.generated.h"

UENUM(BlueprintType)
enum class ECk_SnapshotResult : uint8
{
    Success,
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
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_SkipReason);

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_OrphanRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_OrphanRecord);

private:
    UPROPERTY() uint32                     _SavedId = 0xFFFFFFFFu;
    UPROPERTY() ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
    // label / script-or-actor class path / save-key / player id — whichever the provenance carries.
    UPROPERTY() FString                    _Identity;
    UPROPERTY() uint32                     _OwnerSavedId = 0xFFFFFFFFu;
    // Reason bucket string — the set is enumerated in CkSnapshot/CLAUDE.md.
    UPROPERTY() FString                    _Reason;

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
    UPROPERTY() uint32                     _SavedId = 0xFFFFFFFFu;
    UPROPERTY() ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
    // label / script-or-actor class path / save-key / player id — whichever the provenance carries.
    UPROPERTY() FString                    _Identity;
    UPROPERTY() uint32                     _OwnerSavedId = 0xFFFFFFFFu;
    UPROPERTY() ECk_Snapshot_SkipReason    _Reason = ECk_Snapshot_SkipReason::ClassUnloadable;

public:
    CK_PROPERTY(_SavedId);
    CK_PROPERTY(_Provenance);
    CK_PROPERTY(_Identity);
    CK_PROPERTY(_OwnerSavedId);
    CK_PROPERTY(_Reason);
};

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_LoadReport
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_LoadReport);

private:
    UPROPERTY() ECk_SnapshotResult _Result = ECk_SnapshotResult::Failed_IO;
    // Rows in the save's entity table. Restored + Skipped + Orphaned partitions it exactly.
    UPROPERTY() int32              _EntitiesTotal = 0;
    UPROPERTY() int32              _EntitiesRestored = 0;
    UPROPERTY() int32              _EntitiesSkipped = 0;
    UPROPERTY() int32              _EntitiesOrphaned = 0;
    UPROPERTY() int32              _EntitiesPartiallyRestored = 0;
    // Rows in the save's payload table. Enqueued + OnSkipped + OnOrphaned + OnUnresolvedOwner + Dropped
    // partitions it exactly.
    UPROPERTY() int32              _PayloadsTotal = 0;
    UPROPERTY() int32              _PayloadsEnqueued = 0;
    // Payloads whose owner entity the loader deliberately did not rebuild — see _Skips for the per-entity reason.
    UPROPERTY() int32              _PayloadsOnSkippedEntities = 0;
    // Payloads whose owner entity never mapped and was not skipped — see _Orphans.
    UPROPERTY() int32              _PayloadsOnOrphanedEntities = 0;
    // Payloads whose owner saved-id is neither mapped-to-a-live-handle, skipped, nor orphaned: an owner id absent
    // from the entity table, or one mapped to a handle that has since gone invalid.
    UPROPERTY() int32              _PayloadsOnUnresolvedOwner = 0;
    // Payloads that failed to deserialize: empty bytes, or a type absent since the save (content drift).
    UPROPERTY() int32              _PayloadsDropped = 0;
    UPROPERTY() TArray<FString>    _SkippedFragmentTypes;
    UPROPERTY() TArray<FString>    _SkippedDynamicTypes;
    UPROPERTY() TArray<FString>    _SkippedScriptClasses;
    UPROPERTY() TArray<FCk_Snapshot_OrphanRecord> _Orphans;
    UPROPERTY() TArray<FCk_Snapshot_SkipRecord>   _Skips;
    UPROPERTY() FCk_Snapshot_Header _LoadedHeader;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY(_EntitiesTotal);
    CK_PROPERTY(_EntitiesRestored);
    CK_PROPERTY(_EntitiesSkipped);
    CK_PROPERTY(_EntitiesOrphaned);
    CK_PROPERTY(_EntitiesPartiallyRestored);
    CK_PROPERTY(_PayloadsTotal);
    CK_PROPERTY(_PayloadsEnqueued);
    CK_PROPERTY(_PayloadsOnSkippedEntities);
    CK_PROPERTY(_PayloadsOnOrphanedEntities);
    CK_PROPERTY(_PayloadsOnUnresolvedOwner);
    CK_PROPERTY(_PayloadsDropped);
    CK_PROPERTY(_SkippedFragmentTypes);
    CK_PROPERTY(_SkippedDynamicTypes);
    CK_PROPERTY(_SkippedScriptClasses);
    CK_PROPERTY(_Orphans);
    CK_PROPERTY(_Skips);
    CK_PROPERTY(_LoadedHeader);

public:
    /** True when every saved entity landed in exactly one of restored / skipped / orphaned. */
    auto Get_IsEntityAccountingClosed() const -> bool;

    /** True when every saved payload landed in exactly one of enqueued / on-skipped / on-orphaned /
     *  on-unresolved-owner / dropped. */
    auto Get_IsPayloadAccountingClosed() const -> bool;

    /** Both of the above. A load report that does not close has an unaccounted saved entity or payload. */
    auto Get_IsAccountingClosed() const -> bool;
};
