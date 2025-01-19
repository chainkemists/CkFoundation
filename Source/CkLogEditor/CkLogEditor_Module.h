#pragma once

#include "CkLogEditor/CkLog_CategoryGraphPanelPinFactory.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

class FCkLogEditorModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FCk_LogCategory_GraphPanelPinFactory> _GraphPanelPinFactory;
};

// --------------------------------------------------------------------------------------------------------------------
