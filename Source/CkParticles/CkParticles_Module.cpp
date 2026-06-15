#include "CkParticles_Module.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkParticlesModule"

void FCkParticlesModule::StartupModule()
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkParticles"), TEXT("Shaders"), TEXT("CkParticles"));
        AddShaderSourceDirectoryMapping(TEXT("/CkParticles"), ShaderDir);
    }
}

void FCkParticlesModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkParticlesModule, CkParticles)
