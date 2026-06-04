#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FComponentVisualizer;

// --------------------------------------------------------------------------------------------------------------------

class FCkGridEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // Registers the grid-spawner component visualizer with GUnrealEd. Deferred to OnPostEngineInit
    // because this module loads at the "Default" phase, before GUnrealEd exists.
    auto RegisterSpawnerVisualizer() -> void;

private:
    TSharedPtr<FComponentVisualizer> _SpawnerVisualizer;
    FName                            _SpawnerVisualizerName;
    FDelegateHandle                  _PostEngineInitHandle;
};

// --------------------------------------------------------------------------------------------------------------------
