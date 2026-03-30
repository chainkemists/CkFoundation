#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include <EdGraphUtilities.h>

class FCkCVarEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FGraphPanelPinFactory> _CVarRefPinFactory;
};
