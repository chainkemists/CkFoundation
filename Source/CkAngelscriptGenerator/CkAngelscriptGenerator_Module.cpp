#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptCompileGuard.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"
#include "CkAngelscriptGenerator/Settings/CkAngelscriptGenerator_Settings.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDynamic/CkDynamic_AngelScript.h"

#include "Editor.h"
#include <Misc/CoreDelegates.h>
#include <Misc/CommandLine.h>
#include <Misc/FileHelper.h>
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

    // Detects whether DynamicHandleTypes.json currently has any entries with
    // the dispatcher's stub-marker Description. Used to catch the force-quit
    // case where a stub written this session OR a stub left over from a
    // previously force-quit session needs to be regenerated.
    auto Has_StubMarkersInJson() -> bool
    {
        const auto JsonPath = FCkDynamic_HandleTypeRegistry::GetRegistryFilePath();
        if (JsonPath.IsEmpty())
        { return false; }

        auto Content = FString{};
        if (NOT FFileHelper::LoadFileToString(Content, *JsonPath))
        { return false; }

        // The dispatcher's stub writes "Synthesized stub for emergency
        // recovery (CkAngelscriptGenerator Rev 10)" verbatim into the
        // Description field. Substring check is sufficient — the real
        // generator's Descriptions are sourced from the data asset and
        // won't contain that exact text.
        return Content.Contains(TEXT("Synthesized stub for emergency recovery"));
    }

    // Deferred JSON regen for the DynamicHandle recovery path. Fires from
    // OnPostEngineInit — by which time GEditor is available (unlike at the
    // modal-tick recovery time where the dispatcher writes the initial JSON
    // stub). Replaces stub entries with properly-sourced JSON sourced from
    // the discovered UCkDynamic_HandleDefinition data assets AND refreshes
    // the in-memory FCkAngelScript_HandleRegistry validators (the
    // register-or-update path landed in b9fb7b162).
    //
    // Two triggers:
    //   1. The dispatcher set the session flag — a stub was written this
    //      session and we know to clean it up.
    //   2. The on-disk JSON has stub-marker entries — covers the force-quit
    //      case where a stub from a prior session survived to disk. Without
    //      this trigger, a force-quit between modal-tick recovery and
    //      OnPostEngineInit would leave the user with a permissive validator
    //      indefinitely (until they manually click Force Refresh or edit
    //      an AS file that triggers a recompile and the PreCompile hook).
    auto Maybe_RegenDynamicHandleJson_OnPostInit() -> void
    {
        const auto SessionFlagSet  = ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Did_SynthesizeJsonStub_ThisSession();
        const auto JsonHasStub     = Has_StubMarkersInJson();
        if (NOT SessionFlagSet && NOT JsonHasStub)
        { return; }

        if (JsonHasStub && NOT SessionFlagSet)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] Detected leftover stub-marker entries in DynamicHandleTypes.json ")
                TEXT("(likely from a force-quit session before deferred regen could fire). ")
                TEXT("Running deferred regen now."));
        }

        if (NOT GEditor)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] Deferred DynamicHandle JSON regen wanted, but GEditor is null at ")
                TEXT("OnPostEngineInit — skipping. JSON stays in synthesized-stub state until ")
                TEXT("user clicks 'Generate Handle Type Registry' manually."));
            return;
        }

        auto* Subsystem = GEditor->GetEditorSubsystem<UCkDynamicHandleSubsystem>();
        if (Subsystem == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] Deferred DynamicHandle JSON regen wanted, but ")
                TEXT("UCkDynamicHandleSubsystem isn't loaded — skipping. JSON stays in ")
                TEXT("synthesized-stub state until user clicks 'Generate Handle Type Registry' manually."));
            return;
        }

        ck::angelscriptgenerator::Log(
            TEXT("[Module] Deferred DynamicHandle JSON regen firing (dispatcher synthesized a stub ")
            TEXT("earlier this session). Replacing stub entries with properly-sourced data."));

        Subsystem->GenerateHandleTypeRegistry();

        // ALSO refresh the in-memory registry so the current session uses the
        // strict validator (sourced from the data asset's RequiredFragments)
        // rather than the permissive validator from our JSON stub. With the
        // CkDynamic register-or-update path now in place, calling
        // DiscoverAndRegisterAllDefinitions iterates the AR-discovered
        // UCkDynamic_HandleDefinition data assets (which materialized once AS
        // compile succeeded post-recovery) and updates each in-memory entry
        // via FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle. No
        // restart required.
        FCkDynamic_HandleTypeRegistry::DiscoverAndRegisterAllDefinitions();

        ck::angelscriptgenerator::Log(
            TEXT("[Module] DynamicHandle JSON regenerated and in-memory bindings refreshed ")
            TEXT("from data assets. Current session now uses strict validators — no restart needed."));
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
    _PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda([]()
    {
        Run_AllGenerators();
        Maybe_RegenDynamicHandleJson_OnPostInit();
    });

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
