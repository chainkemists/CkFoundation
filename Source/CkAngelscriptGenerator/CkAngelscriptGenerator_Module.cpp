#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptCompileGuard.h"

#include <Misc/CoreDelegates.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkAngelscriptGeneratorModule"

namespace
{
#if WITH_EDITOR
    auto Run_AllGenerators() -> void
    {
        FCkAngelscriptEntityScriptParamsGenerator::GenerateAll();
        FCkAutoTestWrapperGenerator::GenerateAll();
    }
#endif
}

void FCkAngelscriptGeneratorModule::StartupModule()
{
#if WITH_EDITOR
    _PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddStatic(&Run_AllGenerators);

#if WITH_ANGELSCRIPT_CK
    _PostAngelscriptCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda(
        []() { Run_AllGenerators(); });

    ck::angelscriptgenerator::FCk_AngelscriptCompileGuard::Install();
#endif
#endif // WITH_EDITOR
}

void FCkAngelscriptGeneratorModule::ShutdownModule()
{
#if WITH_EDITOR
    if (_PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(_PostEngineInitHandle);
        _PostEngineInitHandle.Reset();
    }

#if WITH_ANGELSCRIPT_CK
    if (_PostAngelscriptCompileHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("AngelscriptCode"))
    {
        FAngelscriptCodeModule::GetPostCompile().Remove(_PostAngelscriptCompileHandle);
        _PostAngelscriptCompileHandle.Reset();
    }

    ck::angelscriptgenerator::FCk_AngelscriptCompileGuard::Uninstall();
#endif
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAngelscriptGeneratorModule, CkAngelscriptGenerator)

// --------------------------------------------------------------------------------------------------------------------
