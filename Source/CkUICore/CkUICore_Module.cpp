#include "CkUICore_Module.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkUICoreModule"

auto FCkUICoreModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkUICoreModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkUICoreModule, CkUICore);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
