#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_CueEditorModule : public IModuleInterface
{
public:
    static constexpr auto ModuleName = "CkCueToolbox";

public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

private:
    auto DoRegisterTabSpawner() -> void;
    auto DoUnregisterTabSpawner() -> void;
    auto DoOnSpawnTab(const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>;

private:
    static constexpr auto TabName = "CkCueToolbox";
    static constexpr auto TabDisplayName = "Cue Toolbox";
};

// --------------------------------------------------------------------------------------------------------------------
