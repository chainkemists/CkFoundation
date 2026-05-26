#include "CkEcs_Module.h"

#include "CkEcs/DependencyInjection/CkDependencyProvider_InjectionCache.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkEcsModule"

auto FCkEcsModule::StartupModule() -> void
{
    // The DI injection-plan cache keys plans on UClass* via TWeakObjectPtr.
    // Native hot reload (Live Coding) reinstances UClasses; cached entries
    // referencing the old UClass become orphaned. AS hot reload is handled
    // lazily by GetOrBuild itself (miss → rebuild) plus opportunistic
    // dead-weak-ptr pruning, but for Live Coding we drop the whole map so
    // any recompiled native EntityScript rebuilds its plan on next spawn.
    _ReloadCompleteHandle = FCoreUObjectDelegates::ReloadCompleteDelegate.AddLambda(
        [](EReloadCompleteReason)
        { FCk_InjectionCache::InvalidateAll(); });

    return IModuleInterface::StartupModule();
}

auto FCkEcsModule::ShutdownModule() -> void
{
    if (_ReloadCompleteHandle.IsValid())
    {
        FCoreUObjectDelegates::ReloadCompleteDelegate.Remove(_ReloadCompleteHandle);
        _ReloadCompleteHandle.Reset();
    }

    // Drop the cache before the module unloads so any late UClass GC won't
    // touch our static map.
    FCk_InjectionCache::InvalidateAll();

    // Flip the slot table's "alive" sentinel BEFORE Super::ShutdownModule
    // returns. After this call, Free()/Resolve()/TryResolve() are safe
    // no-ops for any UObject destructors that fire later in the DLL
    // teardown sequence — the phoenix singleton's whole purpose.
    ck::registry_table::ShutdownTable();

    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkEcsModule, CkEcs);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
