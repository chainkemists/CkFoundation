#include "CkCueEditor_Module.h"

#define LOCTEXT_NAMESPACE "FCkCueEditorModule"

auto
    FCkCueEditorModule::
    StartupModule()
    -> void
{
}

auto
    FCkCueEditorModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkCueEditorModule, CkCueEditor)
