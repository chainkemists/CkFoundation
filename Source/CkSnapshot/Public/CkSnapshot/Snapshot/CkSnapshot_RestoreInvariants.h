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
    // Walk the structural handle-storing fragments and return a human-readable description of every stored
    // handle that is inconsistent. Two tiers, matching the framework's own ref semantics:
    //   - FFragment_LifetimeOwner / FFragment_ContextOwner are HARD refs — they must resolve to a live entity;
    //     a dangling one is flagged (the canonical registry-rehome bug: a remapped entity-id against a stale
    //     registry read [INVALID REGISTRY], silently breaking Get_LifetimeOwner and every downstream consumer).
    //   - FFragment_LifetimeDependents is a LAZILY-PRUNED WEAK-ref list — destruction does not remove the entry
    //     (CkEntityLifetime_Utils.cpp:153-155; consumers filter via ck::IsValid), so a STALE entry is by design,
    //     not a bug. This leg instead asserts BACK-POINTER CONSISTENCY: a still-live dependent's LifetimeOwner
    //     must name this owner. A strict resolve here would false-positive on every EntityScript world (each
    //     spawn leaves a destroyed spawn-request child in its parent's dependents).
    // Unset / tombstone handles are skipped. An EMPTY result means the restored entity graph's structural
    // backbone is internally consistent.
    //
    // Intended callers: snapshot tests (assert empty after a restore) and, in non-shipping builds, the
    // restore path itself as a defense-in-depth check.
    CKSNAPSHOT_API auto
    Verify_AllStoredHandlesResolve(
        ck::SnapshotRegistryType& InRegistry) -> TArray<FString>;
}
