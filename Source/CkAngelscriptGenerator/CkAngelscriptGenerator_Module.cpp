#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptCompileGuard.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"
#include "CkAngelscriptGenerator/Settings/CkAngelscriptGenerator_Settings.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDynamic/CkDynamic_AngelScript.h"

#include "Editor.h"
#include <Containers/Ticker.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Misc/CoreDelegates.h>
#include <Misc/CommandLine.h>
#include <Misc/FileHelper.h>
#include <Misc/Parse.h>
#include <Misc/Paths.h>
#include <Misc/ScopeExit.h>
#include <ShaderCompiler.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkAngelscriptGeneratorModule"

namespace
{
#if WITH_EDITOR
    // Flips true once FCoreDelegates::OnFEngineLoopInitComplete fires. Gates the
    // PostCompile-driven AssetRegistry cleanup so it can't run during cold-start
    // shader-compile + BP-discovery contention (where GenerateAllAssetRegistries
    // empirically blocks for 6.5min instead of its normal 3.7s). The existing
    // deferred OnFEngineLoopInitComplete + shader-idle poll already covers
    // startup; this flag just prevents the PostCompile path from racing it.
    static bool sg_EngineLoopInitComplete = false;

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

    // Returns true if any generated *Assets.as file on disk contains the
    // AssetRegistry synthesizer's marker comment. Catches the force-quit case
    // where a stub written in a prior session survived to disk without ever
    // being overwritten by a real GenerateAllAssetRegistries pass.
    auto Has_AssetRegistryStubMarkersInFiles() -> bool
    {
        const auto Marker = ck::angelscriptgenerator::self_heal::
            FCkAsAssetRegistryStubSynthesizer::Get_MarkerComment();

        auto Dirs = TArray<FString>{};
        Dirs.Add(FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"));
        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        { Dirs.Add(Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated")); }

        for (const auto& Dir : Dirs)
        {
            if (NOT IFileManager::Get().DirectoryExists(*Dir))
            { continue; }

            auto Files = TArray<FString>{};
            IFileManager::Get().FindFilesRecursive(Files, *Dir, TEXT("*Assets.as"),
                /*Files=*/true, /*Directories=*/false);

            for (const auto& File : Files)
            {
                auto Contents = FString{};
                if (FFileHelper::LoadFileToString(Contents, *File)
                    && Contents.Contains(Marker))
                { return true; }
            }
        }
        return false;
    }

    // Deferred AssetRegistry regen, parallels Maybe_RegenDynamicHandleJson_OnPostInit.
    // Two triggers:
    //   1. The dispatcher set the session flag — a stub was written this session.
    //   2. The on-disk marker scan — covers force-quit between modal-tick recovery
    //      and OnPostEngineInit. Without this, a force-quit session leaves the stub
    //      in a *Assets.as file indefinitely; next clean editor launch would still
    //      see the stub (harmless but cosmetically ugly + counts toward the wrong-
    //      file-targeting edge case where it accumulates over time).
    auto Maybe_RegenAssetRegistry_OnPostInit() -> void
    {
        const auto SessionFlagSet = ck::angelscriptgenerator::self_heal::
            FCkAsRecoveryDispatcher::Did_SynthesizeAssetRegistryStub_ThisSession();
        const auto FileHasMarker = Has_AssetRegistryStubMarkersInFiles();
        if (NOT SessionFlagSet && NOT FileHasMarker)
        { return; }

        if (FileHasMarker && NOT SessionFlagSet)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] Detected leftover AssetRegistry stub marker(s) in generated *Assets.as ")
                TEXT("file(s) (likely from a force-quit session before deferred regen could fire). ")
                TEXT("Running GenerateAllAssetRegistries now."));
        }

        if (NOT GEditor)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] Deferred AssetRegistry regen wanted, but GEditor is null at ")
                TEXT("OnPostEngineInit — skipping. Stubs stay in place until user clicks ")
                TEXT("'Generate All Asset Registries' manually."));
            return;
        }

        auto* Subsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
        if (Subsystem == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] Deferred AssetRegistry regen wanted, but UCkAssetRegistrySubsystem ")
                TEXT("isn't loaded — skipping. Stubs stay in place until user clicks ")
                TEXT("'Generate All Asset Registries' manually."));
            return;
        }

        ck::angelscriptgenerator::Log(
            TEXT("[Module] Deferred AssetRegistry regen scheduled (dispatcher synthesized a stub ")
            TEXT("earlier this session, or a stub marker is present on disk). Will hook ")
            TEXT("FCoreDelegates::OnFEngineLoopInitComplete and then poll for shader-compile idle ")
            TEXT("before firing GenerateAllAssetRegistries — this rewrites all *Assets.as files ")
            TEXT("from the AR-scanned state, restoring correct file placement and overwriting ")
            TEXT("Tier 3 UObject fallbacks with the proper resolved classes."));

        // Hook the regen to OnFEngineLoopInitComplete, which broadcasts AFTER
        // FEngineLoop::Init fully returns (slow-task stack unwound — fixes the
        // out-of-order FSlowTask destruction crash empirically caught 2026-05-12
        // when we wired regen to OnPostEngineInit inline). OnPostEngineInit fires
        // BEFORE init's slow task pops; OnFEngineLoopInitComplete fires AFTER.
        //
        // Then within that callback, poll via FTSTicker until the shader
        // compiler is idle and async-load queue is drained, so we don't pile
        // our 400+ texture async loads on top of cold-start shader compiles
        // and BP discovery (empirically caused a multi-minute slow-task modal
        // block, 2026-05-12). Hard cap at 60 seconds — if conditions never
        // satisfy, fire anyway with a warning.
        FCoreDelegates::OnFEngineLoopInitComplete.AddLambda(
            []()
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[Module] OnFEngineLoopInitComplete fired. ")
                    TEXT("Starting idle-wait poll before AssetRegistry regen."));

                // Capture by-value into the ticker — the lambda needs its own
                // mutable wait-counter and shared subsystem ptr resolution at
                // each tick (UCkAssetRegistrySubsystem is GEditor-owned and
                // may not survive PIE shutdown, though that's unlikely between
                // boot and our first poll).
                auto WaitTicks = MakeShared<int32>(0);
                constexpr int32 MaxWaitTicks = 30; // ~60 seconds at 0.5Hz polling.

                FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateLambda(
                        [WaitTicks](float /*InDeltaTime*/) -> bool
                        {
                            ++(*WaitTicks);

                            const auto ShadersIdle = (GShaderCompilingManager == nullptr)
                                || (GShaderCompilingManager->GetNumRemainingJobs() == 0);
                            const auto Hit_HardCap = (*WaitTicks) >= MaxWaitTicks;

                            if (NOT ShadersIdle && NOT Hit_HardCap)
                            {
                                // Stay subscribed for next poll tick.
                                return true;
                            }

                            if (Hit_HardCap && NOT ShadersIdle)
                            {
                                ck::angelscriptgenerator::Warning(
                                    TEXT("[Module] Idle-wait hit hard cap at {} polls "
                                         "while shader compiler still busy ({} jobs remaining). "
                                         "Firing AssetRegistry regen anyway — the slow-task "
                                         "modal may stick for a while."),
                                    *WaitTicks,
                                    GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0);
                            }
                            else
                            {
                                ck::angelscriptgenerator::Log(
                                    TEXT("[Module] Editor settled (shader compiler idle) after "
                                         "{} polls. Firing AssetRegistry regen now."),
                                    *WaitTicks);
                            }

                            if (NOT GEditor)
                            {
                                ck::angelscriptgenerator::Warning(
                                    TEXT("[Module] GEditor went null while waiting to regen — abandoning."));
                                return false;
                            }
                            auto* Subsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
                            if (Subsystem == nullptr)
                            {
                                ck::angelscriptgenerator::Warning(
                                    TEXT("[Module] UCkAssetRegistrySubsystem unavailable at regen time — abandoning."));
                                return false;
                            }

                            Subsystem->GenerateAllAssetRegistries();
                            return false; // one-shot
                        }),
                    /*InDelay=*/2.0f); // polls every ~2 seconds
            });
    }

    // Re-entrancy guard so back-to-back PostCompiles (or hot-reload bursts)
    // don't queue overlapping regens. Set when a ticker is in flight; cleared
    // by the ticker callback once the regen completes (or is abandoned).
    static bool sg_PostCompileRegenInFlight = false;

    // PostCompile-driven AssetRegistry cleanup. Gated narrowly so it doesn't
    // tax every successful AS recompile with a full GenerateAllAssetRegistries
    // pass (3.7s optimal, 6.5min under cold-start contention).
    //
    // Fires only when ALL of the following hold:
    //   1. OnFEngineLoopInitComplete has fired — prevents racing the existing
    //      deferred-startup regen during cold-start shader/BP contention.
    //   2. An *Assets.as file on disk contains the synthesizer's marker comment —
    //      i.e. the dispatcher synthesized a stub mid-session (or a prior-session
    //      stub survived past the deferred startup pass for whatever reason).
    //   3. No regen ticker is already queued from a prior PostCompile.
    //
    // Condition (2) is also our self-termination: once GenerateAllAssetRegistries
    // rewrites the files from AR state, the marker is gone and subsequent
    // PostCompile invocations are no-ops (cost: one cheap fs walk).
    //
    // Why FTSTicker instead of a synchronous call: PostCompile broadcasts from
    // inside FAngelscriptManager::CompileModules, which is inside an FSlowTask
    // scope ("Script Module Compilation"). GenerateAllAssetRegistries opens its
    // own FSlowTask ("Generating Asset Registry: <File>.as"). Running it
    // synchronously means the AR slow task is still on the stack when
    // CompileModules's slow task destructs, tripping the "Task == this"
    // out-of-order ensure in SlowTask.cpp:149. Empirically caught 2026-05-13.
    // Deferring via FTSTicker lets CompileModules return and pop its slow task
    // before the regen begins.
    //
    // What this fixes that the existing startup-only path doesn't:
    //   Mid-session, a user adds a new asset + a new AS file referencing it.
    //   AS fails to compile (accessor missing). Dispatcher synthesizes a stub.
    //   AS recompile succeeds with the stub. Without this hook, the stub sits
    //   in *Assets.as until next editor launch (cosmetic accumulation, plus
    //   the "wrong file" + Tier 3 UObject fallback edge cases). With this hook,
    //   the very next PostCompile rewrites the file fresh.
    auto Maybe_RegenAssetRegistry_OnPostCompile() -> void
    {
        if (NOT sg_EngineLoopInitComplete)
        { return; }

        if (sg_PostCompileRegenInFlight)
        { return; }

        if (NOT Has_AssetRegistryStubMarkersInFiles())
        { return; }

        sg_PostCompileRegenInFlight = true;

        ck::angelscriptgenerator::Log(
            TEXT("[Module] PostCompile detected pending AssetRegistry stub marker(s) on disk. ")
            TEXT("Queueing deferred GenerateAllAssetRegistries (via FTSTicker, 1s delay) to escape ")
            TEXT("CompileModules's slow-task scope before AR opens its own."));

        FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda(
                [](float /*InDeltaTime*/) -> bool
                {
                    ON_SCOPE_EXIT { sg_PostCompileRegenInFlight = false; };

                    if (NOT GEditor)
                    { return false; }

                    auto* Subsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
                    if (Subsystem == nullptr)
                    { return false; }

                    ck::angelscriptgenerator::Log(
                        TEXT("[Module] Deferred PostCompile AssetRegistry regen firing — ")
                        TEXT("rewriting *Assets.as files from AR state."));

                    Subsystem->GenerateAllAssetRegistries();
                    return false; // one-shot
                }),
            /*InDelay=*/1.0f);
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
        Maybe_RegenAssetRegistry_OnPostInit();
    });

    // Track engine-loop-init completion so the PostCompile AssetRegistry
    // cleanup knows it's safe to fire (no longer in cold-start contention).
    _EngineLoopInitCompleteHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddLambda(
        []() { sg_EngineLoopInitComplete = true; });

#if WITH_ANGELSCRIPT_CK
    _PostAngelscriptCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda(
        []()
        {
            Run_AllGenerators();
            Maybe_RegenAssetRegistry_OnPostCompile();
        });

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

    if (_EngineLoopInitCompleteHandle.IsValid())
    {
        FCoreDelegates::OnFEngineLoopInitComplete.Remove(_EngineLoopInitCompleteHandle);
        _EngineLoopInitCompleteHandle.Reset();
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
