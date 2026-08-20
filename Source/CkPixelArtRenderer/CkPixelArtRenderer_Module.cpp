#include "CkPixelArtRender_Module.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkPixelArtRenderModule"

void FCkPixelArtRenderModule::StartupModule()
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkPixelArtRender"), TEXT("Shaders"), TEXT("CkPixelArt"));
        AddShaderSourceDirectoryMapping(TEXT("/CkPixelArt"), ShaderDir);
    }
}

void FCkPixelArtRenderModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkPixelArtRenderModule, CkPixelArtRender)
