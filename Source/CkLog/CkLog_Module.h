#pragma once

#include "CkLog/CkLog_Utils.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CKLOG_API DECLARE_LOG_CATEGORY_EXTERN(CkLogger, Log, All);

// --------------------------------------------------------------------------------------------------------------------

class CKLOG_API FCkLogModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};
