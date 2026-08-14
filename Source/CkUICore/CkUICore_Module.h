#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

class CKUICORE_API FCkUICoreModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
