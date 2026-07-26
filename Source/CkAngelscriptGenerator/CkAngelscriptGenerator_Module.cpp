#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/AutoTests/CkAutoTestNetStubGenerator.h"
#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/ScriptProcessors/CkScriptProcessorDriverGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptCompileGuard.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"
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

namespace ck_angelscript_generator_module
{
#if WITH_EDITOR
    constexpr auto* sSelfHealLogChannel = TEXT("CkAngelscriptGenerator");

    // Keeps the PostCompile AssetRegistry cleanup off the cold-start path, where
    // GenerateAllAssetRegistries blocks for minutes instead of seconds under contention.
    static bool sg_EngineLoopInitComplete = false;

    static bool sg_AnyAssetRegistryRegenInFlight = false;

    // AR is async (FTSTicker) — its sibling may only be deleted AFTER that ticker's
    // GenerateAllAssetRegistries returns, never from the outer PostCompile lambda.
    constexpr auto* sStubPattern_EntitySpawnParams = TEXT("_StubRecovery_*_EntitySpawnParams.as");
    constexpr auto* sStubPattern_AssetRegistry     = TEXT("_StubRecovery_*Assets.as");
    constexpr auto* sStubPattern_DynamicHandle     = TEXT("_StubRecovery_*.json");

    auto Run_AllGenerators() -> void
    {
        FCkAngelscriptEntityScriptParamsGenerator::GenerateAll();
        FCkAutoTestWrapperGenerator::GenerateAll();
        FCkAutoTestNetStubGenerator::GenerateAll();
        FCkScriptProcessorDriverGenerator::GenerateAll();
    }

    auto Delete_StubRecoveryFiles_ForPatterns(
        TArrayView<const TCHAR* const> Patterns) -> int32
    {
        if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
                TEXT("Module.Delete_StubRecoveryFiles")))
        { return 0; }

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
                    // A stub is a stale survivor only once its canonical exists to supersede it.
                    // While the canonical is absent the stub IS the bootstrap for the upcoming AS
                    // compile, and deleting it here re-wedges the cook retry that synthesized it.
                    const auto CanonicalPath = FPaths::GetPath(File) /
                        FPaths::GetCleanFilename(File).Replace(TEXT("_StubRecovery_"), TEXT(""));
                    if (NOT IFileManager::Get().FileExists(*CanonicalPath))
                    {
                        ck::angelscriptgenerator::Log(
                            TEXT("[Module] Retaining self-heal stub (its canonical is absent — the stub is ")
                            TEXT("the bootstrap for the next AS compile): {}"), File);
                        continue;
                    }

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
                    "Self-heal cleanup: deleted {0} stub recovery file(s)."),
                FText::AsNumber(DeletedCount)));
        }

        return DeletedCount;
    }

    // StartupModule force-quit-survivor sweep only — PostCompile must use the pattern-scoped
    // variant, because the AR pattern is not safe to delete there.
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

    // Deferred to OnPostEngineInit because GEditor is unavailable at modal-tick time, where the
    // dispatcher writes the stub. The on-disk trigger covers a force-quit before cleanup fired.
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

        // Replaces the stub's permissive validator with each data asset's strict one, in place —
        // this routes through the register-or-update path, so no editor restart is needed.
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

        // Reserve the slot now so a PostCompile firing during the idle-wait window bails.
        sg_AnyAssetRegistryRegenInFlight = true;

        // OnFEngineLoopInitComplete, NOT OnPostEngineInit: the latter fires inside init's slow
        // task, so AR opening its own trips an out-of-order FSlowTask destruction ensure.
        // The inner poll must clear BOTH gates — firing mid-cataloging emits partial *Assets.as.
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

                            // Bails below leave the AR sibling on disk on purpose: the canonical
                            // was never regenerated, so a later sweep owns the cleanup.
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

                            // Canonical written; only NOW may the AR sibling be cleaned.
                            const auto ArPatterns = TArray<const TCHAR*>{ sStubPattern_AssetRegistry };
                            Delete_StubRecoveryFiles_ForPatterns(ArPatterns);

                            sg_AnyAssetRegistryRegenInFlight = false;
                            return false; // one-shot
                        }),
                    /*InDelay=*/2.0f);
            });
    }

    // Safe to call synchronously (unlike its AssetRegistry counterpart): this reads already-loaded
    // data assets and writes a small JSON — no shader contention, no nested FSlowTask.
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

    // Narrowly gated: a full GenerateAllAssetRegistries on every successful AS recompile is far
    // too expensive. FTSTicker rather than a sync call because PostCompile broadcasts from inside
    // CompileModules's FSlowTask scope, and a nested AR slow task trips the Task==this ensure.
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

        // Same idle-wait gate as the cold-start path.
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

                    // Bails leave the AR sibling on disk (see OnPostInit twin).
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

                    // Canonical written; only NOW may the AR sibling be cleaned — never from the
                    // outer PostCompile lambda.
                    const auto ArPatterns = TArray<const TCHAR*>{ sStubPattern_AssetRegistry };
                    Delete_StubRecoveryFiles_ForPatterns(ArPatterns);

                    sg_AnyAssetRegistryRegenInFlight = false;
                    return false; // one-shot
                }),
            /*InDelay=*/2.0f);
    }

