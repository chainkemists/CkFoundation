#include "CkVfxEditor_Module.h"

#define LOCTEXT_NAMESPACE "FCkVfxEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkVfxEditorModule::
    StartupModule()
    -> void
{
}

auto
    FCkVfxEditorModule::
    ShutdownModule()
    -> void
{
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkVfxEditorModule, CkVfxEditor)