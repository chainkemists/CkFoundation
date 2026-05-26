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

private:
    // Handle for the FCoreUObjectDelegates::ReloadCompleteDelegate binding that
    // drops the DI injection-plan cache on native hot reload (Live Coding).
    // Cleared in ShutdownModule so the delegate doesn't outlive the module.
    FDelegateHandle _ReloadCompleteHandle;
};

// --------------------------------------------------------------------------------------------------------------------

