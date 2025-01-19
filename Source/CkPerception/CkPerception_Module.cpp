#include "CkPerception_Module.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkPerceptionModule"

auto FCkPerceptionModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkPerceptionModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkPerceptionModule, CkPerception);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
