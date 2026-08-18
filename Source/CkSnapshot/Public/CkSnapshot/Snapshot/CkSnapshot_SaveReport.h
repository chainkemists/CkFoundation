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

// A live entity that produced a payload the save did not take, because the entity itself is not captured: it was
// created at runtime with no construction recipe and no identity to rendezvous on (capture rule 5).
//
// This is DATA, not a defect report. Under C5 the sanctioned shape for runtime-created framework state — the
// timer an SM state starts, the one a request handler arms — is that it does NOT persist: the durable intent is
// the deadline, held by the owning feature, and the feature's Setup re-creates the timer from it. What was wrong
// before was only that the loss was invisible: nothing counted it, nothing named it, and an author had no way to
// tell "deliberately session" from "silently dropped". A save now says which, and the answer is auditable
// afterwards rather than needing a breakpoint at capture time.
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_UncapturedRuntimeRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_UncapturedRuntimeRecord);

private:
    // The entity, formatted the way every other snapshot diagnostic names one.
    UPROPERTY()
    FString _Identity;

    // The first save-participating type that produced a payload for it — enough to say WHAT was not carried.
    UPROPERTY()
    FString _PayloadType;

public:
    CK_PROPERTY(_Identity);
    CK_PROPERTY(_PayloadType);
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

    // Runtime-created entities that produced a payload the capture did not take. Under C5 that is usually the
    // DESIGNED outcome, so this is a census rather than an error list — but a census that exists, which is the
    // whole difference from before.
    UPROPERTY()
    TArray<FCk_Snapshot_UncapturedRuntimeRecord> _UncapturedRuntimeEntities;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY_GET(_Losses);
    CK_PROPERTY_GET(_UncapturedRuntimeEntities);

public:
    auto Add_Loss(FCk_Snapshot_SaveLossRecord InRecord) -> void;
    auto Add_UncapturedRuntimeEntity(FCk_Snapshot_UncapturedRuntimeRecord InRecord) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
