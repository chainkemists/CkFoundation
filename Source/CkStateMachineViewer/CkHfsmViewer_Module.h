#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewer_Window;

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    auto
    OpenViewer() -> void;

    TSharedPtr<FCkHfsmViewer_Window> _Window;
};

// --------------------------------------------------------------------------------------------------------------------
