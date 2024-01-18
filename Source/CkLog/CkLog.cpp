#include "CkLog.h"

#include <PropertyEditorModule.h>

// --------------------------------------------------------------------------------------------------------------------

DEFINE_LOG_CATEGORY(CkLogger);

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkLogModule"

auto
    FCkLogModule::
    StartupModule()
    -> void
{
}

auto
    FCkLogModule::
    ShutdownModule()
    -> void
{
}

IMPLEMENT_MODULE(FCkLogModule, CkLog);

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
