#pragma once

#include "Modules/ModuleManager.h"

class FCkWebUmgEditorModule : public IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;
};
