#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h" // ECk_SnapshotResult

#include "CkSnapshot_SaveReport.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// One durable value the save could not carry, named at the moment the fact exists. A handle's target is a runtime
// property of the TARGET ENTITY, not of the field's type, so no type-level fence can predict this — the capture is
// the earliest point at which it is knowable, and the save is the last point at which anyone can act on it.
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_SaveLossRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_SaveLossRecord);

private:
    // Reflected path of the payload type whose walk found it.
    UPROPERTY()
    FString _PayloadType;

    // Dotted field path from the payload root to the offending handle, container indices included.
    UPROPERTY()
    FString _FieldPath;

    UPROPERTY()
    uint32 _OwnerSavedId = 0xFFFFFFFFu;

    // The entity the handle names, and its label if it carries one — the two things a fix needs.
    UPROPERTY()
    uint32 _TargetEntityId = 0xFFFFFFFFu;

    UPROPERTY()
    FString _TargetIdentity;

    UPROPERTY()
    FString _Reason;

public:
    CK_PROPERTY(_PayloadType);
    CK_PROPERTY(_FieldPath);
    CK_PROPERTY(_OwnerSavedId);
    CK_PROPERTY(_TargetEntityId);
    CK_PROPERTY(_TargetIdentity);
    CK_PROPERTY(_Reason);
};

// --------------------------------------------------------------------------------------------------------------------

// The save-side twin of FCk_Snapshot_LoadReport, deliberately minimal: a result plus the losses the capture named.
// The public save entry points keep returning a bare ECk_SnapshotResult, so no caller has to change; a consumer
// that wants the detail pulls the last report off the subsystem.
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_SaveReport
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_SaveReport);

private:
    UPROPERTY()
    ECk_SnapshotResult _Result = ECk_SnapshotResult::Failed_IO;

    UPROPERTY()
    TArray<FCk_Snapshot_SaveLossRecord> _Losses;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY_GET(_Losses);

public:
    auto Add_Loss(FCk_Snapshot_SaveLossRecord InRecord) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
