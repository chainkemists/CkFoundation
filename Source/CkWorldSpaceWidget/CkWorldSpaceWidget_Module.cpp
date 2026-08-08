#include "CkWorldSpaceWidget_Module.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkWorldSpaceWidgetModule"

auto FCkWorldSpaceWidgetModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkWorldSpaceWidgetModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkWorldSpaceWidgetModule, CkWorldSpaceWidget);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
