#pragma once

#include "CkDependencyProvider_Common.h"

#include "HAL/CriticalSection.h"

#include "UObject/Class.h"
#include "UObject/WeakObjectPtr.h"

// --------------------------------------------------------------------------------------------------------------------

// Per-UClass lazy plan of dependency-injection sites. Built once on first
// Construct of a given EntityScript class, then served from a TMap on every
// subsequent spawn. Eliminates TFieldRange<FStructProperty> per-tick cost on
// the deferred-construction hot path.
//
// Thread safety: editor-side hot reload calls InvalidateAll on a separate
// thread from any Construct path. The internal FCriticalSection guards the
// map for both reads and writes.
class CKECS_API FCk_InjectionCache
{
public:
    // Returns the cached plan for the EntityScript class, building it on first
    // use. The returned reference is stable for the lifetime of the cache
    // entry (i.e. until InvalidateAll is called).
    static auto GetOrBuild(UClass* InScriptClass) -> const TArray<FCk_InjectionSite>&;

    // Editor-only: drop all cached plans. Wired to hot-reload / module-reload
    // delegates so a recompiled EntityScript re-builds its plan on next spawn.
    static auto InvalidateAll() -> void;

    // Diagnostic — exposed for the Phase 0 spike test and the Phase 2
    // validator. Returns a fresh copy, not the cached entry.
    static auto DoBuildPlan_ForTesting(UClass* InScriptClass) -> TArray<FCk_InjectionSite>;

private:
    static TMap<TWeakObjectPtr<UClass>, TArray<FCk_InjectionSite>> _Plans;
    static FCriticalSection                                        _PlansLock;

    static auto DoBuildPlan(UClass* InScriptClass) -> TArray<FCk_InjectionSite>;
};
