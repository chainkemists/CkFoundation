#include "CkWidgets_Module.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkWidgetsModule"

auto FCkWidgetsModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkWidgetsModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkWidgetsModule, CkWidgets);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
