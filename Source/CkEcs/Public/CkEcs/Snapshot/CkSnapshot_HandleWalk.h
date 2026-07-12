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
    // Route every FCk_Handle-DERIVED field inside InStruct (top-level, nested structs, and inside TArray/TSet/TMap,
    // with load-side rehash) through FSnapshotContext::Snapshot_Handle. Save writes each handle's entity id; load
    // reads it back through the continuous-loader remap. Save and load MUST visit the same handles in the same order
    // -- a single deterministic TFieldIterator walk guarantees that, and the container element counts/order are
    // materialized by the preceding data pass before this runs on load. Handle detection is IsChildOf (not exact),
    // so C++-native typed handles (FCk_Handle_Item, ...) are covered as well as AngelScript dynamic handles.
    //
    // Lifted verbatim (behavior-identical) from ck_dynamic_snapshot::RemapHandles so both the dynamic-fragment
    // serialize path AND the v3 save capture share one incident-hardened walker (the typed-handle tombstone fix).
    CKECS_API auto
    RemapHandles(
        const UScriptStruct* InStruct,
        void* InMemory,
        FArchive& InAr,
        ck::FSnapshotContext& InCtx) -> void;

    // Visit-only counterpart: invoke InVisitor on every FCk_Handle-derived field (same walk order as RemapHandles),
    // serializing nothing and mutating nothing. Used by the v3 capture's forward-reference guard — each handle a
    // recipe's spawn params carry must reference an entity already written to the save's entity table.
    CKECS_API auto
    ForEachHandle(
        const UScriptStruct* InStruct,
        void* InMemory,
        const TFunctionRef<void(FCk_Handle&)>& InVisitor) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
