#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::SnapshotRegistryType

#include "Containers/Array.h"
#include "Containers/UnrealString.h"

// --------------------------------------------------------------------------------------------------------------------
// Feature-agnostic post-restore invariants.
//
// These exist to make the "a restored handle points nowhere" failure class LOUD instead of silent. The
// canonical instance is the cross-world (seamless-travel) registry-rehome bug: a restored FCk_Handle whose
// entity-id was remapped but whose registry was stale read [INVALID REGISTRY], silently breaking
// Get_LifetimeOwner and every downstream consumer. A generic post-restore sweep catches that class without
// any per-feature gate.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Walk the structural handle-storing fragments (FFragment_LifetimeOwner / FFragment_ContextOwner /
    // FFragment_LifetimeDependents) and return a human-readable description of every stored handle that
    // points at an entity which does NOT exist in InRegistry (a dangling reference). Unset / tombstone
    // handles are skipped (they are not references). An EMPTY result means the restored entity graph's
    // structural backbone is internally consistent.
    //
    // Intended callers: snapshot tests (assert empty after a restore) and, in non-shipping builds, the
    // restore path itself as a defense-in-depth check.
    CKSNAPSHOT_API auto
    Verify_AllStoredHandlesResolve(
        ck::SnapshotRegistryType& InRegistry) -> TArray<FString>;
}
