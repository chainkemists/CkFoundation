#include "CkVat_Module.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCkVatModule"

void FCkVatModule::StartupModule()
{
    // Ship the VAT decode shaders (same idiom as FCkUsfModule) — the CkVat look definitions
    // reference "/CkVat/Vat.ush".
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto ShaderDir = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Source"), TEXT("CkVat"), TEXT("Shaders"), TEXT("CkVat"));
        AddShaderSourceDirectoryMapping(TEXT("/CkVat"), ShaderDir);
    }
}

void FCkVatModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkVatModule, CkVat)
