#include "CkEditorToolbar_Module.h"

#define LOCTEXT_NAMESPACE "FCkEditorToolbarModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkEditorToolbarModule::
    StartupModule()
    -> void
{
}

auto
    FCkEditorToolbarModule::
    ShutdownModule()
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEditorToolbarModule, CkEditorToolbar)
