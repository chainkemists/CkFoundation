#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
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
#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Containers/Ticker.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Logging/MessageLog.h>
#include <MessageLogModule.h>
#include <Misc/CoreDelegates.h>
#include <Misc/CommandLine.h>
#include <Misc/FileHelper.h>
#include <Misc/Parse.h>
#include <Misc/Paths.h>
#include <ShaderCompiler.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkAngelscriptGeneratorModule"

namespace
{
#if WITH_EDITOR
    // Shared with the dispatcher's UI-surfacing helpers. Keep in sync.
    constexpr auto* sSelfHealLogChannel = TEXT("CkAngelscriptGenerator");
    // Flips true once FCoreDelegates::OnFEngineLoopInitComplete fires. Gates the
    // PostCompile-driven AssetRegistry cleanup so it can't run during cold-start
    // shader-compile + BP-discovery contention (where GenerateAllAssetRegistries
    // empirically blocks for 6.5min instead of its normal 3.7s). The existing
    // deferred OnFEngineLoopInitComplete + shader-idle poll already covers
    // startup; this flag just prevents the PostCompile path from racing it.
    static bool sg_EngineLoopInitComplete = false;

    // Shared in-flight guard so the cold-start (OnFEngineLoopInitComplete) path
    // and the PostCompile path don't double-fire GenerateAllAssetRegistries
    // within the same cold-start window. Either path bails on its precheck if
    // a regen is already queued/firing; the flag is cleared by a marker-
    // disappearance poll once the regen has actually rewritten the *Assets.as
    // files from AR state (or by a hard-cap timeout as a safety net).
    //
    // The async-completion delegate on UCkAssetRegistrySubsystem
    // (OnAssetRegistryComplete) is a UFUNCTION-shaped DYNAMIC multicast — wiring
    // it from a module-local handler would require a UCLASS sink plus a
    // .generated.h, which is heavyweight for one callback. The marker-poll
    // mirrors what the rest of this file already does (file-scan as the source
    // of truth for stub state) and avoids introducing a sink UObject.
    static bool sg_AnyAssetRegistryRegenInFlight = false;

    auto Run_AllGenerators() -> void
    {
        FCkAngelscriptEntityScriptParamsGenerator::GenerateAll();
        FCkAutoTestWrapperGenerator::GenerateAll();
    }

    // ---- Self-heal stub-recovery file cleanup --------------------------------------
    //
    // The self-heal dispatcher writes synthesized stubs to SIBLING files
    // (`Script/Generated/_StubRecovery_*.{as,json}`) rather than mutating the
    // canonical generated files. Canonical files therefore stay byte-clean
    // from HEAD and accidental staging is impossible (the `_StubRecovery_*`
    // paths are gitignored).
    //
    // Cleanup runs from the PostCompile hook — i.e. only after a successful
    // AS compile. If next launch still has the same drift, the dispatcher
    // re-creates the stubs fresh. We walk the project + every enabled
    // plugin's `Script/Generated/` directory and delete any matching files.

