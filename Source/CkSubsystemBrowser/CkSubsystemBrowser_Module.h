#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkSubsystemBrowserModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

private:
    auto DoRegisterTabSpawner() -> void;
    auto DoUnregisterTabSpawner() -> void;
    auto DoOnSpawnTab(const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>;

private:
    static constexpr auto SubsystemBrowser_TabName = "CkSubsystemBrowserTab";
    static constexpr auto SubsystemBrowser_TabDisplayName = "Ck Subsystem Browser";
};

// --------------------------------------------------------------------------------------------------------------------
