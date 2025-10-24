#include "CkObjectiveEditor_Module.h"

#define LOCTEXT_NAMESPACE "FCkObjectiveEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkObjectiveEditorModule::
    StartupModule()
    -> void
{
}

auto
    FCkObjectiveEditorModule::
    ShutdownModule()
    -> void
{
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkObjectiveEditorModule, CkObjectiveEditor)