#if WITH_ANGELSCRIPT_CK
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
    // Must resolve ownership BEFORE the first mutation (the stub sweep below).
    FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(TEXT("Module.StartupModule"));

    // A force-quit between stub synthesis and the next clean compile strands `_StubRecovery_*`
    // files; carried across sessions they collide with canonical regen output and produce
    // duplicate-function AS errors. Wipe before AS hooks are wired — the dispatcher re-synthesizes
    // if drift is still present.
    {
        const auto DeletedCount = ck_angelscript_generator_module::Delete_AllStubRecoveryFiles();
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
            FName{ck_angelscript_generator_module::sSelfHealLogChannel},
            LOCTEXT("MessageLogTitle", "AngelScript Generator"),
            InitOptions);
    }

    _PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda([]()
    {
        ck_angelscript_generator_module::Run_AllGenerators();
        ck_angelscript_generator_module::Maybe_RegenDynamicHandleJson_OnPostInit();
        ck_angelscript_generator_module::Maybe_RegenAssetRegistry_OnPostInit();
    });

    _EngineLoopInitCompleteHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddLambda(
        []()
        {
            ck_angelscript_generator_module::sg_EngineLoopInitComplete = true;
            ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Mark_BootstrapComplete();
        });

#if WITH_ANGELSCRIPT_CK
    _PostAngelscriptCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda(
        []()
        {
            // PostCompile fires on SUCCESSFUL compile only, so any queued recovery action was
            // parsed from errors this compile just invalidated. Drop them FIRST — a later drain
            // would re-synthesize stubs from stale records into the now-healthy state.
            ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Clear_PendingRecoveryState();

            ck_angelscript_generator_module::Run_AllGenerators();
            ck_angelscript_generator_module::Maybe_RegenDynamicHandleJson_OnPostCompile();
            ck_angelscript_generator_module::Maybe_RegenAssetRegistry_OnPostCompile();

            // ESP + DH generators ran synchronously above, so their siblings are safe to clean.
            // AR is INTENTIONALLY EXCLUDED here: its ticker hasn't run yet.
            const auto PostCompilePatterns = TArray<const TCHAR*>{
                ck_angelscript_generator_module::sStubPattern_EntitySpawnParams,
                ck_angelscript_generator_module::sStubPattern_DynamicHandle,
            };
            const auto DeletedCount = ck_angelscript_generator_module::Delete_StubRecoveryFiles_ForPatterns(PostCompilePatterns);
            if (DeletedCount > 0)
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[Module] PostCompile self-heal cleanup: deleted {} stub recovery file(s) (ESP + DH; AR owned by ticker)."),
                    DeletedCount);
            }
        });

    ck::angelscriptgenerator::FCk_AngelscriptCompileGuard::Install();

    if (ck_angelscript_generator_module::Is_SelfHealEnabled())
    {
        ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::Reset_CyclesRun();

        _ReloadHadErrorsHandle = FAngelscriptCodeModule::GetReloadHadErrors().AddStatic(
            &ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::OnAngelscriptReloadHadErrors);
        _SelfHealArmed = true;

        // Self-heal stays ARMED in a secondary — its file-mutating drains are gated, and lazy
        // takeover makes it effective the moment this instance acquires ownership. The banner
        // only names the mode so a stuck-secondary is diagnosable.
        const auto IsOwner = FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("Module.SelfHealArmBanner"));
        if (IsOwner)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] AS bootstrap self-heal armed (Rev 12, cycle cap {})."),
                ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::MaxCycles);
        }
        else
        {
            ck::angelscriptgenerator::Log(
                TEXT("[Module] AS bootstrap self-heal armed in SECONDARY mode (Rev 12, cycle cap {}) ")
                TEXT("— recovery writes are deferred to the owning instance; this instance takes over ")
                TEXT("automatically if the owner exits."),
                ck::angelscriptgenerator::self_heal::FCkAsRecoveryDispatcher::MaxCycles);
        }
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
        MessageLogModule.UnregisterLogListing(FName{ck_angelscript_generator_module::sSelfHealLogChannel});
    }

    // For in-process module reload only — the OS releases the lock on process exit regardless.
    FCkAngelscriptGenerator_RegenOwnership::Release();
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAngelscriptGeneratorModule, CkAngelscriptGenerator)

// --------------------------------------------------------------------------------------------------------------------
