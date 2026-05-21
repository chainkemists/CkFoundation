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
    constexpr auto* sSelfHealLogChannel = TEXT("CkAngelscriptGenerator");

    // Gates the PostCompile-driven AssetRegistry cleanup so it can't run during
    // cold-start contention (where GenerateAllAssetRegistries empirically
    // blocks for 6.5min vs its normal 3.7s).
    static bool sg_EngineLoopInitComplete = false;

    // Shared in-flight guard so the cold-start and PostCompile regen paths
    // don't double-fire GenerateAllAssetRegistries within the same window.
    //
    // We don't subscribe to UCkAssetRegistrySubsystem::OnAssetRegistryComplete
    // for clearing because it's a UFUNCTION-shaped DYNAMIC multicast — wiring
    // a module-local handler would require a UCLASS sink + .generated.h for
    // one callback. The file-scan-based marker check the rest of this file
    // uses is the cheaper source of truth.
    //
    // The flag's `false` write also marks the boundary at which the AR
    // sibling-file cleanup is safe to run — that cleanup is owned by the
    // ticker callback inside the regen functions, not by the outer
    // PostCompile lambda. See "Force-quit between recovery and PostCompile"
    // in CkAngelscriptGenerator/CLAUDE.md and the bug pinned by the
    // _probe_assetregistry_loop probe.
    static bool sg_AnyAssetRegistryRegenInFlight = false;

    // ----------------------------------------------------------------------------------------------------------------
    // Pattern constants: one per stub-synthesizer's output. Each generator
    // that emits a sibling owns the deletion of its own pattern AFTER it
    // successfully rewrites the canonical. Broad-sweep cleanup is reserved
    // for StartupModule's force-quit-survivor walk only.
    // ----------------------------------------------------------------------------------------------------------------

    constexpr auto* sStubPattern_EntitySpawnParams = TEXT("_StubRecovery_*_EntitySpawnParams.as");
    constexpr auto* sStubPattern_AssetRegistry     = TEXT("_StubRecovery_*Assets.as");
    constexpr auto* sStubPattern_DynamicHandle     = TEXT("_StubRecovery_*.json");

    auto Run_AllGenerators() -> void
    {
        FCkAngelscriptEntityScriptParamsGenerator::GenerateAll();
        FCkAutoTestWrapperGenerator::GenerateAll();
    }

    // Walks project + plugin Script/Generated/ dirs and deletes
    // `_StubRecovery_*` siblings whose filename matches ANY of `Patterns`.
    // Returns the count deleted.
    //
    // Per-pattern scoping is load-bearing: the AssetRegistry regen path is
    // async (FTSTicker with idle-wait), so its sibling MUST NOT be deleted
    // until the ticker has actually rewritten the canonical. Calling this
    // from the outer PostCompile lambda with the AR pattern reintroduces
    // the loop pinned by _probe_assetregistry_loop. The synchronous
    // EntitySpawnParams + DynamicHandle generators are safe to clean from
    // PostCompile immediately because they complete before PostCompile
    // returns.
    auto Delete_StubRecoveryFiles_ForPatterns(
        TArrayView<const TCHAR* const> Patterns) -> int32
    {
        auto Dirs = TArray<FString>{};
        Dirs.Add(FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"));
        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        { Dirs.Add(Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated")); }

        auto DeletedCount = 0;

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
            // Context-agnostic message — caller (StartupModule, PostCompile,
            // or the AssetRegistry ticker callback) logs its own
            // disambiguating line.
            FMessageLog{FName{sSelfHealLogChannel}}.Info(FText::Format(
                LOCTEXT("CleanupEntry",
                    "Self-heal cleanup: deleted {0} stub recovery file(s)."),
                FText::AsNumber(DeletedCount)));
        }

        return DeletedCount;
    }

    // Broad-sweep convenience for StartupModule's force-quit-survivor walk.
    // Do NOT call this from PostCompile — the AssetRegistry path is async
    // and its sibling must survive until the deferred regen actually runs.
    auto Delete_AllStubRecoveryFiles() -> int32
    {
        const auto AllPatterns = TArray<const TCHAR*>{
            sStubPattern_EntitySpawnParams,
            sStubPattern_AssetRegistry,
            sStubPattern_DynamicHandle,
        };
        return Delete_StubRecoveryFiles_ForPatterns(AllPatterns);
    }

    auto Has_DynamicHandleStubRecoveryFile_OnDisk() -> bool
    {
        const auto JsonPath = FCkDynamic_HandleTypeRegistry::GetRegistryFilePath();
        if (JsonPath.IsEmpty())
        { return false; }

        const auto StubPath = FPaths::GetPath(JsonPath) /
            (FString{TEXT("_StubRecovery_")} + FPaths::GetCleanFilename(JsonPath));
        return IFileManager::Get().FileExists(*StubPath);
    }

    // Deferred DynamicHandle JSON regen for the cold-start path. Fires from
    // OnPostEngineInit (GEditor available, unlike at modal-tick where the
    // dispatcher writes the stub). Triggers:
    //   1. Session flag — a stub was synthesized this session.
    //   2. On-disk sibling — covers force-quit between modal-tick recovery
    //      and OnPostEngineInit cleanup.
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
                TEXT("[Module] Deferred DynamicHandle JSON regen wanted, but GEditor is null — skipping. ")
                TEXT("JSON stays in synthesized-stub state until user clicks 'Generate Handle Type Registry' manually."));
            return;
        }

        auto* Subsystem = GEditor->GetEditorSubsystem<UCkDynamicHandleSubsystem>();
        if (Subsystem == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] Deferred DynamicHandle JSON regen wanted, but UCkDynamicHandleSubsystem ")
                TEXT("isn't loaded — skipping."));
            return;
        }

        ck::angelscriptgenerator::Log(
            TEXT("[Module] Deferred DynamicHandle JSON regen firing. Replacing stub entries with properly-sourced data."));

        Subsystem->GenerateHandleTypeRegistry();

        // Refresh the in-memory registry so this session uses the strict
        // validator from each data asset's RequiredFragments instead of the
        // permissive one our JSON stub registered. DiscoverAndRegisterAll-
        // Definitions routes through the register-or-update path; no restart
        // needed.
        FCkDynamic_HandleTypeRegistry::DiscoverAndRegisterAllDefinitions();

        ck::angelscriptgenerator::Log(
            TEXT("[Module] DynamicHandle JSON regenerated and in-memory bindings refreshed — strict validators active."));
    }

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

    // Deferred AssetRegistry regen for the cold-start path. Same trigger shape
    // as the DynamicHandle counterpart above (session flag OR sibling-on-disk).
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
                TEXT("[Module] Deferred AssetRegistry regen wanted, but GEditor is null — skipping."));
            return;
        }

        auto* Subsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
        if (Subsystem == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] Deferred AssetRegistry regen wanted, but UCkAssetRegistrySubsystem isn't loaded — skipping."));
            return;
        }

        ck::angelscriptgenerator::Log(
            TEXT("[Module] Deferred AssetRegistry regen scheduled. Waiting for shader-compile + AR-cataloging idle ")
            TEXT("before firing GenerateAllAssetRegistries."));

        // Reserve the slot now so PostCompile firing during the idle-wait
        // window bails on its precheck.
        sg_AnyAssetRegistryRegenInFlight = true;

        // Hook the regen to OnFEngineLoopInitComplete (broadcasts AFTER
        // FEngineLoop::Init returns and pops its slow task). OnPostEngineInit
        // fires INSIDE init's slow task — wiring regen there triggers an
        // out-of-order FSlowTask destruction ensure when AR opens its own
        // slow task. Empirically caught 2026-05-12.
        //
        // Inside the callback, FTSTicker polls until shader compiler is idle
        // AND AR is done cataloging. Without the AR-idle gate, GenerateAll-
        // AssetRegistries can fire while AR is still scanning, producing
        // partial *Assets.as output with accessors silently missing. Without
        // the shader gate, our 400+ texture async loads pile on top of cold-
        // start contention (the 6.5min wedge, empirically caught 2026-05-12).
        // Hard cap at 60s — fire anyway with a warning rather than hang.
        FCoreDelegates::OnFEngineLoopInitComplete.AddLambda(
            []()
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[Module] OnFEngineLoopInitComplete fired. Starting idle-wait poll before AssetRegistry regen."));

                auto WaitTicks = MakeShared<int32>(0);
                constexpr int32 MaxWaitTicks = 30; // ~60s at 0.5Hz polling.

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
                                    TEXT("[Module] Idle-wait hit hard cap at {} polls — shaders idle [{}] "
                                         "(jobs remaining [{}]), AR idle [{}]. Firing AssetRegistry regen "
                                         "anyway — partial *Assets.as output is possible if AR is still cataloging."),
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

                            // Bail paths below intentionally do NOT clean the
                            // AR sibling — leaving the stub on disk is correct
                            // when the canonical wasn't actually regenerated.
                            // StartupModule's broad sweep on the next launch
                            // (or a subsequent successful PostCompile) handles it.
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

                            // Canonical was just rewritten — NOW the AR sibling
                            // can be cleaned. Doing this in the synchronous
                            // PostCompile lambda would race the regen and
                            // produce the loop pinned by
                            // _probe_assetregistry_loop.
                            const auto ArPatterns = TArray<const TCHAR*>{ sStubPattern_AssetRegistry };
                            Delete_StubRecoveryFiles_ForPatterns(ArPatterns);

                            sg_AnyAssetRegistryRegenInFlight = false;
                            return false; // one-shot
                        }),
                    /*InDelay=*/2.0f);
            });
    }

    // PostCompile-driven DynamicHandle canonical regen — counterpart to
    // Maybe_RegenAssetRegistry_OnPostCompile but lighter: GenerateHandleType-
    // Registry reads already-loaded data assets and writes a small JSON, no
    // shader-contention hazard and no FSlowTask scope conflict (sync call OK).
    //
    // Mid-session value: user adds a new `FCk_Handle_X` reference + data
    // asset, AS fails, dispatcher synthesizes a permissive-validator sibling
    // stub, AS recompiles. Without this hook the canonical JSON stays stale
    // until next launch and the in-memory validator stays permissive. With
    // it, the very next PostCompile rewrites the canonical and upgrades the
    // in-memory validator to strict via DiscoverAndRegisterAllDefinitions
    // (register-or-update path).
    auto Maybe_RegenDynamicHandleJson_OnPostCompile() -> void
    {
        if (NOT Has_DynamicHandleStubRecoveryFile_OnDisk())
        { return; }

        if (NOT GEditor)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] PostCompile DynamicHandle regen wanted, but GEditor is null — skipping."));
            return;
        }

        auto* Subsystem = GEditor->GetEditorSubsystem<UCkDynamicHandleSubsystem>();
        if (Subsystem == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[Module] PostCompile DynamicHandle regen wanted, but UCkDynamicHandleSubsystem isn't loaded — skipping."));
            return;
        }

        ck::angelscriptgenerator::Log(
            TEXT("[Module] PostCompile detected pending _StubRecovery_DynamicHandleTypes.json sibling. ")
            TEXT("Firing GenerateHandleTypeRegistry + DiscoverAndRegisterAllDefinitions."));

        Subsystem->GenerateHandleTypeRegistry();
        FCkDynamic_HandleTypeRegistry::DiscoverAndRegisterAllDefinitions();

        ck::angelscriptgenerator::Log(
            TEXT("[Module] PostCompile DynamicHandle regen complete — strict validators active."));
    }

    // PostCompile-driven AssetRegistry cleanup. Gated narrowly to avoid taxing
    // every successful AS recompile with a full GenerateAllAssetRegistries
    // pass (3.7s optimal, much worse under contention).
    //
    // All gates must hold:
    //   1. OnFEngineLoopInitComplete has fired — keeps this off the cold-start
    //      path so it doesn't race the existing startup-deferred regen.
    //   2. A sibling *Assets.as stub exists on disk — i.e. the dispatcher
    //      synthesized mid-session (or a prior-session stub survived past
    //      startup cleanup). Self-terminates: after the regen rewrites the
    //      canonical files, the sibling is deleted by Delete_AllStubRecovery-
    //      Files in the same PostCompile pass.
    //   3. No regen ticker is already queued from a prior PostCompile.
    //
    // FTSTicker rather than sync call: PostCompile broadcasts from inside
    // FAngelscriptManager::CompileModules (FSlowTask scope "Script Module
    // Compilation"). GenerateAllAssetRegistries opens its own FSlowTask. Sync
    // call leaves the AR slow task on the stack when CompileModules's
    // destructs, tripping the Task==this ensure at SlowTask.cpp:149.
    // Empirically caught 2026-05-13.
    //
    // Mid-session value: user adds a new asset + AS file referencing it. AS
    // fails, dispatcher synthesizes a stub, AS recompiles with the stub.
    // Without this hook the stub sits until next launch; with it, the very
    // next PostCompile rewrites the canonical fresh.
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
            TEXT("[Module] PostCompile detected pending _StubRecovery_*Assets.as sibling file(s). ")
            TEXT("Queueing deferred GenerateAllAssetRegistries."));

        sg_AnyAssetRegistryRegenInFlight = true;

        // Same idle-wait gate as the cold-start path — without AR-idle the
        // regen can fire mid-cataloging and emit partial *Assets.as output.
        auto WaitTicks = MakeShared<int32>(0);
        constexpr int32 MaxWaitTicks = 30; // ~60s at ~2s polling.

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
                            TEXT("[Module] PostCompile idle-wait hit hard cap at {} polls — shaders idle [{}] "
                                 "(jobs remaining [{}]), AR idle [{}]. Firing regen anyway — partial output possible."),
                            *WaitTicks,
                            ShadersIdle,
                            GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0,
                            ArIdle);
                    }
                    else
                    {
                        ck::angelscriptgenerator::Log(
                            TEXT("[Module] PostCompile settled (shader compiler idle AND AR idle) after {} polls. Firing regen."),
                            *WaitTicks);
                    }

                    // Bail paths below intentionally do NOT clean the AR
                    // sibling (see OnPostInit twin for full rationale).
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

                    // Canonical was just rewritten — NOW the AR sibling can be
                    // cleaned. Pinned by _probe_assetregistry_loop.
                    const auto ArPatterns = TArray<const TCHAR*>{ sStubPattern_AssetRegistry };
                    Delete_StubRecoveryFiles_ForPatterns(ArPatterns);

                    sg_AnyAssetRegistryRegenInFlight = false;
                    return false; // one-shot
                }),
            /*InDelay=*/2.0f);
    }

