#include "CkEcs_Module.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkEcsModule"

auto FCkEcsModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkEcsModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkEcsModule, CkEcs);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
