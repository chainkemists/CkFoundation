#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCkProfileModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void OnBeginFrame();
    void OnEndFrame();

#if WITH_ANGELSCRIPT_CK
    FDelegateHandle _PreCompileDelegateHandle;
    FDelegateHandle _PostCompileDelegateHandle;
#endif
};