    auto Delete_AllStubRecoveryFiles() -> int32
    {
        auto Dirs = TArray<FString>{};
        Dirs.Add(FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"));
        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        { Dirs.Add(Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated")); }

        auto DeletedCount = 0;
        const auto Patterns = TArray<const TCHAR*>{
            TEXT("_StubRecovery_*.as"),
            TEXT("_StubRecovery_*.json"),
        };

        for (const auto& Dir : Dirs)
        {
            if (NOT IFileManager::Get().DirectoryExists(*Dir))
            { continue; }

            for (const auto* Pattern : Patterns)
            {
                auto Files = TArray<FString>{};
                IFileManager::Get().FindFilesRecursive(Files, *Dir, Pattern,
                    /*Files=*/true, /*Directories=*/false);

                for (const auto& File : Files)
                {
                    if (IFileManager::Get().Delete(*File, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true))
                    {
                        ck::angelscriptgenerator::Log(
                            TEXT("[Module] Self-heal stub file served its purpose — deleting: {}"), File);
                        ++DeletedCount;
                    }
                    else
                    {
                        ck::angelscriptgenerator::Warning(
                            TEXT("[Module] Failed to delete self-heal stub file: {}"), File);
                    }
                }
            }
        }

        if (DeletedCount > 0)
        {
            FMessageLog{FName{sSelfHealLogChannel}}.Info(FText::Format(
                LOCTEXT("CleanupEntry",
                    "PostCompile cleanup: deleted {0} self-heal stub file(s)."),
                FText::AsNumber(DeletedCount)));
        }

        return DeletedCount;
    }


    // Detects whether the DynamicHandle stub sibling file exists on disk —
    // `Script/Generated/_StubRecovery_DynamicHandleTypes.json`. The canonical
    // file is never touched by the self-heal dispatcher; presence of the
    // sibling is the unambiguous signal that a stub was synthesized and the
    // canonical may be missing entries.
    auto Has_DynamicHandleStubRecoveryFile_OnDisk() -> bool
    {
        const auto JsonPath = FCkDynamic_HandleTypeRegistry::GetRegistryFilePath();
        if (JsonPath.IsEmpty())
        { return false; }

        const auto StubPath = FPaths::GetPath(JsonPath) /
            (FString{TEXT("_StubRecovery_")} + FPaths::GetCleanFilename(JsonPath));
        return IFileManager::Get().FileExists(*StubPath);
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
        const auto SessionFlagSet = ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Did_SynthesizeJsonStub_ThisSession();
        const auto StubOnDisk     = Has_DynamicHandleStubRecoveryFile_OnDisk();
        if (NOT SessionFlagSet && NOT StubOnDisk)
        { return; }

        if (StubOnDisk && NOT SessionFlagSet)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] Detected leftover _StubRecovery_DynamicHandleTypes.json sibling on disk ")
                TEXT("(likely from a force-quit session before PostCompile cleanup could fire). ")
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

    // Returns true if any `_StubRecovery_*Assets.as` sibling exists across
    // the project + enabled-plugin `Script/Generated/` directories. The
    // canonical *Assets.as files are never touched by the self-heal
    // dispatcher; sibling-file presence is the unambiguous signal that
    // recovery wrote a stub.
    auto Has_AssetRegistryStubRecoveryFiles_OnDisk() -> bool
    {
        auto Dirs = TArray<FString>{};
        Dirs.Add(FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"));
        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        { Dirs.Add(Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated")); }

        for (const auto& Dir : Dirs)
        {
            if (NOT IFileManager::Get().DirectoryExists(*Dir))
            { continue; }

            auto Files = TArray<FString>{};
            IFileManager::Get().FindFilesRecursive(Files, *Dir, TEXT("_StubRecovery_*Assets.as"),
                /*Files=*/true, /*Directories=*/false);
            if (Files.Num() > 0)
            { return true; }
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
        const auto FileHasMarker = Has_AssetRegistryStubRecoveryFiles_OnDisk();
        if (NOT SessionFlagSet && NOT FileHasMarker)
        { return; }

        if (sg_AnyAssetRegistryRegenInFlight)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] Cold-start AssetRegistry regen wanted, but a regen is already in flight — bailing."));
            return;
        }

        if (FileHasMarker && NOT SessionFlagSet)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] Detected leftover _StubRecovery_*Assets.as sibling file(s) on disk ")
                TEXT("(likely from a force-quit session before PostCompile cleanup could fire). ")
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

        // Reserve the regen slot now so any PostCompile firing during the 2-second
        // idle-wait window (after OnFEngineLoopInitComplete sets sg_EngineLoopInitComplete
        // but before our ticker fires) will see the in-flight flag and bail.
        sg_AnyAssetRegistryRegenInFlight = true;

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
                            // Also gate on AssetRegistry cataloging being complete. If AR is
                            // still scanning its discovery roots when GenerateAllAssetRegistries
                            // fires, AR's GetAssetsByPath returns a partial list — the regen
                            // then emits an incomplete *Assets.as file with accessors silently
                            // missing (causes a recovery loop on next compile, or worst-case a
                            // permanently dropped accessor if AR cataloging consistently
                            // out-races the hard cap).
                            auto& ArModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
                            const auto ArIdle = NOT ArModule.Get().IsLoadingAssets();
                            const auto Settled = ShadersIdle && ArIdle;
                            const auto Hit_HardCap = (*WaitTicks) >= MaxWaitTicks;

                            if (NOT Settled && NOT Hit_HardCap)
                            {
                                // Stay subscribed for next poll tick.
                                return true;
                            }

                            if (Hit_HardCap && NOT Settled)
                            {
                                ck::angelscriptgenerator::Warning(
                                    TEXT("[Module] Idle-wait hit hard cap at {} polls — "
                                         "shaders idle [{}] (jobs remaining [{}]), AR idle [{}]. "
                                         "Firing AssetRegistry regen anyway — partial *Assets.as "
                                         "output is possible if AR is still cataloging."),
                                    *WaitTicks,
                                    ShadersIdle,
                                    GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0,
                                    ArIdle);
                            }
                            else
                            {
                                ck::angelscriptgenerator::Log(
                                    TEXT("[Module] Editor settled (shader compiler idle AND AR idle) after "
                                         "{} polls. Firing AssetRegistry regen now."),
                                    *WaitTicks);
                            }

                            if (NOT GEditor)
                            {
                                ck::angelscriptgenerator::Warning(
                                    TEXT("[Module] GEditor went null while waiting to regen — abandoning."));
                                sg_AnyAssetRegistryRegenInFlight = false;
                                return false;
                            }
                            auto* Subsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
                            if (Subsystem == nullptr)
                            {
                                ck::angelscriptgenerator::Warning(
                                    TEXT("[Module] UCkAssetRegistrySubsystem unavailable at regen time — abandoning."));
                                sg_AnyAssetRegistryRegenInFlight = false;
                                return false;
                            }

                            Subsystem->GenerateAllAssetRegistries();
                            // Sibling-file model: stub files are independent of the canonical
                            // *Assets.as files. The regen call's synchronous return is enough
                            // signal that next-launch will be clean — no need to poll for any
                            // marker disappearance. Clear the in-flight guard immediately so
                            // back-to-back PostCompiles can re-attempt if needed.
                            sg_AnyAssetRegistryRegenInFlight = false;
                            return false; // one-shot
                        }),
                    /*InDelay=*/2.0f); // polls every ~2 seconds
            });
    }

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

        if (sg_AnyAssetRegistryRegenInFlight)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] PostCompile fired but AssetRegistry regen already in flight — bailing."));
            return;
        }

        if (NOT Has_AssetRegistryStubRecoveryFiles_OnDisk())
        { return; }

        ck::angelscriptgenerator::Log(
            TEXT("[Module] PostCompile detected pending _StubRecovery_*Assets.as sibling file(s) on disk. ")
            TEXT("Queueing deferred GenerateAllAssetRegistries (via FTSTicker, ~1s polling) to escape ")
            TEXT("CompileModules's slow-task scope before AR opens its own, and to gate on shader-")
            TEXT("compiler idle + AssetRegistry cataloging completion before firing."));

        // Reserve the regen slot now so back-to-back PostCompiles don't queue
        // overlapping tickers, and so a cold-start path firing concurrently
        // bails on its precheck.
        sg_AnyAssetRegistryRegenInFlight = true;

        // Mirror the cold-start path's gating: poll until shaders are idle AND AR is done
        // cataloging. Without the AR-idle gate, GenerateAllAssetRegistries can fire while AR
        // is still scanning its discovery roots, producing partial *Assets.as output with
        // accessors silently missing.
        auto WaitTicks = MakeShared<int32>(0);
        constexpr int32 MaxWaitTicks = 30; // ~60s at ~2s polling — match cold-start path.

        FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda(
                [WaitTicks](float /*InDeltaTime*/) -> bool
                {
                    ++(*WaitTicks);

                    const auto ShadersIdle = (GShaderCompilingManager == nullptr)
                        || (GShaderCompilingManager->GetNumRemainingJobs() == 0);
                    auto& ArModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
                    const auto ArIdle = NOT ArModule.Get().IsLoadingAssets();
                    const auto Settled = ShadersIdle && ArIdle;
                    const auto Hit_HardCap = (*WaitTicks) >= MaxWaitTicks;

                    if (NOT Settled && NOT Hit_HardCap)
                    { return true; }

                    if (Hit_HardCap && NOT Settled)
                    {
                        ck::angelscriptgenerator::Warning(
                            TEXT("[Module] PostCompile idle-wait hit hard cap at {} polls — "
                                 "shaders idle [{}] (jobs remaining [{}]), AR idle [{}]. "
                                 "Firing AssetRegistry regen anyway — partial *Assets.as "
                                 "output is possible if AR is still cataloging."),
                            *WaitTicks,
                            ShadersIdle,
                            GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0,
                            ArIdle);
                    }
                    else
                    {
                        ck::angelscriptgenerator::Log(
                            TEXT("[Module] PostCompile settled (shader compiler idle AND AR idle) after "
                                 "{} polls. Firing AssetRegistry regen now — rewriting *Assets.as "
                                 "files from AR state."),
                            *WaitTicks);
                    }

                    if (NOT GEditor)
                    {
                        sg_AnyAssetRegistryRegenInFlight = false;
                        return false;
                    }

                    auto* Subsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
                    if (Subsystem == nullptr)
                    {
                        sg_AnyAssetRegistryRegenInFlight = false;
                        return false;
                    }

                    Subsystem->GenerateAllAssetRegistries();
                    sg_AnyAssetRegistryRegenInFlight = false;
                    return false; // one-shot
                }),
            /*InDelay=*/2.0f);
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
    {
        auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
        auto InitOptions = FMessageLogInitializationOptions{};
        InitOptions.bShowFilters = true;
        InitOptions.bShowPages   = false;
        InitOptions.bAllowClear  = true;
        MessageLogModule.RegisterLogListing(
            FName{sSelfHealLogChannel},
            LOCTEXT("MessageLogTitle", "AngelScript Generator"),
            InitOptions);
    }

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

            // PostCompile fires only on a successful AS compile — i.e. the
            // sibling stub files (if any) served their purpose. Delete them
            // so the working tree returns to clean canonical state. If next
            // launch still has drift, the dispatcher re-creates them fresh.
            const auto DeletedCount = Delete_AllStubRecoveryFiles();
            if (DeletedCount > 0)
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[Module] PostCompile self-heal cleanup: deleted {} stub recovery file(s)."),
                    DeletedCount);
            }
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

    if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
    {
        auto& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
        MessageLogModule.UnregisterLogListing(FName{sSelfHealLogChannel});
    }
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAngelscriptGeneratorModule, CkAngelscriptGenerator)

// --------------------------------------------------------------------------------------------------------------------
