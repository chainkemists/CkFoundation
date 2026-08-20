#include "CkPixelArtRenderer_Module.h"

#include "Interfaces/IPluginManager.h"
#include <Misc/Paths.h>
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkPixelArtRendererModule"

void FCkPixelArtRendererModule::StartupModule()
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkPixelArtRenderer"), TEXT("Shaders"), TEXT("CkPixelArt"));
        AddShaderSourceDirectoryMapping(TEXT("/CkPixelArt"), ShaderDir);
    }
}

void FCkPixelArtRendererModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkPixelArtRendererModule, CkPixelArtRenderer)
