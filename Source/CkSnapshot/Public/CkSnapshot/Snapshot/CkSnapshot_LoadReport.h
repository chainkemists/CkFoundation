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
    UPROPERTY() TArray<FString>    _SkippedFragmentTypes;
    UPROPERTY() TArray<FString>    _SkippedDynamicTypes;
    UPROPERTY() TArray<FString>    _SkippedScriptClasses;
    UPROPERTY() FCk_Snapshot_Header _LoadedHeader;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY(_EntitiesRestored);
    CK_PROPERTY(_EntitiesOrphaned);
    CK_PROPERTY(_EntitiesPartiallyRestored);
    CK_PROPERTY(_SkippedFragmentTypes);
    CK_PROPERTY(_SkippedDynamicTypes);
    CK_PROPERTY(_SkippedScriptClasses);
    CK_PROPERTY(_LoadedHeader);
};
