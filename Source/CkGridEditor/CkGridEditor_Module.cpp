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
    // The UEdMode registers by AUTO-DISCOVERY (UAssetEditorSubsystem::RegisterEditorModes iterates every
    // non-abstract UEdMode CDO at OnAllModuleLoadingPhasesComplete) — there is no RegisterMode() call for
    // UEdMode. This StaticClass() reference exists only so the linker cannot strip the TU, and with it the CDO.
    (void)UCk_2dGridSystem_EdMode::StaticClass();

    // GUnrealEd does not exist yet at the "Default" loading phase, so defer registration when it is absent.
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

    // The auto-discovered UEdMode is unregistered by UAssetEditorSubsystem on OnEnginePreExit.
    UE_LOG(CkGridEditor, Log, TEXT("CkGridEditor module shut down"));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkGridEditorModule, CkGridEditor)
