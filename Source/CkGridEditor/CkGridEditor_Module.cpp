#include "CkGridEditor_Module.h"

#include "CkGridEditor_Log.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"
#include "CkGridEditor/Visualizer/Ck2dGridSystem_SpawnerVisualizer.h"

#include "Components/SceneComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "UnrealEdGlobals.h"

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

    // The out-of-mode grid preview is a component visualizer registered on GUnrealEd. GUnrealEd does not
    // exist yet at the "Default" loading phase, so defer registration to OnPostEngineInit.
    if (GUnrealEd != nullptr)
    {
        RegisterSpawnerVisualizer();
    }
    else
    {
        _PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
            this, &FCkGridEditorModule::RegisterSpawnerVisualizer);
    }

    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module started (Grid Paint mode auto-discovered)"));
}

void FCkGridEditorModule::RegisterSpawnerVisualizer()
{
    if (GUnrealEd == nullptr)
    { return; }

    // Idempotent: OnPostEngineInit may fire after a direct StartupModule registration in some configs.
    if (_SpawnerVisualizer.IsValid())
    { return; }

    _SpawnerVisualizerName = USceneComponent::StaticClass()->GetFName();
    _SpawnerVisualizer     = MakeShareable(new FCk_2dGridSystem_SpawnerVisualizer());

    GUnrealEd->RegisterComponentVisualizer(_SpawnerVisualizerName, _SpawnerVisualizer);
    _SpawnerVisualizer->OnRegister();
}

void FCkGridEditorModule::ShutdownModule()
{
    if (_PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(_PostEngineInitHandle);
        _PostEngineInitHandle.Reset();
    }

    if (GUnrealEd != nullptr && ! _SpawnerVisualizerName.IsNone())
    {
        GUnrealEd->UnregisterComponentVisualizer(_SpawnerVisualizerName);
    }
    _SpawnerVisualizer.Reset();

    // The auto-discovered UEdMode is torn down by UAssetEditorSubsystem on OnEnginePreExit
    // (UnregisterEditorModes) — nothing to undo for it here.
    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module shut down"));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkGridEditorModule, CkGridEditor)
