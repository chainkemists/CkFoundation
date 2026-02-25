#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCkEcsEditorModule : public IModuleInterface
{
public:
	auto StartupModule() -> void override;
	auto ShutdownModule() -> void override;

private:
	auto DoRegisterTabSpawner() -> void;
	auto DoUnregisterTabSpawner() -> void;
	auto DoOnSpawnTab(const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>;

private:
	static constexpr auto SpawnParamsToolbox_TabName = "CkEntityScriptSpawnParamsToolbox";
	static constexpr auto SpawnParamsToolbox_TabDisplayName = "EntityScript SpawnParams Toolbox";
};
