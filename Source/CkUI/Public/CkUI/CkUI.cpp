#include "CkUI.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkUIModule"

auto FCkUIModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkUIModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkUIModule, CkUI);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
