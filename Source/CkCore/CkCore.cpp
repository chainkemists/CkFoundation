#include "CkCore.h"

#define LOCTEXT_NAMESPACE "FCkCoreModule"

auto FCkCoreModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkCoreModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkCoreModule, CkCore);

#undef LOCTEXT_NAMESPACE
