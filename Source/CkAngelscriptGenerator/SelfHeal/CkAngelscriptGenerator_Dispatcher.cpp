#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDynamic/CkDynamic_AngelScript.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Containers/Ticker.h"
#include "Logging/MessageLog.h"
#include "MessageLogModule.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "CkSelfHeal"

#if WITH_ANGELSCRIPT_CK
    #include <AngelscriptManager.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace
    {
        // OnReloadHadErrors broadcasts synchronously from CompileModules on the
        // main thread; all session state below is single-threaded.
        int32 sCyclesRun = 0;
        bool  sDidSynthesizeJsonStub = false;
        bool  sDidSynthesizeAssetRegistryStub = false;
        bool  sBootstrapComplete = false;

        // Per-signature convergence tracking. Mid-session synthesis was
        // unbounded before May 2026 — a "dueling-overloads" loop (e.g. caller
        // arg order doesn't match the entity script's declared field order, or
        // two callers force mutually-exclusive overload shapes) re-synthesized
        // the same stubs every cleanup boundary and the editor never settled.
        // Each key tracks count + observed callsites; on hitting
        // MaxPerSignatureRepeats the key is blacklisted, the breaker fires
        // Log_TerminalBanner_ConvergenceFailed, and no further synthesis is
        // attempted this session for that signature. Cleared at cold-launch
        // (Reset_CyclesRun) and at the bootstrap→mid-session transition
        // (Mark_BootstrapComplete) — same semantics as sCyclesRun.
        struct FConvergenceTracker
        {
            int32          RecoveryCount = 0;
            TSet<FString>  Callsites;  // "file:line:col" each
        };
        TMap<FString, FConvergenceTracker> sPerSignatureRecoveryCount;
        TSet<FString>                      sBlacklistedSignatures;

        // Cold-start deferral: file mutations applied inside OnReloadHadErrors
        // are invisible to Hazelight's AS hot-reload checker thread because that
        // thread hasn't started yet — its first scan establishes mtime baselines
        // for every .as file, including any we've already written, so no
        // subsequent scan detects a change. We defer to the modal-tick pump
        // which fires AFTER the modal opens and the thread is running.
        // Empirically caught 2026-05-12.
        //
        // sModalTicksToWait is a safety margin for the thread's first-scan
        // baseline. Empirically one tick (~16ms at 60Hz) suffices.
        TArray<FCk_RecoveryAction> sPendingActions;
        FDelegateHandle            sModalTickHandle;
        int32                      sModalTicksWaited  = 0;
        constexpr int32            sModalTicksToWait  = 2;

        // Mid-session deferral (Issue #7, 2026-05-13): mid-session hot-reload
        // failures do NOT open a Hazelight modal, so modal-tick never fires.
        // FTSTicker covers that case. No settle margin needed — the AS hot-
        // reload thread is already running and quiescent between scans.
        FTSTicker::FDelegateHandle sTickerHandle;
        constexpr float            sTickerDelaySeconds = 0.15f;

        // Channel name shared with the Module's RegisterLogListing call.
        constexpr auto* sSelfHealLogChannel = TEXT("CkAngelscriptGenerator");

        // Held across the OnReloadHadErrors → modal-tick-apply lifetime so the
        // apply path can transition the in-progress toast in place rather than
        // spawning a new one. Weak pointer; the notification's 30s ExpireDuration
        // is the safety net if anything orphans it.
        TWeakPtr<SNotificationItem> sInProgressNotification;

        // Fallback-path coalescing. The in-progress→final transition above is
        // already singleton'd via sInProgressNotification, so spam can only
        // arise from the fallback branches in Show_RecoveryToast /
        // Show_TerminalToast — mid-session ticker drains and any path where
        // no in-progress toast was spawned. Identical-content bursts within
        // kCoalesceWindowSeconds collapse into one toast with a (xN) counter.
        enum class ECk_BannerKind : uint8 { None, Recovery, Terminal };

        struct FLastBannerState
        {
            uint32                       ContentHash    = 0;
            ECk_BannerKind               Kind           = ECk_BannerKind::None;
            double                       FirstShownTime = 0.0;
            double                       LastShownTime  = 0.0;
            int32                        RepeatCount    = 1;
            FText                        BaseSummary;
            TWeakPtr<SNotificationItem>  Item;
        };
        static FLastBannerState sLastBanner;
        constexpr double        kCoalesceWindowSeconds = 10.0;

        // ---- Candidate-file discovery ----------------------------------------------

        auto Collect_EntitySpawnParamsCandidates() -> TArray<FString>
        {
            auto Candidates = TArray<FString>{};

            Candidates.Add(FPaths::ProjectDir() / TEXT("Script/Generated") /
                (FApp::GetProjectName() + FString{TEXT("_EntitySpawnParams.as")}));

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

        // ---- DynamicHandle strategy ------------------------------------------------

        // Chicken-and-egg: a missing handle's data asset can't materialize until
        // AS compiles, but AS won't compile because the JSON lacks the entry. We
        // break it by synthesizing a minimal JSON entry from the error text alone
        // (TypeName + ShortName are sufficient for AS bindings; the rest are
        // placeholders that the next clean regen sources from the data asset).

        auto Derive_HandleShortName(
            const FString& InMissingIdentifier) -> FString
        {
            const auto Prefix = FString{TEXT("FCk_Handle_")};
            if (InMissingIdentifier.StartsWith(Prefix))
            { return InMissingIdentifier.RightChop(Prefix.Len()); }
            return InMissingIdentifier;
        }

        // Sibling stub path: same dir as canonical, filename prefixed with
        // `_StubRecovery_`. Mirrors EntitySpawnParams + AssetRegistry conventions.
        auto Derive_DynamicHandleStubPath(
            const FString& InCanonicalJsonPath) -> FString
        {
            if (InCanonicalJsonPath.IsEmpty())
            { return FString{}; }

            const auto Dir      = FPaths::GetPath(InCanonicalJsonPath);
            const auto BaseName = FPaths::GetCleanFilename(InCanonicalJsonPath);
            return Dir / (FString{TEXT("_StubRecovery_")} + BaseName);
        }

        auto Apply_DynamicHandleStrategy(
            const FCk_AsParsedError& InError) -> bool
        {
            const auto CanonicalJsonPath = FCkDynamic_HandleTypeRegistry::GetRegistryFilePath();
            if (CanonicalJsonPath.IsEmpty())
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: Get_RegistryFilePath returned empty — skipping."));
                return false;
            }

            const auto StubJsonPath = Derive_DynamicHandleStubPath(CanonicalJsonPath);
            if (StubJsonPath.IsEmpty())
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to derive stub sibling path — skipping."));
                return false;
            }

            auto ExistingContent = FString{};
            const auto StubExisted = FFileHelper::LoadFileToString(ExistingContent, *StubJsonPath);
            if (NOT StubExisted)
            {
                Log(TEXT("[SelfHeal] DynamicHandle: stub sibling missing at '{}' — synthesizing fresh."), StubJsonPath);
                ExistingContent = TEXT("{\"_WARNING\":\"AUTO-GENERATED RECOVERY STUBS. This file is gitignored and self-cleans after successful AS compile. Do not edit by hand.\",\"HandleTypes\":[]}");
            }

            auto RootObj    = TSharedPtr<FJsonObject>{};
            auto JsonReader = TJsonReaderFactory<>::Create(ExistingContent);
            if (NOT FJsonSerializer::Deserialize(JsonReader, RootObj) || NOT RootObj.IsValid())
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to parse stub JSON at '{}' — skipping."), StubJsonPath);
                return false;
            }

            // Ensure warning field present even if a prior write missed it.
            RootObj->SetStringField(TEXT("_WARNING"),
                TEXT("AUTO-GENERATED RECOVERY STUBS. This file is gitignored and self-cleans after successful AS compile. Do not edit by hand."));

            auto HandleTypes = TArray<TSharedPtr<FJsonValue>>{};
            if (RootObj->HasField(TEXT("HandleTypes")))
            { HandleTypes = RootObj->GetArrayField(TEXT("HandleTypes")); }

            // Already-recovered: refresh in-memory registry and bail.
            for (const auto& Entry : HandleTypes)
            {
                const auto Obj = Entry->AsObject();
                if (Obj.IsValid() && Obj->GetStringField(TEXT("TypeName")) == InError.MissingIdentifier)
                {
                    Log(TEXT("[SelfHeal] DynamicHandle: stub entry for '{}' already present — refreshing in-memory registry."),
                        InError.MissingIdentifier);

                    FCkDynamic_HandleTypeRegistry::ResetJsonRegistryLoadedFlag();
                    FCkAngelScript_HandleRegistry::ResetBindingsCompleteFlag();
                    FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry();
                    FCkAngelScript_HandleRegistry::RegisterNewTypesIncremental();
                    return true;
                }
            }

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

            // Atomic write to SIBLING stub file. Canonical JSON never touched.
            auto NewContent = FString{};
            auto Writer     = TJsonWriterFactory<>::Create(&NewContent);
            if (NOT FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer))
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to re-serialize stub JSON — skipping."));
                return false;
            }

            const auto TempPath = StubJsonPath + TEXT(".dhsynthtmp");
            IFileManager::Get().Delete(*TempPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);

            if (NOT FFileHelper::SaveStringToFile(NewContent, *TempPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to write temp stub JSON at '{}' — skipping."), TempPath);
                return false;
            }
            if (NOT IFileManager::Get().Move(*StubJsonPath, *TempPath, /*Replace=*/true))
            {
                Warning(TEXT("[SelfHeal] DynamicHandle: failed to move temp stub JSON into place at '{}' — skipping."), StubJsonPath);
                return false;
            }

            Log(TEXT("[SelfHeal] DynamicHandle: synthesized JSON stub entry for '{}' (ShortName='{}') -> {}"),
                InError.MissingIdentifier, ShortName, StubJsonPath);

            FCkAsRecoveryDispatcher::Mark_JsonStubSynthesized();

            FCkDynamic_HandleTypeRegistry::ResetJsonRegistryLoadedFlag();
            FCkAngelScript_HandleRegistry::ResetBindingsCompleteFlag();
            FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry();
            const auto NewBindingCount = FCkAngelScript_HandleRegistry::RegisterNewTypesIncremental();
            Log(TEXT("[SelfHeal] DynamicHandle: registered {} new AS binding(s) after JSON reload."), NewBindingCount);

            // Stub's empty RequiredFragments registers a PERMISSIVE validator —
            // handle.As_<ShortName>() casts succeed unchecked until OnPostEngineInit's
            // DiscoverAndRegisterAllDefinitions upgrades it to strict via
            // UpdateExistingDynamicHandle. Window is sub-second after main screen.
            Log(TEXT("[SelfHeal] Permissive validator in effect for '{}' until OnPostEngineInit deferred regen fires."),
                InError.MissingIdentifier);

            // Nudge hot-reload thread to trigger a fresh AS compile.
            if (NOT InError.FilePath.IsEmpty()
                && IFileManager::Get().FileExists(*InError.FilePath))
            {
                IFileManager::Get().SetTimeStamp(*InError.FilePath, FDateTime::UtcNow());
                Log(TEXT("[SelfHeal] Touched caller mtime: {}"), InError.FilePath);
            }

            return true;
        }

        // ---- Strategy application --------------------------------------------------

        // ESP-strategy actions arrive via two error shapes: the classic
        // `U<X>::Params(<args>)` overload miss (NoMatchingSignatures), and
        // the direct-construction shapes — `F<X>_SpawnParams` as a missing
        // declared type (IdentifierNotADataType) or as a bare ctor call
        // (BareCtorNoMatchingSignatures). The latter two carry the struct
        // name in MissingIdentifier instead of a namespace + signature.
        auto Get_EspStructErrorIdentifier(
            const FCk_AsParsedError& InError) -> FString
        {
            const auto IsStructShapedError =
                   InError.Kind == ECk_AsParsedError_Kind::IdentifierNotADataType
                || InError.Kind == ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures;

            return IsStructShapedError ? InError.MissingIdentifier : FString{};
        }

        auto Apply_Strategy(
            ECk_RecoveryStrategy     InStrategy,
            const FCk_AsParsedError& InError) -> bool
        {
            switch (InStrategy)
            {
                case ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams:
                {
                    const auto Candidates = Collect_EntitySpawnParamsCandidates();

                    const auto StructIdentifier = Get_EspStructErrorIdentifier(InError);
                    const auto ClassName = StructIdentifier.IsEmpty()
                        ? InError.TargetNamespace
                        : FCkAsStubSynthesizer::Derive_ClassNameFromStructName(StructIdentifier);

                    if (ClassName.IsEmpty())
                    {
                        Warning(TEXT("[SelfHeal] Cannot derive an entity-script class from '{}' — no stub synthesized."),
                            StructIdentifier);
                        return false;
                    }

                    // Prefer source-derived full-shape synthesis: parse the
                    // entity-script class's own .as declaration for the
                    // complete ExposeOnSpawn field set, so direct
                    // construction and field-access callers (`P.Phase = ...`)
                    // heal too — the wholesale-missing case a gitignored
                    // canonical creates on every fresh clone. Falls back to
                    // the error-text stub when the class source can't be
                    // found/parsed, or when the struct already exists in the
                    // canonical (incremental drift — the per-signature path
                    // owns that, and it's the battle-tested behavior).
                    const auto SourceDerived = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
                        ClassName, InError, Candidates, FCkAsSourceScanner::Get_DefaultScanRoots());

                    if (SourceDerived.Success)
                    {
                        Log(TEXT("[SelfHeal] Source-derived full-shape stub for {} -> {}"),
                            ClassName, SourceDerived.TargetFilePath);
                        return true;
                    }

                    Log(TEXT("[SelfHeal] Source-derived synthesis unavailable for {} ({}) — falling back to error-text stub."),
                        ClassName, SourceDerived.ErrorMessage);

                    // Error-text fallback. Direct-construction errors carry
                    // no namespace signature — synthesize the no-arg shape
                    // (empty struct + Params()), which heals no-arg
                    // construction and Params() callers; field-access
                    // callers are unrecoverable without the class source and
                    // surface the convergence banner.
                    auto FallbackError = InError;
                    if (NOT StructIdentifier.IsEmpty())
                    {
                        FallbackError                 = FCk_AsParsedError{};
                        FallbackError.Kind            = ECk_AsParsedError_Kind::NoMatchingSignatures;
                        FallbackError.FilePath        = InError.FilePath;
                        FallbackError.Line            = InError.Line;
                        FallbackError.Column          = InError.Column;
                        FallbackError.TargetNamespace = ClassName;
                        FallbackError.FunctionName    = TEXT("Params");
                    }

                    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(FallbackError, Candidates);

                    if (Result.Success)
                    {
                        Log(TEXT("[SelfHeal] Synthesized stub for {}::{}({}) -> {}"),
                            FallbackError.TargetNamespace, FallbackError.FunctionName, FallbackError.ArgsList, Result.TargetFilePath);
                        return true;
                    }

                    Warning(TEXT("[SelfHeal] Stub synthesis failed for {}::{}({}): {}"),
                        FallbackError.TargetNamespace, FallbackError.FunctionName, FallbackError.ArgsList, Result.ErrorMessage);
                    return false;
                }

                case ECk_RecoveryStrategy::KickGenerator_DynamicHandle:
                {
                    return Apply_DynamicHandleStrategy(InError);
                }

                case ECk_RecoveryStrategy::KickGenerator_AssetRegistry:
                {
                    // Tier 1: AssetData.GetClass() for loaded native classes.
                    // Tier 2: sync-load asset + Get_NonBlueprintParentClass walk.
                    // Tier 3 (sync load fails): REFUSED for all flavors as of
                    // 2026-05-13 — see AssetRegistryStub.h docstring for the
                    // probe_a2.log rationale. The synthesizer returns Success=false
                    // with an actionable manual-recovery banner; we log it and
                    // bail so Hazelight's modal continues displaying the original
                    // `No matching signatures` error (actionable) instead of a
                    // parser-blind typed-conversion derivative (wedging).
                    const auto Synth = FCkAsAssetRegistryStubSynthesizer::Inject_AssetRegistryStub(InError);

                    if (Synth.Success)
                    {
                        FCkAsRecoveryDispatcher::Mark_AssetRegistryStubSynthesized();

                        Log(TEXT("[SelfHeal] Synthesized AssetRegistry stub for {}::{}({}) (return type {}, asset {}) -> {}"),
                            InError.TargetNamespace, InError.FunctionName, InError.ArgsList,
                            Synth.ResolvedAssetClass, Synth.ResolvedAssetPath, Synth.TargetFilePath);
                        return true;
                    }

                    Warning(TEXT("[SelfHeal] AssetRegistry stub synthesis failed for {}::{}({}) at {}:{}:{}: {}"),
                        InError.TargetNamespace, InError.FunctionName, InError.ArgsList,
                        InError.FilePath, InError.Line, InError.Column, Synth.ErrorMessage);
                    return false;
                }

                case ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral:
                {
                    // No auto-fix — modifying user source is out of contract for
                    // the dispatcher. Emit an actionable diagnostic; return true
                    // so the dispatcher's outer flow doesn't fall through to the
                    // "all-actions-failed" terminal banner (this IS the
                    // recognized action — diagnose and surface).
                    Error(TEXT("[SelfHeal] AS compile error at {}:{}:{} — adjacent string literals not supported. ")
                          TEXT("AngelScript does not splice `\"foo \" \"bar\"` C-style across lines. ")
                          TEXT("Fix: join the literals into one string, or chain via f\"{{Base}}continuation\" / a local variable. ")
                          TEXT("(In headless test runs the editor stalls after AS post-compile without this diagnostic — ")
                          TEXT("the matching error in toolbox stdout is your tip-off.)"),
                        InError.FilePath, InError.Line, InError.Column);
                    return true;
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

        // ---- Terminal-banner logging -----------------------------------------------

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
                  TEXT("strategy. Per-root diagnostics are in the warnings logged above. Manual intervention required."),
                InRootCount);
        }

        auto Log_TerminalBanner_MaxCyclesExceeded() -> void
        {
            Error(TEXT("[SelfHeal] Recovery cycle cap ({}) exceeded. The dispatcher will not ")
                  TEXT("attempt further recovery this session. Restart the editor after fixing the ")
                  TEXT("underlying AS issue manually."),
                FCkAsRecoveryDispatcher::MaxCycles);
        }

        auto Log_TerminalBanner_ConvergenceFailed(
            const FString&       InKey,
            const TSet<FString>& InCallsites) -> void
        {
            auto Sites = FString{};
            for (const auto& Site : InCallsites)
            {
                Sites.Append(LINE_TERMINATOR);
                Sites.Append(TEXT("  - "));
                Sites.Append(Site);
            }

            Error(TEXT("[SelfHeal] Convergence failed for {} after {} synthesis attempts. ")
                  TEXT("Root cause is upstream of self-heal — likely a caller/entity-script ")
                  TEXT("signature mismatch (arg order, mutability, or type drift).{}")
                  TEXT("Callers seen:{}{}")
                  TEXT("Self-heal will NOT retry this signature for the rest of this session. ")
                  TEXT("Fix the call site or the entity script, then restart the editor."),
                InKey,
                FCkAsRecoveryDispatcher::MaxPerSignatureRepeats,
                LINE_TERMINATOR,
                Sites,
                LINE_TERMINATOR);

            // Also surface in MessageLog — the "View details" hyperlink on the
            // toast opens MessageLog, so the diagnostic needs to live here too.
            // (The regular log goes to Saved/Logs/<Project>.log only, which the
            // user can't reach from the toast.)
            auto MessageLog = FMessageLog{FName{sSelfHealLogChannel}};
            auto SitesAsBullets = FString{};
            for (const auto& Site : InCallsites)
            {
                if (NOT SitesAsBullets.IsEmpty())
                { SitesAsBullets.Append(LINE_TERMINATOR); }
                SitesAsBullets.Append(TEXT("    \x2022 "));
                SitesAsBullets.Append(Site);
            }

            MessageLog.Error(FText::Format(
                LOCTEXT("ConvergenceFailedMessageLog",
                    "Self-heal CONVERGENCE FAILED for {0} after {1} synthesis attempts.\n"
                    "Root cause is upstream of self-heal — caller or entity-script signature "
                    "mismatch (arg order, mutability, or type drift).\n"
                    "Callers seen:\n{2}\n"
                    "Self-heal will not retry this signature for the rest of this session. "
                    "Fix the call site(s) or the entity script, then restart the editor."),
                FText::FromString(InKey),
                FText::AsNumber(FCkAsRecoveryDispatcher::MaxPerSignatureRepeats),
                FText::FromString(SitesAsBullets)));
        }

        // ---- UI surfacing (Slate toast + MessageLog) -------------------------------
        //
        // Single notification lifecycle, transitioning in place:
        //   1. In-progress — spawned at OnReloadHadErrors (cold-start only).
        //      User sees "self-heal attempting recovery" alongside Hazelight's modal.
        //   2. Recovered — transitioned at successful apply (CS_Success, fade out).
        //   3. Failed — transitioned at terminal-banner paths (CS_Fail, longer hold).
        //
        // Skipped mid-session: recovery completes in <200ms with no modal to
        // mediate panic, so a throbber would just be noise.

        auto Describe_Action(
            const FCk_RecoveryAction& InAction) -> FString
        {
            switch (InAction.Strategy)
            {
                case ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams:
                case ECk_RecoveryStrategy::KickGenerator_AssetRegistry:
                {
                    if (const auto StructIdentifier = Get_EspStructErrorIdentifier(InAction.Error);
                        NOT StructIdentifier.IsEmpty())
                    {
                        return FString::Printf(TEXT("%s (direct construction)"), *StructIdentifier);
                    }

                    return FString::Printf(TEXT("%s::%s(%s)"),
                        *InAction.Error.TargetNamespace,
                        *InAction.Error.FunctionName,
                        *InAction.Error.ArgsList);
                }
                case ECk_RecoveryStrategy::KickGenerator_DynamicHandle:
                {
                    return InAction.Error.MissingIdentifier;
                }
                case ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral:
                {
                    return FString::Printf(TEXT("adjacent string literals @ %s:%d:%d"),
                        *InAction.Error.FilePath, InAction.Error.Line, InAction.Error.Column);
                }
                case ECk_RecoveryStrategy::Unrecognized:
                default:
                {
                    return TEXT("<unrecognized>");
                }
            }
        }

        auto Log_AppliedActions_ToMessageLog(
            const TArray<FCk_RecoveryAction>& InApplied) -> void
        {
            auto MessageLog = FMessageLog{FName{sSelfHealLogChannel}};
            for (const auto& Action : InApplied)
            {
                const auto Caller = Action.Error.FilePath.IsEmpty()
                    ? FString{TEXT("<unknown caller>")}
                    : FString::Printf(TEXT("%s:%d:%d"),
                        *Action.Error.FilePath, Action.Error.Line, Action.Error.Column);

                MessageLog.Info(FText::Format(
                    LOCTEXT("RecoveryEntry", "Self-heal recovered: {0} (caller {1})"),
                    FText::FromString(Describe_Action(Action)),
                    FText::FromString(Caller)));
            }
        }

        auto Show_InProgressToast() -> void
        {
            if (NOT FSlateApplication::IsInitialized())
            { return; }

            if (sInProgressNotification.IsValid())
            { return; }

            auto Info = FNotificationInfo{LOCTEXT("RecoveryInProgressToast",
                "AngelScript self-heal is attempting to recover from the compile errors shown.\n"
                "This is normal — please wait a moment before closing the editor.")};
            Info.bFireAndForget       = true;
            Info.ExpireDuration       = 30.0f;
            Info.bUseLargeFont        = false;
            Info.bUseThrobber         = true;
            Info.bUseSuccessFailIcons = false;
            Info.Hyperlink            = FSimpleDelegate::CreateLambda([]()
            {
                if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
                {
                    auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
                    MessageLogModule.OpenMessageLog(FName{sSelfHealLogChannel});
                }
            });
            Info.HyperlinkText = LOCTEXT("ViewLog", "View details");

            const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
            if (NotificationPtr.IsValid())
            {
                NotificationPtr->SetCompletionState(SNotificationItem::CS_Pending);
                sInProgressNotification = NotificationPtr;
            }

            // Orphan guard. If modal-tick doesn't transition this notification
            // within 5s, the failure path that fired OnReloadHadErrors did NOT
            // open a Hazelight modal (typical "Cycle 2 with no modal" hot-reload
            // retry after a successful initial-compile recovery — finding
            // 2026-05-13). Cycle 1 transitions in ~1.2s empirically, so 5s is
            // comfortably outside the normal path.
            FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateLambda([](float) -> bool
                {
                    if (auto Item = sInProgressNotification.Pin(); Item.IsValid())
                    {
                        Item->SetCompletionState(SNotificationItem::CS_None);
                        Item->ExpireAndFadeout();
                        sInProgressNotification.Reset();
                    }
                    return false; // one-shot
                }),
                5.0f);
        }

        auto Compute_BannerHash(
            ECk_BannerKind InKind,
            const FText&   InSummary,
            const FText&   InSubText) -> uint32
        {
            const auto KindHash    = GetTypeHash(static_cast<uint8>(InKind));
            const auto SummaryHash = GetTypeHash(InSummary.ToString());
            const auto SubHash     = GetTypeHash(InSubText.ToString());
            return HashCombine(HashCombine(KindHash, SummaryHash), SubHash);
        }

        auto Describe_BannerKind(
            ECk_BannerKind InKind) -> const TCHAR*
        {
            switch (InKind)
            {
                case ECk_BannerKind::Recovery: return TEXT("recovery");
                case ECk_BannerKind::Terminal: return TEXT("terminal");
                default:                       return TEXT("none");
            }
        }

        // Returns true when the incoming banner matched the last one and was
        // folded into it in place. Callers must skip their AddNotification path
        // in that case.
        auto TryCoalesce_LastBanner(
            ECk_BannerKind InKind,
            const FText&   InSummary,
            const FText&   InSubText,
            float          InRefreshedExpireDuration) -> bool
        {
            const auto Item = sLastBanner.Item.Pin();
            if (NOT Item.IsValid())
            { return false; }

            const auto Now = FPlatformTime::Seconds();
            if (Now - sLastBanner.LastShownTime >= kCoalesceWindowSeconds)
            { return false; }

            const auto IncomingHash = Compute_BannerHash(InKind, InSummary, InSubText);
            if (IncomingHash != sLastBanner.ContentHash
                || InKind != sLastBanner.Kind)
            { return false; }

            ++sLastBanner.RepeatCount;
            sLastBanner.LastShownTime = Now;

            const auto CountedSummary = FText::Format(
                LOCTEXT("BannerCoalescedFmt", "{0} (\x00D7{1})"),
                sLastBanner.BaseSummary,
                FText::AsNumber(sLastBanner.RepeatCount));

            Item->SetText(CountedSummary);
            Item->SetExpireDuration(InRefreshedExpireDuration);
            return true;
        }

        // Record_NewBanner is called after a fresh fallback toast was spawned.
        // If the previous tracked banner accumulated more than one event in its
        // window, emit a postmortem MessageLog entry before overwriting state.
        auto Record_NewBanner(
            ECk_BannerKind                     InKind,
            const FText&                       InBaseSummary,
            const FText&                       InSubText,
            const TWeakPtr<SNotificationItem>& InItem) -> void
        {
            if (sLastBanner.RepeatCount > 1)
            {
                auto MessageLog = FMessageLog{FName{sSelfHealLogChannel}};
                const auto BurstDuration = sLastBanner.LastShownTime - sLastBanner.FirstShownTime;
                MessageLog.Info(FText::Format(
                    LOCTEXT("BurstSummary",
                        "Self-heal {0} burst suppressed: {1} identical events in {2}s "
                        "(prior banner: \"{3}\")"),
                    FText::FromString(Describe_BannerKind(sLastBanner.Kind)),
                    FText::AsNumber(sLastBanner.RepeatCount),
                    FText::AsNumber(BurstDuration),
                    sLastBanner.BaseSummary));
            }

            const auto Now = FPlatformTime::Seconds();
            sLastBanner.ContentHash    = Compute_BannerHash(InKind, InBaseSummary, InSubText);
            sLastBanner.Kind           = InKind;
            sLastBanner.FirstShownTime = Now;
            sLastBanner.LastShownTime  = Now;
            sLastBanner.RepeatCount    = 1;
            sLastBanner.BaseSummary    = InBaseSummary;
            sLastBanner.Item           = InItem;
        }

        auto Show_RecoveryToast(
            const TArray<FCk_RecoveryAction>& InApplied) -> void
        {
            if (NOT FSlateApplication::IsInitialized())
            { return; }

            const auto NumApplied = InApplied.Num();

            auto Subtext = FString{};
            for (const auto& Action : InApplied)
            {
                if (NOT Subtext.IsEmpty())
                { Subtext.Append(LINE_TERMINATOR); }
                Subtext += FString::Printf(TEXT("\x2022 %s"), *Describe_Action(Action));
            }

            const auto SummaryText = FText::Format(
                LOCTEXT("RecoveryToast", "AngelScript self-heal recovered {0} drift(s)"),
                FText::AsNumber(NumApplied));

            // Transition the in-progress toast in place so the user sees one
            // continuous notification rather than a flash of nothing.
            if (auto Item = sInProgressNotification.Pin(); Item.IsValid())
            {
                Item->SetText(SummaryText);
                Item->SetSubText(FText::FromString(Subtext));
                Item->SetCompletionState(SNotificationItem::CS_Success);
                Item->SetExpireDuration(12.0f);
                Item->ExpireAndFadeout();
                sInProgressNotification.Reset();
                return;
            }

            // Fallback: no in-progress toast (e.g. mid-session, or Slate wasn't
            // up at OnReloadHadErrors time). Coalesce identical bursts so a
            // save-storm doesn't stack N copies on screen.
            const auto SubtextAsText = FText::FromString(Subtext);
            constexpr auto kRecoveryExpire = 12.0f;
            if (TryCoalesce_LastBanner(ECk_BannerKind::Recovery, SummaryText, SubtextAsText, kRecoveryExpire))
            { return; }

            auto Info = FNotificationInfo{SummaryText};
            Info.ExpireDuration       = kRecoveryExpire;
            Info.bUseLargeFont        = false;
            Info.bUseThrobber         = false;
            Info.bUseSuccessFailIcons = true;
            Info.SubText              = SubtextAsText;
            Info.Hyperlink            = FSimpleDelegate::CreateLambda([]()
            {
                if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
                {
                    auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
                    MessageLogModule.OpenMessageLog(FName{sSelfHealLogChannel});
                }
            });
            Info.HyperlinkText = LOCTEXT("ViewLog", "View details");

            const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
            if (NotificationPtr.IsValid())
            {
                NotificationPtr->SetCompletionState(SNotificationItem::CS_Success);
            }
            Record_NewBanner(ECk_BannerKind::Recovery, SummaryText, SubtextAsText, NotificationPtr);
        }

        auto Show_TerminalToast(
            const FText& InMessage) -> void
        {
            if (NOT FSlateApplication::IsInitialized())
            { return; }

            if (auto Item = sInProgressNotification.Pin(); Item.IsValid())
            {
                Item->SetText(InMessage);
                Item->SetCompletionState(SNotificationItem::CS_Fail);
                Item->SetExpireDuration(20.0f);
                Item->ExpireAndFadeout();
                sInProgressNotification.Reset();
                return;
            }

            constexpr auto kTerminalExpire = 20.0f;
            if (TryCoalesce_LastBanner(ECk_BannerKind::Terminal, InMessage, FText::GetEmpty(), kTerminalExpire))
            { return; }

            auto Info = FNotificationInfo{InMessage};
            Info.ExpireDuration       = kTerminalExpire;
            Info.bUseLargeFont        = false;
            Info.bUseThrobber         = false;
            Info.bUseSuccessFailIcons = true;
            Info.Hyperlink            = FSimpleDelegate::CreateLambda([]()
            {
                if (FModuleManager::Get().IsModuleLoaded(TEXT("MessageLog")))
                {
                    auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
                    MessageLogModule.OpenMessageLog(FName{sSelfHealLogChannel});
                }
            });
            Info.HyperlinkText = LOCTEXT("ViewLog", "View details");

            const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
            if (NotificationPtr.IsValid())
            {
                NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
            }
            Record_NewBanner(ECk_BannerKind::Terminal, InMessage, FText::GetEmpty(), NotificationPtr);
        }

        // ---- Per-signature convergence gate ----------------------------------------
        //
        // Built from the same content the existing Describe_Action helper uses
        // for log/MessageLog entries, except the args list is NORMALIZED
        // (const/& stripped) so call-site argument-category variants of the
        // same logical signature (literal vs lvalue → "const int" vs "int&")
        // share one tracker instead of each getting an independent repeat
        // budget. Describe_Action stays raw on purpose — it echoes the error
        // verbatim for forensics.
        auto Build_SignatureKey(
            const FCk_RecoveryAction& InAction) -> FString
        {
            switch (InAction.Strategy)
            {
                case ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams:
                case ECk_RecoveryStrategy::KickGenerator_AssetRegistry:
                {
                    // Direct-construction ESP errors carry a struct name, not
                    // a namespace + signature — one convergence key per
                    // struct, NOT one shared "::()"-shaped key for all of
                    // them (which would trip the breaker after 3 distinct
                    // classes during bulk bootstrap).
                    if (const auto StructIdentifier = Get_EspStructErrorIdentifier(InAction.Error);
                        NOT StructIdentifier.IsEmpty())
                    {
                        return FString::Printf(TEXT("EspStruct::%s"), *StructIdentifier);
                    }

                    return FString::Printf(TEXT("%s::%s(%s)"),
                        *InAction.Error.TargetNamespace,
                        *InAction.Error.FunctionName,
                        *FCkAsStubSynthesizer::Normalize_ArgsList(InAction.Error.ArgsList));
                }
                case ECk_RecoveryStrategy::KickGenerator_DynamicHandle:
                {
                    return FString::Printf(TEXT("DynamicHandle::%s"),
                        *InAction.Error.MissingIdentifier);
                }
                case ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral:
                case ECk_RecoveryStrategy::Unrecognized:
                default:
                {
                    // Author-fixup and Unrecognized don't synthesize files —
                    // they can't form a synthesis loop, so we don't track them.
                    return FString{};
                }
            }
        }

        // Returns true when the action is cleared to proceed to Apply_Strategy.
        // Returns false when the key is blacklisted (already tripped) or trips
        // the breaker on this call — caller should skip Apply_Strategy and
        // move to the next queued action. The breaker emits the terminal
        // banner + toast exactly once per signature; subsequent skips are
        // silent (one log line for observability, no UI spam).
        auto Try_ReserveSynthesis(
            const FCk_RecoveryAction& InAction) -> bool
        {
            const auto Key = Build_SignatureKey(InAction);
            if (Key.IsEmpty())
            { return true; }

            if (sBlacklistedSignatures.Contains(Key))
            {
                Log(TEXT("[SelfHeal] Skipping convergence-blacklisted signature: {}"), Key);
                return false;
            }

            auto& Tracker = sPerSignatureRecoveryCount.FindOrAdd(Key);
            ++Tracker.RecoveryCount;

            const auto Callsite = InAction.Error.FilePath.IsEmpty()
                ? FString{TEXT("<unknown caller>")}
                : FString::Printf(TEXT("%s:%d:%d"),
                    *InAction.Error.FilePath, InAction.Error.Line, InAction.Error.Column);
            Tracker.Callsites.Add(Callsite);

            if (Tracker.RecoveryCount < FCkAsRecoveryDispatcher::MaxPerSignatureRepeats)
            { return true; }

            // Trip the breaker — refuse synthesis on this attempt, blacklist
            // the key for the rest of the session, emit banner + toast.
            sBlacklistedSignatures.Add(Key);
            Log_TerminalBanner_ConvergenceFailed(Key, Tracker.Callsites);
            Show_TerminalToast(FText::Format(
                LOCTEXT("ConvergenceFailedToast",
                    "AngelScript self-heal: convergence failed for {0} after {1} attempts. "
                    "Manual intervention required — see the Message Log."),
                FText::FromString(Key),
                FText::AsNumber(FCkAsRecoveryDispatcher::MaxPerSignatureRepeats)));
            return false;
        }

        // ---- Modal-tick handler (cold-start deferred apply) ------------------------

        auto OnModalLoopTick(
            float /*InDeltaTime*/) -> void
        {
            if (sModalTicksWaited < sModalTicksToWait)
            {
                ++sModalTicksWaited;
                return;
            }

            if (sPendingActions.Num() == 0)
            {
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

            auto AppliedActions = TArray<FCk_RecoveryAction>{};
            for (const auto& Action : sPendingActions)
            {
                if (NOT Try_ReserveSynthesis(Action))
                { continue; }
                if (Apply_Strategy(Action.Strategy, Action.Error))
                { AppliedActions.Add(Action); }
            }
            const auto QueuedCount = sPendingActions.Num();
            sPendingActions.Reset();

            if (AppliedActions.Num() > 0)
            {
                ++sCyclesRun;
                Log(TEXT("[SelfHeal] Cycle {} applied {} strategy/strategies. ")
                    TEXT("Hot-reload thread's next scan should pick up the file mtime change."),
                    sCyclesRun, AppliedActions.Num());

                Log_AppliedActions_ToMessageLog(AppliedActions);
                Show_RecoveryToast(AppliedActions);
            }
            else
            {
                Log_TerminalBanner_AllUnactionable(QueuedCount);
                Show_TerminalToast(LOCTEXT("RecoveryFailedToast",
                    "AngelScript self-heal could not act on the current compile errors. "
                    "Manual intervention required — see the Message Log for details."));
            }

            // Unsubscribe — next OnReloadHadErrors invocation will resubscribe.
            if (sModalTickHandle.IsValid() && FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(sModalTickHandle);
            }
            sModalTickHandle.Reset();
            sModalTicksWaited = 0;
        }

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

        // ---- Mid-session ticker handler (deferred apply, no modal) -----------------

        auto OnTicker_DrainActions(
            float /*InDeltaTime*/) -> bool
        {
            if (sPendingActions.Num() == 0)
            {
                sTickerHandle.Reset();
                return false;
            }

            Log(TEXT("[SelfHeal] Mid-session ticker firing — draining {} pending action(s)."),
                sPendingActions.Num());

            auto AppliedActions = TArray<FCk_RecoveryAction>{};
            for (const auto& Action : sPendingActions)
            {
                if (NOT Try_ReserveSynthesis(Action))
                { continue; }
                if (Apply_Strategy(Action.Strategy, Action.Error))
                { AppliedActions.Add(Action); }
            }
            const auto QueuedCount = sPendingActions.Num();
            sPendingActions.Reset();

            if (AppliedActions.Num() > 0)
            {
                ++sCyclesRun;
                Log(TEXT("[SelfHeal] Cycle {} applied {} strategy/strategies (mid-session)."),
                    sCyclesRun, AppliedActions.Num());

                // Mid-session success is intentionally silent on screen — no
                // Hazelight modal was up, recovery is invisible-and-fast, and
                // a green toast for a problem the user never saw is noise.
                // Cold-start still transitions the in-progress throbber to
                // success in place (separate code path).
                Log_AppliedActions_ToMessageLog(AppliedActions);
            }
            else
            {
                Log_TerminalBanner_AllUnactionable(QueuedCount);
                Show_TerminalToast(LOCTEXT("RecoveryFailedToast_MidSession",
                    "AngelScript self-heal could not act on the current compile errors. "
                    "Manual intervention required — see the Message Log for details."));
            }

            sTickerHandle.Reset();
            return false; // one-shot
        }

        auto Ensure_TickerSubscribed() -> void
        {
            if (sTickerHandle.IsValid())
            { return; }

            sTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateStatic(&OnTicker_DrainActions),
                sTickerDelaySeconds);
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

                // `F<X>_SpawnParams` used as a declared type while its
                // generated canonical is missing (gitignored ESP / fresh
                // clone) — ESP synthesis, preferring the source-derived
                // full shape.
                if (NOT FCkAsStubSynthesizer::Derive_ClassNameFromStructName(InError.MissingIdentifier).IsEmpty())
                { return ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams; }

                return ECk_RecoveryStrategy::Unrecognized;
            }

            case ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures:
            {
                // Direct construction of a generated `F<X>_SpawnParams`
                // (`auto P = FBb_X_SpawnParams(...)`) whose canonical is
                // missing. Any other bare-ctor miss is an authoring error —
                // terminal banner.
                if (NOT FCkAsStubSynthesizer::Derive_ClassNameFromStructName(InError.MissingIdentifier).IsEmpty())
                { return ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams; }

                return ECk_RecoveryStrategy::Unrecognized;
            }

            case ECk_AsParsedError_Kind::AdjacentStringLiteral:
            {
                return ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral;
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
        sInProgressNotification.Reset();
        sLastBanner = FLastBannerState{};
        sPerSignatureRecoveryCount.Reset();
        sBlacklistedSignatures.Reset();
        // sModalTickHandle left as-is; OnModalLoopTick self-cleans on empty queue.
    }

    auto FCkAsRecoveryDispatcher::Clear_PendingRecoveryState() -> void
    {
        if (sPendingActions.Num() > 0)
        {
            Log(TEXT("[SelfHeal] Compile succeeded with {} stale queued recovery action(s) — dropping them. ")
                TEXT("(A deferred drain firing after a clean compile would re-synthesize stubs from stale ")
                TEXT("error records and re-corrupt a healthy state.)"),
                sPendingActions.Num());
        }
        sPendingActions.Reset();
        sModalTicksWaited = 0;

        if (sTickerHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(sTickerHandle);
            sTickerHandle.Reset();
        }

        if (sModalTickHandle.IsValid() && FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(sModalTickHandle);
        }
        sModalTickHandle.Reset();

        // The in-progress toast is normally transitioned by the drain that
        // applied the recovery; if it's still pending here, the compile
        // succeeded without our intervention — fade it out.
        if (auto Item = sInProgressNotification.Pin(); Item.IsValid())
        {
            Item->SetCompletionState(SNotificationItem::CS_None);
            Item->ExpireAndFadeout();
        }
        sInProgressNotification.Reset();
    }

    auto FCkAsRecoveryDispatcher::Did_SynthesizeJsonStub_ThisSession() -> bool
    { return sDidSynthesizeJsonStub; }

    auto FCkAsRecoveryDispatcher::Mark_JsonStubSynthesized() -> void
    { sDidSynthesizeJsonStub = true; }

    auto FCkAsRecoveryDispatcher::Did_SynthesizeAssetRegistryStub_ThisSession() -> bool
    { return sDidSynthesizeAssetRegistryStub; }

    auto FCkAsRecoveryDispatcher::Mark_AssetRegistryStubSynthesized() -> void
    { sDidSynthesizeAssetRegistryStub = true; }

    auto FCkAsRecoveryDispatcher::Is_BootstrapMode() -> bool
    { return NOT sBootstrapComplete; }

    auto FCkAsRecoveryDispatcher::Mark_BootstrapComplete() -> void
    {
        sBootstrapComplete = true;
        // Reset cycle counter so bootstrap-consumed cycles don't count against
        // mid-session. (Cap is bootstrap-only anyway, but accurate counter
        // makes logs easier to read.) Same for the per-signature tracker —
        // bootstrap-mode counts shouldn't penalize mid-session attempts on
        // brand-new entity scripts authored after the editor reached main.
        sCyclesRun = 0;
        sPerSignatureRecoveryCount.Reset();
        sBlacklistedSignatures.Reset();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsRecoveryDispatcher::
        OnAngelscriptReloadHadErrors()
        -> void
    {
#if WITH_ANGELSCRIPT_CK
        const auto BootstrapMode = Is_BootstrapMode();

        // Cycle cap is bootstrap-only (see MaxCycles docstring). Mid-session
        // is interactive — user can intervene.
        if (BootstrapMode && sCyclesRun >= MaxCycles)
        {
            Log_TerminalBanner_MaxCyclesExceeded();
            Show_TerminalToast(FText::Format(
                LOCTEXT("CycleCapToast",
                    "AngelScript self-heal cycle cap ({0}) exceeded. Manual intervention required — "
                    "restart the editor after fixing the underlying AS issue."),
                FText::AsNumber(MaxCycles)));
            return;
        }

        const auto Diagnostics = FAngelscriptManager::Get().FormatDiagnostics();
        const auto Errors      = FCkAsErrorParser::ParseErrors(Diagnostics);
        const auto Roots       = FCkAsErrorParser::DeduplicateRoots(Errors);

        Log(TEXT("[SelfHeal] OnReloadHadErrors fired ({} mode, cycle {} of {}). ")
            TEXT("Parsed {} actionable roots from {} raw error records."),
            BootstrapMode ? TEXT("bootstrap") : TEXT("mid-session"),
            sCyclesRun + 1, MaxCycles, Roots.Num(), Errors.Num());

        if (Roots.Num() == 0)
        {
            Log_TerminalBanner_NoRoots(Diagnostics);
            return;
        }

        const auto Plan = BuildActionPlan(Roots);
        sPendingActions.Append(Plan);

        if (BootstrapMode)
        {
            Log(TEXT("[SelfHeal] Queued {} recovery action(s) for bootstrap modal-tick apply ")
                TEXT("(queue depth now {})."), Plan.Num(), sPendingActions.Num());

            // OnReloadHadErrors fires synchronously inside CompileModules —
            // i.e. before Hazelight's modal opens. The notification manager
            // queues the toast and renders it on the modal's tick, so the
            // user sees both at the same time.
            Show_InProgressToast();
            Ensure_ModalTickSubscribed();
        }
        else
        {
            Log(TEXT("[SelfHeal] Queued {} recovery action(s) for mid-session ticker apply ")
                TEXT("(queue depth now {})."), Plan.Num(), sPendingActions.Num());

            // Skip in-progress toast — see UI surfacing section header.
            Ensure_TickerSubscribed();
        }
#else
        Warning(TEXT("[SelfHeal] OnAngelscriptReloadHadErrors invoked without WITH_ANGELSCRIPT_CK — no-op."));
#endif
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
