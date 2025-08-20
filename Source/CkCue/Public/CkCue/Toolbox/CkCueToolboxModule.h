#pragma once

#include <Modules/ModuleManager.h>
#include <Framework/Docking/TabManager.h>
#include <Widgets/Docking/SDockTab.h>

// --------------------------------------------------------------------------------------------------------------------

class CKCUE_API FCkCueToolboxModule : public IModuleInterface
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