#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCkEntityVisualizerModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};
