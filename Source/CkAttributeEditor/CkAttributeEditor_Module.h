#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>
#include <AssetRegistry/AssetData.h>

// ----------------------------------------------------------------------------------------------------------------

class FCkAttributeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

// ----------------------------------------------------------------------------------------------------------------