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
}

// --------------------------------------------------------------------------------------------------------------------