#if WITH_ANGELSCRIPT_CK
    // Self-heal opt-outs:
    //   * `-NoCkAsRegen` CLI flag — per-launch (e.g. debugging an authoring
    //     bug whose error matches one of our recovery patterns).
    //   * `_EnableAsBootstrapSelfHeal` project setting — persistent project-wide.
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
    // Defensive: PostCompile is the only path that deletes self-heal sibling
    // stubs, so a force-quit or crash between stub synthesis and the next
    // clean compile leaves `_StubRecovery_*` files on disk. Carrying them
    // across sessions is unsafe — a stale sibling can collide with canonical
    // regen output and produce duplicate-function errors at the next AS
    // compile. Wipe them before AS hooks are wired; the dispatcher will
    // re-synthesize from scratch if drift is still present.
    {
        const auto DeletedCount = Delete_AllStubRecoveryFiles();
        if (DeletedCount > 0)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] Startup self-heal cleanup: deleted {} stale stub recovery file(s) from prior session."),
                DeletedCount);
        }
    }

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

    // Engine-loop-init completion: PostCompile AssetRegistry cleanup gates on
    // this; dispatcher's bootstrap-vs-mid-session routing also flips here.
    _EngineLoopInitCompleteHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddLambda(
        []()
        {
            sg_EngineLoopInitComplete = true;
            ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Mark_BootstrapComplete();
        });

