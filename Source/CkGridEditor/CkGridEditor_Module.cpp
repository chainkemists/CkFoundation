#include "CkGridEditor_Module.h"

#include "CkGridEditor_Log.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FCkGridEditorModule"

// --------------------------------------------------------------------------------------------------------------------

void FCkGridEditorModule::StartupModule()
{
    // The "Grid Paint" UEdMode (UCk_2dGridSystem_EdMode) is registered by AUTO-DISCOVERY: at
    // OnAllModuleLoadingPhasesComplete, UAssetEditorSubsystem::RegisterEditorModes() iterates every
    // non-abstract UEdMode CDO and registers it by its FEditorModeInfo::ID. There is no explicit
    // RegisterMode() call for UEdMode-derived modes (FEditorModeRegistry::RegisterMode is for legacy
    // FEdMode only). This module loads at the "Default" phase — before that delegate fires — so the
    // CDO already exists when discovery runs. We force-reference StaticClass() here so the linker
    // cannot strip the translation unit (and thus the CDO) from this module.
    (void)UCk_2dGridSystem_EdMode::StaticClass();

    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module started (Grid Paint mode auto-discovered)"));
}

void FCkGridEditorModule::ShutdownModule()
{
    // Nothing to unregister: auto-discovered UEdModes are torn down by UAssetEditorSubsystem on
    // OnEnginePreExit (UnregisterEditorModes). We added no manual registration to undo.
    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module shut down"));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkGridEditorModule, CkGridEditor)
