#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkEditorToolbarModule : public IModuleInterface
{
public:
    CK_GENERATED_BODY(FCkEditorToolbarModule);

public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
