#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCkThirdParty : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual auto StartupModule() -> void override;
	virtual auto ShutdownModule() -> void override;
};