#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

class CKWORLDSPACEWIDGET_API FCkWorldSpaceWidgetModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
