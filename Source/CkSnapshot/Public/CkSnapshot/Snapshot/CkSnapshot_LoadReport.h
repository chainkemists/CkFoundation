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
struct CKSNAPSHOT_API FCk_Snapshot_LoadReport
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_LoadReport);

private:
    UPROPERTY() ECk_SnapshotResult _Result = ECk_SnapshotResult::Failed_IO;
    UPROPERTY() int32              _EntitiesRestored = 0;
    UPROPERTY() int32              _EntitiesOrphaned = 0;
    UPROPERTY() int32              _EntitiesPartiallyRestored = 0;
    // Payloads that failed to deserialize: empty bytes, or a type absent since the save (content drift).
    UPROPERTY() int32              _PayloadsDropped = 0;
    UPROPERTY() TArray<FString>    _SkippedFragmentTypes;
    UPROPERTY() TArray<FString>    _SkippedDynamicTypes;
    UPROPERTY() TArray<FString>    _SkippedScriptClasses;
    UPROPERTY() TArray<FCk_Snapshot_OrphanRecord> _Orphans;
    UPROPERTY() FCk_Snapshot_Header _LoadedHeader;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY(_EntitiesRestored);
    CK_PROPERTY(_EntitiesOrphaned);
    CK_PROPERTY(_EntitiesPartiallyRestored);
    CK_PROPERTY(_PayloadsDropped);
    CK_PROPERTY(_SkippedFragmentTypes);
    CK_PROPERTY(_SkippedDynamicTypes);
    CK_PROPERTY(_SkippedScriptClasses);
    CK_PROPERTY(_Orphans);
    CK_PROPERTY(_LoadedHeader);
};
