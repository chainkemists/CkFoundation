#include "CkIskmRendererVF_Module.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkIskmRendererVFModule"

void FCkIskmRendererVFModule::StartupModule()
{
    // Map the Plan-2 batched-skinning shader directory. Virtual path kept as "/CkIskmRenderer".
    // Physical dir now lives in this (PostConfigInit) module: <CkFoundation>/Source/CkIskmRendererVF/Shaders/CkIskmRenderer.
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkIskmRendererVF"), TEXT("Shaders"), TEXT("CkIskmRenderer"));
        AddShaderSourceDirectoryMapping(TEXT("/CkIskmRenderer"), ShaderDir);
    }
}

void FCkIskmRendererVFModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkIskmRendererVFModule, CkIskmRendererVF)
