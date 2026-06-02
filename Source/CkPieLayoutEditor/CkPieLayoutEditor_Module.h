#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkPieLayoutEditorModule : public IModuleInterface
{
public:
    CK_GENERATED_BODY(FCkPieLayoutEditorModule);

public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

private:
    auto DoRegisterToolsMenu() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
