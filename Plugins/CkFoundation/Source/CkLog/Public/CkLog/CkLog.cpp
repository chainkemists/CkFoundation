#include "CkLog.h"

// --------------------------------------------------------------------------------------------------------------------

DEFINE_LOG_CATEGORY(CkLogger);

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkLogModule"

auto FCkLogModule::StartupModule() -> void
{
    return IModuleInterface::StartupModule();
}

auto FCkLogModule::ShutdownModule() -> void
{
    return IModuleInterface::ShutdownModule();
}

IMPLEMENT_MODULE(FCkLogModule, CkLog);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
