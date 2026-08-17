#pragma once

#include <Modules/ModuleManager.h>

class FCkDebugSceneModule final : public IModuleInterface
{
public:
    auto
    StartupModule() -> void override;
    auto
    ShutdownModule() -> void override;

private:
    FDelegateHandle _ModifyCookHandle;
};
