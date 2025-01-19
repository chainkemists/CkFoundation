#pragma once

#include "CkLog/CkLog_Utils.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

class CKECS_API FCkEcsModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------

