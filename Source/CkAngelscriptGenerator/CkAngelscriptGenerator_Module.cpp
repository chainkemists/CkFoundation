#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptCompileGuard.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"
#include "CkAngelscriptGenerator/Settings/CkAngelscriptGenerator_Settings.h"

#include "CkCore/Macros/CkMacros.h"

#include <Misc/CoreDelegates.h>
#include <Misc/CommandLine.h>
#include <Misc/Parse.h>

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

#if WITH_ANGELSCRIPT_CK
    // Two opt-outs gate the Rev 10 self-heal dispatcher:
    //   * `-NoCkAsRegen` CLI flag — per-launch escape for the case where the
    //     user wants the editor to show raw Hazelight errors against the
    //     committed accessor files without our recovery intervening (e.g. when
    //     debugging a genuine authoring bug whose error happens to match one
    //     of our recovery patterns).
    //   * `_EnableAsBootstrapSelfHeal` project setting — same semantics,
    //     project-wide and persistent across launches.
    auto Is_SelfHealEnabled() -> bool
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("NoCkAsRegen")))
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] -NoCkAsRegen specified — AS bootstrap self-heal disabled."));
            return false;
        }

        if (const auto* Settings = GetDefault<UCk_AngelscriptGenerator_ProjectSettings_UE>();
            Settings != nullptr && NOT Settings->Get_EnableAsBootstrapSelfHeal())
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] CkSettings disabled AS bootstrap self-heal — skipping OnReloadHadErrors hook."));
            return false;
        }

        return true;
    }
#endif
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

    if (Is_SelfHealEnabled())
    {
        ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Reset_CyclesRun();

        _ReloadHadErrorsHandle = FAngelscriptCodeModule::GetReloadHadErrors().AddStatic(
            &ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::OnAngelscriptReloadHadErrors);
        _SelfHealArmed = true;

        ck::angelscriptgenerator::Log(
            TEXT("[Module] AS bootstrap self-heal armed (Rev 10, cycle cap {})."),
            ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::MaxCycles);
    }
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
    if (FModuleManager::Get().IsModuleLoaded("AngelscriptCode"))
    {
        if (_PostAngelscriptCompileHandle.IsValid())
        {
            FAngelscriptCodeModule::GetPostCompile().Remove(_PostAngelscriptCompileHandle);
            _PostAngelscriptCompileHandle.Reset();
        }

        if (_ReloadHadErrorsHandle.IsValid())
        {
            FAngelscriptCodeModule::GetReloadHadErrors().Remove(_ReloadHadErrorsHandle);
            _ReloadHadErrorsHandle.Reset();
        }
    }
    _SelfHealArmed = false;

    ck::angelscriptgenerator::FCk_AngelscriptCompileGuard::Uninstall();
#endif
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAngelscriptGeneratorModule, CkAngelscriptGenerator)

// --------------------------------------------------------------------------------------------------------------------
