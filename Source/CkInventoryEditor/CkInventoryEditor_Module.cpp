#include "CkInventoryEditor_Module.h"

#define LOCTEXT_NAMESPACE "FCkInventoryEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkInventoryEditorModule::
    StartupModule()
    -> void
{
}

auto
    FCkInventoryEditorModule::
    ShutdownModule()
    -> void
{
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkInventoryEditorModule, CkInventoryEditor)
