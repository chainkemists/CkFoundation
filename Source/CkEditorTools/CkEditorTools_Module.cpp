#include "CkEditorTools_Module.h"

#include "CkEditorTools/Style/CkIconStyle.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkEditorToolsModule::
    StartupModule()
    -> void
{
    FCkIconStyle::Initialize();
}

auto
    FCkEditorToolsModule::
    ShutdownModule()
    -> void
{
    FCkIconStyle::Shutdown();
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FCkEditorToolsModule, CkEditorTools)
