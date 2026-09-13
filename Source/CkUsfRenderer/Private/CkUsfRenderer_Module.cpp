#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FCkUsfRendererModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
        // Engine-only early module: loading this module without its owning plugin is an installation error.
        if (Plugin.IsValid() == false)
        {
            UE_LOG(LogTemp, Fatal, TEXT("CkUsfRenderer requires its CkFoundation plugin"));
            return;
        }
        AddShaderSourceDirectoryMapping(TEXT("/CkUsfRenderer"), FPaths::Combine(
            Plugin->GetBaseDir(), TEXT("Source/CkUsfRenderer/Shaders")));
    }
};

IMPLEMENT_MODULE(FCkUsfRendererModule, CkUsfRenderer)
