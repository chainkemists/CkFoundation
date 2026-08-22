#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::FSnapshotContext (+ transitively FCk_Handle)

#include "Templates/Function.h" // TFunctionRef

// --------------------------------------------------------------------------------------------------------------------

class FArchive;
class UScriptStruct;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Routes every FCk_Handle-DERIVED field inside InStruct — top-level, nested, and inside TArray/TSet/TMap
    // with load-side rehash — through FSnapshotContext::Snapshot_Handle. Save and load MUST visit the same
    // handles in the same order; the single deterministic TFieldIterator walk is what guarantees that.
    CKECS_API auto
    RemapHandles(
        const UScriptStruct* InStruct,
        void* InMemory,
        FArchive& InAr,
        ck::FSnapshotContext& InCtx) -> void;

    // Visit-only counterpart — same walk order as RemapHandles, serializing and mutating nothing. Backs the
    // capture's forward-reference guard: a recipe's spawn-params handles must already be in the entity table.
    CKECS_API auto
    ForEachHandle(
        const UScriptStruct* InStruct,
        void* InMemory,
        const TFunctionRef<void(FCk_Handle&)>& InVisitor) -> void;

    // The AUDIT counterpart: same traversal, but it hands the visitor a dotted field path, and at an
    // FInstancedStruct boundary it descends only into a DURABLE payload. Both differences are why it is a separate
    // entry point rather than a flag on the two above — the remap walk must stay posture-blind (its handle-id
    // stream is positional, so save and load have to visit the same slots), and it must not pay for paths.
    CKECS_API auto
    ForEachDurableHandle(
        const UScriptStruct* InStruct,
        void* InMemory,
        const TFunctionRef<void(FCk_Handle&, const FString&)>& InVisitor) -> void;

    // The OBJECT-reference half of the same audit, and a SEPARATE walk on purpose. The three entry points above
    // share one traversal because they share one contract: the save's handle-id stream is positional, so what
    // they visit cannot change without both sides of a save/load disagreeing about slot i. Teaching that walk to
    // also visit object properties would put a format-affecting change in the path of every payload on every save
    // AND load, to serve a check that only ever runs at capture. This one visits nothing the stream depends on.
    //
    // It answers the sibling of the durable-handle question: a durable payload may name an OBJECT that no load
    // will bring back — the runtime-built material instance, the transient proxy — and the field comes back as a
    // path resolving to nothing. The visitor is handed the object AS IT IS AT CAPTURE, which is the only moment
    // the truth exists: here a runtime object is still alive and can be asked what it is, whereas at load all
    // that survives is a path that failed, and 'was never an asset' is indistinguishable from 'asset deleted'.
    //
    // Soft references that simply are not LOADED resolve null and are never handed over — an unloaded asset path
    // is valid and will resolve after a load, so flagging it would be a false positive.
    CKECS_API auto
    ForEachDurableObjectRef(
        const UScriptStruct* InStruct,
        void* InMemory,
        const TFunctionRef<void(const UObject&, const FString&)>& InVisitor) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
