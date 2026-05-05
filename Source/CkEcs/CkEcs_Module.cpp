#include "CkEcs_Module.h"

#include "CkEcs/Registry/CkRegistry_SlotTable.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkEcsModule"

auto FCkEcsModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkEcsModule::ShutdownModule() -> void
{
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
