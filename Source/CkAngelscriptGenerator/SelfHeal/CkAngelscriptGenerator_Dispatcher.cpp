#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDynamic/CkDynamic_AngelScript.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#if WITH_ANGELSCRIPT_CK
    #include <AngelscriptManager.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace
    {
        // Session-wide cycle counter. Reset via Reset_CyclesRun at arming time.
        // Editor-only, single-threaded for OnReloadHadErrors invocations
        // (broadcast happens synchronously from CompileModules on the main thread).
        int32 sCyclesRun = 0;

        // Set when the DynamicHandle strategy writes a stub entry into
        // DynamicHandleTypes.json this session. See header docstring for the
        // permissive-validator hazard this exposes. The Module's
        // OnPostEngineInit callback consumes this to fire a deferred JSON
        // regen so next launch is clean.
        bool sDidSynthesizeJsonStub = false;

        // Set when the AssetRegistry strategy writes a stub entry into a
        // *Assets.as file this session. Module's OnPostEngineInit callback
        // consumes this to fire a GenerateAllAssetRegistries pass, which
        // restores correct stub placement (right file per discovery root) and
        // resolves any Tier 3 UObject fallbacks with the real class.
        bool sDidSynthesizeAssetRegistryStub = false;

        // ---- Deferred-apply state --------------------------------------------------
        //
        // Strategies cannot be applied synchronously inside OnReloadHadErrors —
        // empirical finding 2026-05-12: the AS hot-reload checker thread has not
        // yet started at the time of the initial-compile failure broadcast. The
        // thread starts when Hazelight's modal opens, AFTER our hook returns. Its
        // first scan establishes mtime baselines for every .as file, including any
        // we've already written to. With the baseline matching disk, no subsequent
        // scan detects a change — so even though our stub IS on disk, the modal
        // never triggers a retry compile.
        //
        // The fix: defer all file mutations to a callback hooked into
        // FSlateApplication::Get().GetOnModalLoopTickEvent(). The same seam
        // Hazelight uses for its own modal auto-close logic. By the time our
        // tick handler fires, the modal is open, the hot-reload thread is
        // running, and any subsequent file mtime change will be detected on
        // the thread's next scan.
        //
        // sModalTicksToWait gives the thread a few frames to settle its first
        // scan baseline before we modify files. Pure safety margin —
        // empirically the first scan completes within ~20ms (one frame at
        // 60Hz), so even 1 tick should suffice, but a small margin is cheap.
        TArray<FCk_RecoveryAction> sPendingActions;
        FDelegateHandle            sModalTickHandle;
        int32                      sModalTicksWaited  = 0;
        constexpr int32            sModalTicksToWait  = 2;

        // ---- Candidate-file discovery ------------------------------------------

        auto Collect_EntitySpawnParamsCandidates() -> TArray<FString>
        {
            auto Candidates = TArray<FString>{};

            Candidates.Add(FPaths::ProjectDir() / TEXT("Script/Generated") /
                (FApp::GetProjectName() + FString{TEXT("_EntitySpawnParams.as")}));

            // TSharedRef from GetEnabledPlugins is always valid by construction.
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

        // ---- DynamicHandle strategy helpers ----------------------------------------

        // Strip the "FCk_Handle_" prefix to produce a short name suitable for
        // the JSON entry's ShortName field — matches the convention used by
        // the generator and the AS-side `As_<ShortName>()` accessor.
        auto Derive_HandleShortName(
            const FString& InMissingIdentifier) -> FString
        {
            const auto Prefix = FString{TEXT("FCk_Handle_")};
            if (InMissingIdentifier.StartsWith(Prefix))
            { return InMissingIdentifier.RightChop(Prefix.Len()); }
            return InMissingIdentifier;
        }

        // DynamicHandle drift recovery via JSON synthesis.
        //
        // The runtime path Discover+RegisterNewTypesIncremental (called by
        // ForceRefreshDynamicHandleBindings) needs the data asset that
        // corresponds to the missing handle type to be present in the
        // Asset Registry. Those data assets are AS-defined via
        // `asset X of UCkDynamic_HandleDefinition { ... }` declarations in
        // AS source files. When AS fails to compile (because the JSON is
        // missing the entry), those declarations don't materialize and the
        // asset never reaches AR — chicken-and-egg deadlock.
        //
        // We break the deadlock by synthesizing a minimal JSON entry from
        // the error text alone. AS bindings need only TypeName + ShortName;
        // the other fields (Description, SourceAsset, RequiredFragments)
        // can be empty placeholders. The next clean editor regen — via
        // OnPostEngineInit, the "Generate Handle Type Registry" button, or
        // a teammate's editor run — overwrites our placeholder with a
        // proper entry sourced from the data asset.
        auto Apply_DynamicHandleStrategy(
            const FCk_AsParsedError& InError) -> bool
        {
            const auto JsonPath = FCkDynamic_HandleTypeRegistry::GetRegistryFilePath();
            if (JsonPath.IsEmpty())
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: Get_RegistryFilePath returned empty — skipping."));
                return false;
            }

            // Read existing JSON. If missing, start with an empty shell.
            auto ExistingContent = FString{};
            const auto FileExisted = FFileHelper::LoadFileToString(ExistingContent, *JsonPath);
            if (NOT FileExisted)
            {
                Log(TEXT("[SelfHeal] DynamicHandle: registry file missing at '{}' — synthesizing fresh."), JsonPath);
                ExistingContent = TEXT("{\"HandleTypes\":[]}");
            }

            auto RootObj    = TSharedPtr<FJsonObject>{};
            auto JsonReader = TJsonReaderFactory<>::Create(ExistingContent);
            if (NOT FJsonSerializer::Deserialize(JsonReader, RootObj) || NOT RootObj.IsValid())
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to parse existing JSON at '{}' — skipping."), JsonPath);
                return false;
            }

            auto HandleTypes = TArray<TSharedPtr<FJsonValue>>{};
            if (RootObj->HasField(TEXT("HandleTypes")))
            { HandleTypes = RootObj->GetArrayField(TEXT("HandleTypes")); }

            // Bail early if the entry already exists — recovery isn't our problem.
            for (const auto& Entry : HandleTypes)
            {
                const auto Obj = Entry->AsObject();
                if (Obj.IsValid() && Obj->GetStringField(TEXT("TypeName")) == InError.MissingIdentifier)
                {
                    Log(TEXT("[SelfHeal] DynamicHandle: entry for '{}' already present in JSON — refreshing in-memory registry."),
                        InError.MissingIdentifier);

                    FCkDynamic_HandleTypeRegistry::ResetJsonRegistryLoadedFlag();
                    FCkAngelScript_HandleRegistry::ResetBindingsCompleteFlag();
                    FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry();
                    FCkAngelScript_HandleRegistry::RegisterNewTypesIncremental();
                    return true;
                }
            }

            // Synthesize the missing entry. Minimum fields only — anything else
            // is filled by the real generator on a subsequent clean regen.
            const auto ShortName = Derive_HandleShortName(InError.MissingIdentifier);
            auto NewEntry = MakeShared<FJsonObject>();
            NewEntry->SetStringField(TEXT("TypeName"),     InError.MissingIdentifier);
            NewEntry->SetStringField(TEXT("ShortName"),    ShortName);
            NewEntry->SetStringField(TEXT("Description"),
                FString::Printf(TEXT("Synthesized stub for emergency recovery (CkAngelscriptGenerator Rev 10). ")
                                TEXT("Replaced on next clean editor regen.")));
            NewEntry->SetStringField(TEXT("SourceAsset"),  TEXT(""));
            NewEntry->SetArrayField(TEXT("RequiredFragments"), TArray<TSharedPtr<FJsonValue>>{});

            HandleTypes.Add(MakeShared<FJsonValueObject>(NewEntry));
            RootObj->SetArrayField(TEXT("HandleTypes"), HandleTypes);

            // Re-serialize and write atomically.
            auto NewContent = FString{};
            auto Writer     = TJsonWriterFactory<>::Create(&NewContent);
            if (NOT FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer))
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to re-serialize JSON — skipping."));
                return false;
            }

            const auto TempPath = JsonPath + TEXT(".dhsynthtmp");
            IFileManager::Get().Delete(*TempPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);

            if (NOT FFileHelper::SaveStringToFile(NewContent, *TempPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to write temp JSON at '{}' — skipping."), TempPath);
                return false;
            }
            if (NOT IFileManager::Get().Move(*JsonPath, *TempPath, /*Replace=*/true))
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to move temp JSON into place at '{}' — skipping."), JsonPath);
                return false;
            }

            Log(TEXT("[SelfHeal] DynamicHandle: synthesized JSON entry for '{}' (ShortName='{}') -> {}"),
                InError.MissingIdentifier, ShortName, JsonPath);

            // Mark so OnPostEngineInit (where GEditor IS available) can call
            // GenerateHandleTypeRegistry and write a proper JSON entry — fixing
            // next launch and making this stub a one-time event.
            FCkAsRecoveryDispatcher::Mark_JsonStubSynthesized();

            // Re-load JSON + register AS bindings for the newly added type.
            FCkDynamic_HandleTypeRegistry::ResetJsonRegistryLoadedFlag();
            FCkAngelScript_HandleRegistry::ResetBindingsCompleteFlag();
            FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry();
            const auto NewBindingCount = FCkAngelScript_HandleRegistry::RegisterNewTypesIncremental();
            Log(TEXT("[SelfHeal] DynamicHandle: registered {} new AS binding(s) after JSON reload."), NewBindingCount);

            // Note: until the deferred OnPostEngineInit JSON regen runs, the
            // in-memory validator for this type is PERMISSIVE (the synthesized
            // JSON stub has empty RequiredFragments). The
            // FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle path
            // (called from OnPostEngineInit's DiscoverAndRegisterAllDefinitions
            // refresh once the data asset has materialized) replaces the
            // permissive validator with the strict one sourced from the data
            // asset. Until that fires, any handle.As_<ShortName>() cast
            // succeeds unchecked — but the window is short (sub-second after
            // editor reaches main screen) and the only code expected to run in
            // it is editor init itself.
            Log(TEXT("[SelfHeal] Permissive validator in effect for '{}' until OnPostEngineInit ")
                TEXT("deferred regen fires (typically <1 sec after editor reaches main screen)."),
                InError.MissingIdentifier);

            // Nudge the hot-reload thread to trigger a fresh AS compile pass.
            if (NOT InError.FilePath.IsEmpty()
                && IFileManager::Get().FileExists(*InError.FilePath))
            {
                IFileManager::Get().SetTimeStamp(*InError.FilePath, FDateTime::UtcNow());
                Log(TEXT("[SelfHeal] Touched caller mtime: {}"), InError.FilePath);
            }

            return true;
        }

        // ---- Strategy application --------------------------------------------------

        // Returns true if the strategy was successfully applied. False means
        // the action did not (or could not) progress recovery.
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
                    return Apply_DynamicHandleStrategy(InError);
                }

                case ECk_RecoveryStrategy::KickGenerator_AssetRegistry:
                {
                    // AssetRegistry stub synthesis — Rev 10 second pass (2026-05-12).
                    //
                    // The accessor's return type encodes the asset's UClass. We can't
                    // infer it from the error text alone — but we CAN do a sync AR
                    // scan at modal-tick, look up the UCkAssetRegistryConfig matching
                    // the failing namespace, find the asset by name in its discovery
                    // root, and resolve the UClass via two tiers:
                    //   - Tier 1: AssetData.GetClass() for already-loaded native classes
                    //   - Tier 2: sync-load the asset and walk Get_NonBlueprintParentClass
                    //
                    // Tier 3 fallback (when sync load fails) is policy-gated:
                    //   - SoftRef / SoftClass accessors get a UObject stub (the
                    //     caller's typed assignment will fail with a follow-up AS
                    //     error pointing at the right line — better than wedging).
                    //   - BlockingLoad accessors REFUSE the fallback — returning a
                    //     default-constructed asset would crash worse than the wedge.
                    //
                    // On Tier 3 refusal or any other failure, log the actionable
                    // banner and return false (dispatcher cycles will run out and
                    // the user gets the manual-intervention message).
                    const auto Synth = FCkAsAssetRegistryStubSynthesizer::Inject_AssetRegistryStub(InError);

                    if (Synth.Success)
                    {
                        // Mark for OnPostEngineInit deferred regen — even Tier 1/2
                        // success may have landed the stub in the wrong file (file-scan
                        // picks the first namespace match, not necessarily the file
                        // whose discovery root matches the asset's package path). A
                        // GenerateAllAssetRegistries pass at PostEngineInit reshuffles
                        // to correct placement and overwrites the marker comment.
                        FCkAsRecoveryDispatcher::Mark_AssetRegistryStubSynthesized();

                        if (Synth.UsedTier3Fallback)
                        {
                            Warning(TEXT("[SelfHeal] AssetRegistry stub synthesized with Tier 3 fallback ({}=UObject) for ")
                                    TEXT("{}::{}({}) -> {} (asset {}). ")
                                    TEXT("The caller's typed assignment is likely to fail with a follow-up AS error — ")
                                    TEXT("the underlying asset class could not be resolved at modal-tick time."),
                                Synth.ResolvedAssetClass,
                                InError.TargetNamespace, InError.FunctionName, InError.ArgsList,
                                Synth.TargetFilePath, Synth.ResolvedAssetPath);
                        }
                        else
                        {
                            Log(TEXT("[SelfHeal] Synthesized AssetRegistry stub for {}::{}({}) (return type {}, asset {}) -> {}"),
                                InError.TargetNamespace, InError.FunctionName, InError.ArgsList,
                                Synth.ResolvedAssetClass, Synth.ResolvedAssetPath, Synth.TargetFilePath);
                        }
                        return true;
                    }

                    Warning(TEXT("[SelfHeal] AssetRegistry stub synthesis failed for {}::{}({}) at {}:{}:{}: {}"),
                        InError.TargetNamespace, InError.FunctionName, InError.ArgsList,
                        InError.FilePath, InError.Line, InError.Column, Synth.ErrorMessage);
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

        // ---- Modal-tick handler (deferred apply) ----------------------------------

        // Drains sPendingActions and applies the recovery strategies. Runs from
        // the FSlateApplication modal-tick pump during the Hazelight AS-failure
        // modal. By the time we run, the hot-reload thread is established and
        // any file writes we do will be detected on its next scan.
        //
        // Self-cleans on the apply tick: after one drain, removes itself from
        // the modal-tick multicast so subsequent ticks don't re-enter.
        auto OnModalLoopTick(
            float /*InDeltaTime*/) -> void
        {
            // Settle a few ticks before applying — gives Hazelight's hot-reload
            // thread a chance to do its first scan + baseline before we mutate
            // any files. Empirically a single tick (~16ms) is enough but a
            // small margin costs nothing.
            if (sModalTicksWaited < sModalTicksToWait)
            {
                ++sModalTicksWaited;
                return;
            }

            if (sPendingActions.Num() == 0)
            {
                // Queue drained on a prior tick; nothing left to do — unsubscribe.
                if (sModalTickHandle.IsValid() && FSlateApplication::IsInitialized())
                {
                    FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(sModalTickHandle);
                }
                sModalTickHandle.Reset();
                sModalTicksWaited = 0;
                return;
            }

            Log(TEXT("[SelfHeal] Modal-tick deferred apply firing — draining {} pending action(s)."),
                sPendingActions.Num());

            auto AppliedAny = false;
            for (const auto& Action : sPendingActions)
            {
                if (Apply_Strategy(Action.Strategy, Action.Error))
                { AppliedAny = true; }
            }
            sPendingActions.Reset();

            if (AppliedAny)
            {
                ++sCyclesRun;
                Log(TEXT("[SelfHeal] Cycle {} applied at least one strategy. ")
                    TEXT("Hot-reload thread's next scan should pick up the file mtime change."),
                    sCyclesRun);
            }
            else
            {
                Log_TerminalBanner_AllUnactionable(sPendingActions.Num());
            }

            // Unsubscribe — if more cycles are needed, OnAngelscriptReloadHadErrors
            // will resubscribe when the next failure fires.
            if (sModalTickHandle.IsValid() && FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(sModalTickHandle);
            }
            sModalTickHandle.Reset();
            sModalTicksWaited = 0;
        }

        // Ensure we are subscribed to the modal-tick pump. Idempotent — re-entry
        // from a second OnReloadHadErrors invocation while still subscribed is
        // a no-op.
        auto Ensure_ModalTickSubscribed() -> void
        {
            if (sModalTickHandle.IsValid())
            { return; }

            if (NOT FSlateApplication::IsInitialized())
            {
                Warning(TEXT("[SelfHeal] FSlateApplication not initialized — cannot subscribe to ")
                        TEXT("modal-tick pump. Recovery deferral will not fire. (Is this a -nullrhi run?)"));
                return;
            }

            sModalTicksWaited = 0;
            sModalTickHandle  = FSlateApplication::Get().GetOnModalLoopTickEvent().AddStatic(&OnModalLoopTick);
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
                if (InError.TargetNamespace == TEXT("assets")
                    || InError.TargetNamespace.StartsWith(TEXT("assets::")))
                { return ECk_RecoveryStrategy::KickGenerator_AssetRegistry; }

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

    auto FCkAsRecoveryDispatcher::Get_CyclesRun() -> int32 { return sCyclesRun; }

    auto FCkAsRecoveryDispatcher::Reset_CyclesRun() -> void
    {
        sCyclesRun = 0;
        sPendingActions.Reset();
        sModalTicksWaited = 0;
        sDidSynthesizeJsonStub = false;
        sDidSynthesizeAssetRegistryStub = false;
        // sModalTickHandle is left as-is; if a leftover subscription exists from
        // a prior session, the modal-tick handler will detect an empty queue
        // and clean itself up on next fire.
    }

    auto FCkAsRecoveryDispatcher::Did_SynthesizeJsonStub_ThisSession() -> bool
    { return sDidSynthesizeJsonStub; }

    auto FCkAsRecoveryDispatcher::Mark_JsonStubSynthesized() -> void
    { sDidSynthesizeJsonStub = true; }

    auto FCkAsRecoveryDispatcher::Did_SynthesizeAssetRegistryStub_ThisSession() -> bool
    { return sDidSynthesizeAssetRegistryStub; }

    auto FCkAsRecoveryDispatcher::Mark_AssetRegistryStubSynthesized() -> void
    { sDidSynthesizeAssetRegistryStub = true; }

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

        // Build the action plan and queue it for deferred application. The
        // actual file mutations happen from inside OnModalLoopTick — see the
        // comment on sPendingActions for the timing rationale.
        const auto Plan = BuildActionPlan(Roots);
        sPendingActions.Append(Plan);

        Log(TEXT("[SelfHeal] Queued {} recovery action(s) for modal-tick apply (queue depth now {})."),
            Plan.Num(), sPendingActions.Num());

        Ensure_ModalTickSubscribed();
#else
        Warning(TEXT("[SelfHeal] OnAngelscriptReloadHadErrors invoked without WITH_ANGELSCRIPT_CK — no-op."));
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
