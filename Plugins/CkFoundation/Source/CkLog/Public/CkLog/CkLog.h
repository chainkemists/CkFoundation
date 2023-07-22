#pragma once

#include "CkLog_Utils.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CKLOG_API DECLARE_LOG_CATEGORY_EXTERN(CkLogger, Log, All);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_LOG_FUNCTIONS(CkLogger);
    CK_REGISTER_LOG_FUNCTIONS(CkLogger);
}

// --------------------------------------------------------------------------------------------------------------------

class CKLOG_API FCkLogModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};
