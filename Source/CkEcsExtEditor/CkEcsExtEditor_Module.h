#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCkEcsExtEditorModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
