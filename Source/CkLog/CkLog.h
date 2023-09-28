#pragma once

#include "CkLog/CkLog_Utils.h"

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CKLOG_API DECLARE_LOG_CATEGORY_EXTERN(CkLogger, Log, All);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::log
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
