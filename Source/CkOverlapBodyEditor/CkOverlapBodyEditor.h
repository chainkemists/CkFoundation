#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCkOverlapBodyEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	auto
    StartupModule() -> void override;

	auto
    ShutdownModule() -> void override;

private:
	bool _TryReloading = true;
    FTimerHandle _CallbackTimer;
};
