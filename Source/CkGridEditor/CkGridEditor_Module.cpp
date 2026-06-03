#include "CkGridEditor_Module.h"

#include "CkGridEditor_Log.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FCkGridEditorModule"

// --------------------------------------------------------------------------------------------------------------------

void FCkGridEditorModule::StartupModule()
{
    // Grid Paint UEdMode registration is added in a later task.
    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module started"));
}

void FCkGridEditorModule::ShutdownModule()
{
    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module shut down"));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkGridEditorModule, CkGridEditor)
