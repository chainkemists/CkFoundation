#include "CkIskmRenderer_Module.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkIskmRendererModule"

void FCkIskmRendererModule::StartupModule()
{
    // ---- Plan-2 batched-skinning shader directory ----
    // CkIskmRenderer is a *module inside* the CkFoundation plugin (not its own .uplugin), so we resolve
    // the CkFoundation plugin base dir then append Source/CkIskmRenderer/Shaders/CkIskmRenderer.
    // Mirrors CkUsf_Module.cpp. A .usf/.ush then includes "/CkIskmRenderer/<file>.ush".
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkIskmRenderer"), TEXT("Shaders"), TEXT("CkIskmRenderer"));
        AddShaderSourceDirectoryMapping(TEXT("/CkIskmRenderer"), ShaderDir);
    }
}

void FCkIskmRendererModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkIskmRendererModule, CkIskmRenderer)
