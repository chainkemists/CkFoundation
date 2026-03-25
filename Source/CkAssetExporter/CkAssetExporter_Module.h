#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkAssetExporterModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

private:
    auto DoRegisterContentBrowserExtension() -> void;

    auto DoRegisterTabSpawner() -> void;
    auto DoUnregisterTabSpawner() -> void;
    auto DoOnSpawnTab(const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>;

private:
    static constexpr auto ExporterTab_TabName = "CkAssetExporterTab";
    static constexpr auto ExporterTab_TabDisplayName = "Asset Exporter";
};

// --------------------------------------------------------------------------------------------------------------------