#if WITH_ANGELSCRIPT_CK
    _PostAngelscriptCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda(
        []()
        {
            Run_AllGenerators();
            Maybe_RegenDynamicHandleJson_OnPostCompile();
            Maybe_RegenAssetRegistry_OnPostCompile();

            // PostCompile fires on a successful compile, so the EntitySpawnParams
            // and DynamicHandle siblings have served their purpose (both
            // generators above are synchronous and have already completed).
            //
            // The AssetRegistry sibling pattern is INTENTIONALLY EXCLUDED here:
            // Maybe_RegenAssetRegistry_OnPostCompile queues an async FTSTicker
            // that hasn't actually rewritten the canonical yet. Deleting the
            // AR sibling synchronously here is the bug pinned by
            // _probe_assetregistry_loop — hot-reload sees the deletion's mtime
            // change, recompiles against the still-stale canonical, fails the
            // same root, and the dispatcher loops. The ticker's post-regen
            // callback owns AR-sibling cleanup.
            const auto PostCompilePatterns = TArray<const TCHAR*>{
                sStubPattern_EntitySpawnParams,
                sStubPattern_DynamicHandle,
            };
            const auto DeletedCount = Delete_StubRecoveryFiles_ForPatterns(PostCompilePatterns);
            if (DeletedCount > 0)
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[Module] PostCompile self-heal cleanup: deleted {} stub recovery file(s) (EntitySpawnParams + DynamicHandle; AssetRegistry sibling cleanup owned by the ticker)."),
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
