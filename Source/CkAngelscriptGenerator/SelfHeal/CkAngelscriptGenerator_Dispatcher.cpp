#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

#if WITH_ANGELSCRIPT_CK
    #include <AngelscriptManager.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace
    {
        // Session-wide cycle counter. Reset via Reset_CyclesRun at arming time.
        // Static lifetime is fine: this is editor-only, single-threaded for
        // OnReloadHadErrors invocations (broadcast happens synchronously from
        // CompileModules on the main thread).
        int32 sCyclesRun = 0;

        // ---- Candidate-file discovery ------------------------------------------

        // Collects all `<Plugin>_EntitySpawnParams.as` files that currently exist
        // on disk. Used by the stub synthesizer to pick the right target file
        // for the namespace it's recovering.
        auto Collect_EntitySpawnParamsCandidates() -> TArray<FString>
        {
            auto Candidates = TArray<FString>{};

            // Project-side file lives at <Project>/Script/Generated/<ProjectName>_EntitySpawnParams.as.
            Candidates.Add(FPaths::ProjectDir() / TEXT("Script/Generated") /
                (FApp::GetProjectName() + FString{TEXT("_EntitySpawnParams.as")}));

            // Plugin-side: each enabled plugin emits its own file under <PluginBase>/Script/Generated.
            // TSharedRef from GetEnabledPlugins is always valid by construction — no nullcheck needed.
            for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
            {
                const auto& PluginName = Plugin->GetName();
                Candidates.Add(Plugin->GetBaseDir() / TEXT("Script/Generated") /
                    (PluginName + FString{TEXT("_EntitySpawnParams.as")}));
            }

            Candidates.RemoveAll([](const FString& Path)
            { return NOT IFileManager::Get().FileExists(*Path); });

            return Candidates;
        }

        // ---- Strategy application --------------------------------------------------

        // Returns true if the strategy was successfully applied. False means
        // the action did not (or could not) progress recovery — caller treats
        // it as if the root had been Unrecognized.
        auto Apply_Strategy(
            ECk_RecoveryStrategy     InStrategy,
            const FCk_AsParsedError& InError) -> bool
        {
            switch (InStrategy)
            {
                case ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams:
                {
                    const auto Candidates = Collect_EntitySpawnParamsCandidates();
                    const auto Result     = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(InError, Candidates);

                    if (Result.Success)
                    {
                        Log(TEXT("[SelfHeal] Synthesized stub for {}::{}({}) -> {}"),
                            InError.TargetNamespace, InError.FunctionName, InError.ArgsList, Result.TargetFilePath);
                        return true;
                    }

                    Warning(TEXT("[SelfHeal] Stub synthesis failed for {}::{}({}): {}"),
                        InError.TargetNamespace, InError.FunctionName, InError.ArgsList, Result.ErrorMessage);
                    return false;
                }

                case ECk_RecoveryStrategy::KickGenerator_DynamicHandle:
                {
                    // v1: classified but not yet wired. Future commit adds the
                    // GenerateHandleTypeRegistry() kick + AS-side binding-cache
                    // reload (otherwise the runtime keeps the stale entries
                    // loaded for the rest of the session).
                    Warning(TEXT("[SelfHeal] Missing dynamic-handle type '{}' (lookup scope: '{}'). ")
                            TEXT("v1 dispatcher does not yet auto-regenerate the registry — run ")
                            TEXT("UCkDynamicHandleSubsystem::GenerateHandleTypeRegistry() from the ")
                            TEXT("editor (or via 'Generate Handle Type Registry' button), then restart."),
                        InError.MissingIdentifier, InError.LookupScope);
                    return false;
                }

                case ECk_RecoveryStrategy::KickGenerator_AssetRegistry:
                {
                    // v1: classified but not yet wired. Future commit adds the
                    // async-pump integration so we can call
                    // Generate_All_Asset_Registries and await its completion
                    // before re-issuing CheckForHotReload (CTO pushback #3).
                    Warning(TEXT("[SelfHeal] Missing asset accessor '{}::{}({})' at {}:{}:{}. ")
                            TEXT("v1 dispatcher does not yet auto-regenerate the asset registry — ")
                            TEXT("run UCk_Utils_AssetRegistry_UE::Generate_All_Asset_Registries() ")
                            TEXT("from the editor."),
                        InError.TargetNamespace, InError.FunctionName, InError.ArgsList,
                        InError.FilePath, InError.Line, InError.Column);
                    return false;
                }

                case ECk_RecoveryStrategy::Unrecognized:
                default:
                {
                    Warning(TEXT("[SelfHeal] Unrecognized root cause at {}:{}:{} — no strategy applies."),
                        InError.FilePath, InError.Line, InError.Column);
                    return false;
                }
            }
        }

        // ---- Terminal-banner logging ----------------------------------------------

        // Single-source of the user-visible "we gave up" message family. Logging
        // at Error level so it lands prominently in the log; a richer Slate
        // banner is a follow-up.
        auto Log_TerminalBanner_NoRoots(const FString& InDiagnostics) -> void
        {
            Error(TEXT("[SelfHeal] AS compile failed with NO recognized root causes. ")
                  TEXT("The dispatcher cannot act on these errors. ")
                  TEXT("Raw Hazelight diagnostics follow:\n{}"),
                InDiagnostics);
        }

        auto Log_TerminalBanner_AllUnactionable(int32 InRootCount) -> void
        {
            Error(TEXT("[SelfHeal] Recognized {} root cause(s), but none mapped to an actionable ")
                  TEXT("strategy in this v1 dispatcher. Per-root diagnostics are in the warnings ")
                  TEXT("logged above. Manual intervention required."),
                InRootCount);
        }

        auto Log_TerminalBanner_MaxCyclesExceeded() -> void
        {
            Error(TEXT("[SelfHeal] Recovery cycle cap ({}) exceeded. The dispatcher will not ")
                  TEXT("attempt further recovery this session. Restart the editor after fixing the ")
                  TEXT("underlying AS issue manually."),
                FCkAsRecoveryDispatcher::MaxCycles);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsRecoveryDispatcher::
        Classify(
            const FCk_AsParsedError& InError)
        -> ECk_RecoveryStrategy
    {
        switch (InError.Kind)
        {
            case ECk_AsParsedError_Kind::NoMatchingSignatures:
            {
                // "assets::X(...)" or "assets::sub::X(...)" -> asset registry.
                if (InError.TargetNamespace == TEXT("assets")
                    || InError.TargetNamespace.StartsWith(TEXT("assets::")))
                { return ECk_RecoveryStrategy::KickGenerator_AssetRegistry; }

                // "UBb_X_EntityScript::Params(...)" / "UCk_X_EntityScript::Params(...)" etc.
                // The U-prefix is the entity-script-class indicator across BB and the framework.
                if (InError.TargetNamespace.StartsWith(TEXT("U"))
                    && InError.FunctionName == TEXT("Params"))
                { return ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams; }

                return ECk_RecoveryStrategy::Unrecognized;
            }

            case ECk_AsParsedError_Kind::IdentifierNotADataType:
            {
                if (InError.MissingIdentifier.StartsWith(TEXT("FCk_Handle_")))
                { return ECk_RecoveryStrategy::KickGenerator_DynamicHandle; }

                return ECk_RecoveryStrategy::Unrecognized;
            }
        }
        return ECk_RecoveryStrategy::Unrecognized;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsRecoveryDispatcher::
        BuildActionPlan(
            const TArray<FCk_AsParsedError>& InDedupedRoots)
        -> TArray<FCk_RecoveryAction>
    {
        auto Plan = TArray<FCk_RecoveryAction>{};
        Plan.Reserve(InDedupedRoots.Num());

        for (const auto& Root : InDedupedRoots)
        {
            auto Action     = FCk_RecoveryAction{};
            Action.Strategy = Classify(Root);
            Action.Error    = Root;
            Plan.Add(MoveTemp(Action));
        }
        return Plan;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsRecoveryDispatcher::
        Get_CyclesRun()
        -> int32
    {
        return sCyclesRun;
    }

    auto
        FCkAsRecoveryDispatcher::
        Reset_CyclesRun()
        -> void
    {
        sCyclesRun = 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsRecoveryDispatcher::
        OnAngelscriptReloadHadErrors()
        -> void
    {
#if WITH_ANGELSCRIPT_CK
        if (sCyclesRun >= MaxCycles)
        {
            Log_TerminalBanner_MaxCyclesExceeded();
            return;
        }

        const auto Diagnostics = FAngelscriptManager::Get().FormatDiagnostics();
        const auto Errors      = FCkAsErrorParser::ParseErrors(Diagnostics);
        const auto Roots       = FCkAsErrorParser::DeduplicateRoots(Errors);

        Log(TEXT("[SelfHeal] OnReloadHadErrors fired (cycle {} of {}). ")
            TEXT("Parsed {} actionable roots from {} raw error records."),
            sCyclesRun + 1, MaxCycles, Roots.Num(), Errors.Num());

        if (Roots.Num() == 0)
        {
            Log_TerminalBanner_NoRoots(Diagnostics);
            return;
        }

        const auto Plan = BuildActionPlan(Roots);

        auto AppliedAny = false;
        for (const auto& Action : Plan)
        {
            if (Apply_Strategy(Action.Strategy, Action.Error))
            { AppliedAny = true; }
        }

        if (NOT AppliedAny)
        {
            Log_TerminalBanner_AllUnactionable(Roots.Num());
            return;
        }

        ++sCyclesRun;

        Log(TEXT("[SelfHeal] Cycle {} applied at least one strategy. Triggering CheckForHotReload(FullReload)."),
            sCyclesRun);

        FAngelscriptManager::Get().CheckForHotReload(ECompileType::FullReload);
#else
        Warning(TEXT("[SelfHeal] OnAngelscriptReloadHadErrors invoked without WITH_ANGELSCRIPT_CK — no-op."));
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
