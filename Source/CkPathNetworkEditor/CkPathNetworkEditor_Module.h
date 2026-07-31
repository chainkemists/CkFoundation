#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkPathNetworkEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
    auto
    DoRegisterToolsMenu() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
