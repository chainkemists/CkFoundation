#pragma once

#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

class FCkVoxelNavEditorModule : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
