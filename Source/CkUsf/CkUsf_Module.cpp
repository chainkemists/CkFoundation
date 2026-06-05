#include "CkUsf_Module.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkUsfModule"

void FCkUsfModule::StartupModule()
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkUsf"), TEXT("Shaders"), TEXT("CkUsf"));
        AddShaderSourceDirectoryMapping(TEXT("/CkUsf"), ShaderDir);
    }
}

void FCkUsfModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkUsfModule, CkUsf)
