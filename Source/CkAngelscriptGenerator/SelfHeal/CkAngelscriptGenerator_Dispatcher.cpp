#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

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
    namespace ck_angelscript_generator_dispatcher
    {
        // OnReloadHadErrors broadcasts synchronously from CompileModules on the
        // main thread; all session state below is single-threaded.
        int32 sCyclesRun = 0;
        bool  sDidSynthesizeJsonStub = false;
        bool  sDidSynthesizeAssetRegistryStub = false;
        bool  sBootstrapComplete = false;

        // Breaker for "dueling-overloads" loops that re-synthesize the same stubs
        // at every cleanup boundary — the module CLAUDE.md § Per-signature
        // convergence cap owns the mechanism and its reset semantics.
        struct FConvergenceTracker
        {
            int32          RecoveryCount = 0;
            TSet<FString>  Callsites;  // "file:line:col" each
        };
        TMap<FString, FConvergenceTracker> sPerSignatureRecoveryCount;
        TSet<FString>                      sBlacklistedSignatures;

        // Canonicals quarantined this session (absolute path) — one shot each.
        // The guard is added on the ATTEMPT, not on success, so a FAILED
        // quarantine cannot loop either (mid-session has no MaxCycles cap).
        TSet<FString> sQuarantinedEspCanonicals;

        // Cold-start deferral: mutations applied inside OnReloadHadErrors land
        // BEFORE Hazelight's hot-reload thread baselines .as mtimes, so no later
        // scan ever sees them. The modal-tick pump fires after the thread is up.
        // Full rationale: CLAUDE.md § The modal-tick deferral.
        // sModalTicksToWait is a settling margin; one tick empirically suffices.
        TArray<FCk_RecoveryAction> sPendingActions;
        FDelegateHandle            sModalTickHandle;
        int32                      sModalTicksWaited  = 0;
        constexpr int32            sModalTicksToWait  = 2;

        // Mid-session hot-reload failures do NOT open a Hazelight modal, so
        // modal-tick never fires; FTSTicker covers that case. No settle margin
        // needed — the AS hot-reload thread is already running.
        FTSTicker::FDelegateHandle sTickerHandle;
        constexpr float            sTickerDelaySeconds = 0.15f;

        // Channel name shared with the Module's RegisterLogListing call.
        constexpr auto* sSelfHealLogChannel = TEXT("CkAngelscriptGenerator");

        // Held across the OnReloadHadErrors → modal-tick-apply lifetime so the
        // apply path transitions this toast in place instead of spawning a new
        // one. The notification's 30s ExpireDuration is the orphan safety net.
        TWeakPtr<SNotificationItem> sInProgressNotification;

        // Coalescing for the fallback branches of Show_RecoveryToast /
        // Show_TerminalToast (the in-progress→final transition is already
        // singleton'd above): identical-content bursts within
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

        // These are the ONLY generated files the dispatcher may delete — the one
        // sanctioned canonical mutation. TRACKED generated files never carry this
        // suffix, so they can never be quarantined. Deliberately IO-free, so
        // classification stays deterministic and unit-testable.
        auto Is_EntitySpawnParamsCanonicalPath(
            const FString& InFilePath) -> bool
        {
            if (NOT InFilePath.EndsWith(TEXT("_EntitySpawnParams.as")))
            { return false; }

            // A sibling stub satisfies the suffix too, and quarantining one would
            // derive a doubly-prefixed sibling from our own output. Stubs are
            // rebuilt per session and swept at startup, so they are never the
            // stale state this recovery exists to clear.
            if (FPaths::GetCleanFilename(InFilePath).StartsWith(TEXT("_StubRecovery_")))
            { return false; }

            auto Normalized = InFilePath;
            FPaths::NormalizeFilename(Normalized);
            return Normalized.Contains(TEXT("/Script/Generated/"));
        }

        // ---- DynamicHandle strategy ------------------------------------------------

        // Chicken-and-egg: the handle's data asset can't materialize until AS
        // compiles, and AS won't compile until the JSON has the entry. TypeName +
        // ShortName from the error text alone are enough to break it.

        auto Derive_HandleShortName(
            const FString& InMissingIdentifier) -> FString
        {
            const auto Prefix = FString{TEXT("FCk_Handle_")};
            if (InMissingIdentifier.StartsWith(Prefix))
            { return InMissingIdentifier.RightChop(Prefix.Len()); }
            return InMissingIdentifier;
        }

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

            // SIBLING stub file — the canonical JSON is never touched.
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

            // The stub's empty RequiredFragments registers a PERMISSIVE validator:
            // As_<ShortName>() casts succeed unchecked until OnPostEngineInit
            // upgrades it to strict, a sub-second window after main screen.
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

        // The direct-construction error shapes carry the struct name in
        // MissingIdentifier instead of a namespace + signature; the classic
        // `U<X>::Params(<args>)` overload miss does not. Empty means the latter.
        auto Get_EspStructErrorIdentifier(
            const FCk_AsParsedError& InError) -> FString
        {
            const auto IsStructShapedError =
                   InError.Kind == ECk_AsParsedError_Kind::IdentifierNotADataType
                || InError.Kind == ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures;

            return IsStructShapedError ? InError.MissingIdentifier : FString{};
        }

        // For a stale canonical referencing a deleted type, the error's FilePath
        // IS the canonical. Passing NO seed classes makes the rebuild enumerate
        // the union from the canonical's own blocks, so every shape it covered is
        // rebuilt in ONE pass rather than one cycle per orphaned class.
        auto Apply_QuarantineStaleEspCanonical(
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InScanRoots,
            FCk_AsSourceScanCache*   InScanCache) -> bool
        {
            const auto& CanonicalPath = InError.FilePath;

            // Defense in depth — Classify already gated on this; re-checked so a
            // future caller can't route a TRACKED generated file into a delete.
            if (NOT Is_EntitySpawnParamsCanonicalPath(CanonicalPath))
            {
                Warning(TEXT("[SelfHeal] Refusing quarantine — '{}' is not an EntitySpawnParams canonical."),
                    CanonicalPath);
                return false;
            }

            if (sQuarantinedEspCanonicals.Contains(CanonicalPath))
            {
                Log(TEXT("[SelfHeal] Canonical '{}' already quarantined this session — not retrying."),
                    CanonicalPath);
                return false;
            }
            sQuarantinedEspCanonicals.Add(CanonicalPath);

            Log(TEXT("[SelfHeal] Stale canonical '{}' references a deleted symbol ('{}' @ {}:{}) — ")
                TEXT("quarantining + rebuilding full shapes from source."),
                CanonicalPath, InError.MissingIdentifier, InError.Line, InError.Column);

            const auto Quarantined = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
                CanonicalPath, /*InSeedClassNames=*/{}, InError, InScanRoots, InScanCache);

            if (Quarantined.Success)
            {
                Log(TEXT("[SelfHeal] Quarantined stale canonical '{}' and rebuilt full-shape stubs -> {}. {}"),
                    CanonicalPath, Quarantined.TargetFilePath, Quarantined.ErrorMessage);
                return true;
            }

            Warning(TEXT("[SelfHeal] Quarantine of stale canonical '{}' FAILED: {}"),
                CanonicalPath, Quarantined.ErrorMessage);
            return false;
        }

        // InCandidates / InScanRoots / InScanCache are hoisted to ONCE PER DRAIN
        // by the drain handlers — they are invariant across a drain. Non-ESP
        // strategies ignore them.
        auto Apply_Strategy(
            ECk_RecoveryStrategy     InStrategy,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidates,
            const TArray<FString>&   InScanRoots,
            FCk_AsSourceScanCache*   InScanCache) -> bool
        {
            switch (InStrategy)
            {
                case ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams:
                {
                    const auto& Candidates = InCandidates;

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

                    // Source-derived full shapes are preferred because they heal
                    // direct-construction and field-access callers (`P.Phase = ...`)
                    // too — the wholesale-missing case of a fresh clone.
                    const auto SourceDerived = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
                        ClassName, InError, Candidates, InScanRoots, InScanCache);

                    if (SourceDerived.Success)
                    {
                        Log(TEXT("[SelfHeal] Source-derived full-shape stub for {} -> {}"),
                            ClassName, SourceDerived.TargetFilePath);
                        return true;
                    }

                    // The struct exists in a PRESENT canonical whose signatures
                    // drifted from source. Per-signature error-text stubs cannot
                    // heal mixed-type callers against it — only a quarantine +
                    // exact-typed bulk resynthesis can.
                    if (SourceDerived.FailReason == ECk_StubInjectFailReason::StructExistsInCanonical)
                    {
                        Log(TEXT("[SelfHeal] Stale canonical detected for {} ('{}') — escalating to quarantine + full-shape resynthesis."),
                            ClassName, SourceDerived.CanonicalFilePath);

                        const auto Quarantined = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
                            SourceDerived.CanonicalFilePath, {ClassName}, InError,
                            InScanRoots, InScanCache);

                        if (Quarantined.Success)
                        {
                            Log(TEXT("[SelfHeal] Quarantined stale canonical '{}' and rebuilt full-shape stubs -> {}. {}"),
                                SourceDerived.CanonicalFilePath, Quarantined.TargetFilePath, Quarantined.ErrorMessage);
                            return true;
                        }

                        Warning(TEXT("[SelfHeal] Quarantine escalation FAILED for {}: {}"),
                            ClassName, Quarantined.ErrorMessage);
                        return false;
                    }

                    Log(TEXT("[SelfHeal] Source-derived synthesis unavailable for {} ({}) — falling back to error-text stub."),
                        ClassName, SourceDerived.ErrorMessage);

                    // Error-text fallback. Direct-construction errors carry no
                    // namespace signature, so the no-arg shape is all that can be
                    // synthesized; field-access callers then hit the convergence
                    // banner, which is correct — they are genuinely unrecoverable.
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

                    // An existing same-arity stub that does NOT satisfy this
                    // caller (mixed static types across call sites) is unhealable
                    // per-signature — escalate to exact-typed full shapes.
                    if (Result.FailReason == ECk_StubInjectFailReason::SameArityAmbiguous)
                    {
                        Log(TEXT("[SelfHeal] Same-arity wedge for {}::{} — escalating to quarantine + full-shape resynthesis."),
                            FallbackError.TargetNamespace, FallbackError.FunctionName);

                        const auto Quarantined = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
                            Result.CanonicalFilePath, {ClassName}, FallbackError,
                            InScanRoots, InScanCache);

                        if (Quarantined.Success)
                        {
                            Log(TEXT("[SelfHeal] Rebuilt full-shape stubs after same-arity wedge -> {}. {}"),
                                Quarantined.TargetFilePath, Quarantined.ErrorMessage);
                            return true;
                        }

                        Warning(TEXT("[SelfHeal] Quarantine escalation FAILED after same-arity wedge for {}::{}: {}"),
                            FallbackError.TargetNamespace, FallbackError.FunctionName, Quarantined.ErrorMessage);
                        return false;
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
                    // An unresolvable class is a deliberate refusal, not a bug:
                    // logging and bailing leaves Hazelight's original actionable
                    // error on screen (see the synthesizer's Tier3_IsAllowed).
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

                case ECk_RecoveryStrategy::Quarantine_StaleEspCanonical:
                {
                    return Apply_QuarantineStaleEspCanonical(InError, InScanRoots, InScanCache);
                }

                case ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral:
                {
                    // No auto-fix — modifying user source is out of contract.
                    // Returns TRUE anyway: diagnosing IS the recognized action, and
                    // the outer flow must not fall through to "all actions failed".
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

            // Duplicated into MessageLog because that is where the toast's
            // "View details" hyperlink lands — the plain log file is unreachable
            // from the toast.
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
        // ONE notification, transitioned in place: in-progress (cold start only) →
        // success on apply, or fail on a terminal banner. Deliberately skipped
        // mid-session — recovery is sub-200ms with no modal, so it would be noise.

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
                case ECk_RecoveryStrategy::Quarantine_StaleEspCanonical:
                {
                    const auto DeadSymbol = InAction.Error.Kind == ECk_AsParsedError_Kind::NotAMemberOfStruct
                        ? FString::Printf(TEXT("dead field '%s.%s'"),
                            *InAction.Error.LookupScope, *InAction.Error.MissingIdentifier)
                        : FString::Printf(TEXT("dead type '%s'"), *InAction.Error.MissingIdentifier);

                    return FString::Printf(TEXT("quarantine stale canonical %s (%s)"),
                        *InAction.Error.FilePath, *DeadSymbol);
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

            // Orphan guard: no transition within 5s means the failure path that
            // fired OnReloadHadErrors opened no Hazelight modal, so modal-tick
            // will never run. The normal path transitions in ~1.2s.
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

        // True when the incoming banner was folded into the last one in place —
        // callers MUST then skip their AddNotification path.
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

        // Called after a fresh fallback toast was spawned: emits a postmortem for
        // the outgoing banner if it had accumulated repeats, then overwrites state.
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

            // In place, so the user sees one continuous notification.
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

            // No in-progress toast (mid-session, or Slate was down earlier) —
            // coalesce so a save-storm doesn't stack N copies on screen.
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
        // Mirrors Describe_Action except the args list is NORMALIZED, so
        // argument-category variants of one logical signature ("const int" vs
        // "int&") share a repeat budget. Describe_Action stays raw for forensics.
        auto Build_SignatureKey(
            const FCk_RecoveryAction& InAction) -> FString
        {
            switch (InAction.Strategy)
            {
                case ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams:
                case ECk_RecoveryStrategy::KickGenerator_AssetRegistry:
                {
                    // One key PER STRUCT — a shared "::()"-shaped key would trip
                    // the breaker after 3 distinct classes during bulk bootstrap.
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
                case ECk_RecoveryStrategy::Quarantine_StaleEspCanonical:
                {
                    // Untracked on purpose: a cap of 3 would permit three deletes
                    // of one canonical. sQuarantinedEspCanonicals gates it instead.
                    return FString{};
                }
                case ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral:
                case ECk_RecoveryStrategy::Unrecognized:
                default:
                {
                    // These synthesize no files, so they can't form a loop.
                    return FString{};
                }
            }
        }

        // False when the key is blacklisted or trips the breaker on this call —
        // the caller must then skip Apply_Strategy. The banner + toast fire
        // exactly once per signature; later skips are log-only.
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

        // Single-writer gate (G9). A SECONDARY instance must not drain: it drops its
        // queued actions and relies on the OWNER's heals arriving through its own
        // hot-reload watcher. True means skip the drain.
        auto Should_SuppressDrain_AsSecondary(FStringView InGateSite) -> bool
        {
            if (FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(InGateSite))
            { return false; }

            const auto QueuedCount = sPendingActions.Num();
            sPendingActions.Reset();
            Log(TEXT("[SelfHeal] {} recovery action(s) suppressed — SECONDARY instance (another ")
                TEXT("editor owns Script/Generated regen). The owning instance's self-heal writes ")
                TEXT("reach this process via its hot-reload watcher; this instance takes over if the ")
                TEXT("owner exits."), QueuedCount);
            return true;
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

            if (Should_SuppressDrain_AsSecondary(TEXT("Dispatcher.OnModalLoopTick")))
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

            // DrainScanCache is drain-LOCAL by contract — never hoist it out.
            const auto DrainScanRoots  = FCkAsSourceScanner::Get_DefaultScanRoots();
            const auto DrainCandidates = Collect_EntitySpawnParamsCandidates();
            auto       DrainScanCache  = FCk_AsSourceScanCache{};

            auto AppliedActions = TArray<FCk_RecoveryAction>{};
            for (const auto& Action : sPendingActions)
            {
                if (NOT Try_ReserveSynthesis(Action))
                { continue; }
                if (Apply_Strategy(Action.Strategy, Action.Error, DrainCandidates, DrainScanRoots, &DrainScanCache))
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

        // Headless (commandlet / unattended) bootstrap drain. No Slate modal
        // pumps here and Hazelight hard-exits right after the broadcast returns,
        // so this synchronous apply is the only window we get — and it is SAFE,
        // because there is no hot-reload watcher whose mtime baseline could
        // swallow the writes. The stubs land before the exit; the next run of
        // the same commandlet compiles against them.
        auto Drain_PendingActions_Headless() -> void
        {
            if (Should_SuppressDrain_AsSecondary(TEXT("Dispatcher.Drain_PendingActions_Headless")))
            { return; }

            Log(TEXT("[SelfHeal] Headless bootstrap — applying {} recovery action(s) synchronously ")
                TEXT("(no modal pump in commandlet/unattended mode)."),
                sPendingActions.Num());

            // DrainScanCache is drain-LOCAL by contract — never hoist it out.
            const auto DrainScanRoots  = FCkAsSourceScanner::Get_DefaultScanRoots();
            const auto DrainCandidates = Collect_EntitySpawnParamsCandidates();
            auto       DrainScanCache  = FCk_AsSourceScanCache{};

            auto AppliedActions = TArray<FCk_RecoveryAction>{};
            for (const auto& Action : sPendingActions)
            {
                if (NOT Try_ReserveSynthesis(Action))
                { continue; }
                if (Apply_Strategy(Action.Strategy, Action.Error, DrainCandidates, DrainScanRoots, &DrainScanCache))
                { AppliedActions.Add(Action); }
            }
            const auto QueuedCount = sPendingActions.Num();
            sPendingActions.Reset();

            if (AppliedActions.Num() > 0)
            {
                ++sCyclesRun;
                Log(TEXT("[SelfHeal] Cycle {} applied {} strategy/strategies synchronously. ")
                    TEXT("If this process exits with 'Cannot run when angelscript has failed to ")
                    TEXT("compile', RE-RUN THE SAME COMMAND — the next run compiles against the ")
                    TEXT("synthesized recovery stubs and regenerates the canonical files."),
                    sCyclesRun, AppliedActions.Num());

                Log_AppliedActions_ToMessageLog(AppliedActions);
            }
            else
            {
                Log_TerminalBanner_AllUnactionable(QueuedCount);
            }
        }

        auto Ensure_ModalTickSubscribed() -> void
        {
            if (sModalTickHandle.IsValid())
            { return; }

            if (NOT FSlateApplication::IsInitialized())
            {
                Drain_PendingActions_Headless();
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

            if (Should_SuppressDrain_AsSecondary(TEXT("Dispatcher.OnTicker_DrainActions")))
            {
                sTickerHandle.Reset();
                return false;
            }

            Log(TEXT("[SelfHeal] Mid-session ticker firing — draining {} pending action(s)."),
                sPendingActions.Num());

            // DrainScanCache is drain-LOCAL by contract — never hoist it out.
            const auto DrainScanRoots  = FCkAsSourceScanner::Get_DefaultScanRoots();
            const auto DrainCandidates = Collect_EntitySpawnParamsCandidates();
            auto       DrainScanCache  = FCk_AsSourceScanCache{};

            auto AppliedActions = TArray<FCk_RecoveryAction>{};
            for (const auto& Action : sPendingActions)
            {
                if (NOT Try_ReserveSynthesis(Action))
                { continue; }
                if (Apply_Strategy(Action.Strategy, Action.Error, DrainCandidates, DrainScanRoots, &DrainScanCache))
                { AppliedActions.Add(Action); }
            }
            const auto QueuedCount = sPendingActions.Num();
            sPendingActions.Reset();

            if (AppliedActions.Num() > 0)
            {
                ++sCyclesRun;
                Log(TEXT("[SelfHeal] Cycle {} applied {} strategy/strategies (mid-session)."),
                    sCyclesRun, AppliedActions.Num());

                // Silent on screen by design: a green toast for a problem the user
                // never saw is noise. Cold start still transitions its throbber.
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

                // `F<X>_SpawnParams` used as a declared type while its generated
                // canonical is missing (gitignored ESP / fresh clone).
                if (NOT FCkAsStubSynthesizer::Derive_ClassNameFromStructName(InError.MissingIdentifier).IsEmpty())
                { return ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams; }

                // An ordinary type that no longer exists. Located INSIDE a
                // generated canonical, that means the canonical is stale — rebuild
                // it. Keyed on LOCATION, so the identical error in author source
                // stays Unrecognized: a real authoring bug must not be papered over.
                if (ck_angelscript_generator_dispatcher::Is_EntitySpawnParamsCanonicalPath(InError.FilePath))
                { return ECk_RecoveryStrategy::Quarantine_StaleEspCanonical; }

                return ECk_RecoveryStrategy::Unrecognized;
            }

            case ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures:
            {
                // Direct construction of a generated `F<X>_SpawnParams` whose
                // canonical is missing. Any OTHER bare-ctor miss is an authoring
                // error and gets the terminal banner.
                if (NOT FCkAsStubSynthesizer::Derive_ClassNameFromStructName(InError.MissingIdentifier).IsEmpty())
                { return ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams; }

                return ECk_RecoveryStrategy::Unrecognized;
            }

            case ECk_AsParsedError_Kind::NotAMemberOfStruct:
            {
                // A FIELD that no longer exists — the deleted-field twin of the
                // deleted-type case above, and keyed the same way. Located INSIDE
                // a generated canonical it means the canonical is stale, so rebuild
                // it; the identical error in author source stays Unrecognized,
                // because a real authoring bug must not be papered over.
                if (ck_angelscript_generator_dispatcher::Is_EntitySpawnParamsCanonicalPath(InError.FilePath))
                { return ECk_RecoveryStrategy::Quarantine_StaleEspCanonical; }

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

    auto FCkAsRecoveryDispatcher::Get_CyclesRun() -> int32 { return ck_angelscript_generator_dispatcher::sCyclesRun; }

    auto FCkAsRecoveryDispatcher::Reset_CyclesRun() -> void
    {
        ck_angelscript_generator_dispatcher::sCyclesRun = 0;
        ck_angelscript_generator_dispatcher::sPendingActions.Reset();
        ck_angelscript_generator_dispatcher::sModalTicksWaited = 0;
        ck_angelscript_generator_dispatcher::sDidSynthesizeJsonStub = false;
        ck_angelscript_generator_dispatcher::sDidSynthesizeAssetRegistryStub = false;
        ck_angelscript_generator_dispatcher::sInProgressNotification.Reset();
        ck_angelscript_generator_dispatcher::sLastBanner = ck_angelscript_generator_dispatcher::FLastBannerState{};
        ck_angelscript_generator_dispatcher::sPerSignatureRecoveryCount.Reset();
        ck_angelscript_generator_dispatcher::sBlacklistedSignatures.Reset();
        ck_angelscript_generator_dispatcher::sQuarantinedEspCanonicals.Reset();
        // sModalTickHandle left as-is; OnModalLoopTick self-cleans on empty queue.
    }

    auto FCkAsRecoveryDispatcher::Clear_PendingRecoveryState() -> void
    {
        if (ck_angelscript_generator_dispatcher::sPendingActions.Num() > 0)
        {
            Log(TEXT("[SelfHeal] Compile succeeded with {} stale queued recovery action(s) — dropping them. ")
                TEXT("(A deferred drain firing after a clean compile would re-synthesize stubs from stale ")
                TEXT("error records and re-corrupt a healthy state.)"),
                ck_angelscript_generator_dispatcher::sPendingActions.Num());
        }
        ck_angelscript_generator_dispatcher::sPendingActions.Reset();
        ck_angelscript_generator_dispatcher::sModalTicksWaited = 0;

        if (ck_angelscript_generator_dispatcher::sTickerHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(ck_angelscript_generator_dispatcher::sTickerHandle);
            ck_angelscript_generator_dispatcher::sTickerHandle.Reset();
        }

        if (ck_angelscript_generator_dispatcher::sModalTickHandle.IsValid() && FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(ck_angelscript_generator_dispatcher::sModalTickHandle);
        }
        ck_angelscript_generator_dispatcher::sModalTickHandle.Reset();

        // Still pending here means the compile succeeded without our
        // intervention (the applying drain normally transitions it) — fade it out.
        if (auto Item = ck_angelscript_generator_dispatcher::sInProgressNotification.Pin(); Item.IsValid())
        {
            Item->SetCompletionState(SNotificationItem::CS_None);
            Item->ExpireAndFadeout();
        }
        ck_angelscript_generator_dispatcher::sInProgressNotification.Reset();
    }

    auto FCkAsRecoveryDispatcher::Did_SynthesizeJsonStub_ThisSession() -> bool
    { return ck_angelscript_generator_dispatcher::sDidSynthesizeJsonStub; }

    auto FCkAsRecoveryDispatcher::Mark_JsonStubSynthesized() -> void
    { ck_angelscript_generator_dispatcher::sDidSynthesizeJsonStub = true; }

    auto FCkAsRecoveryDispatcher::Did_SynthesizeAssetRegistryStub_ThisSession() -> bool
    { return ck_angelscript_generator_dispatcher::sDidSynthesizeAssetRegistryStub; }

    auto FCkAsRecoveryDispatcher::Mark_AssetRegistryStubSynthesized() -> void
    { ck_angelscript_generator_dispatcher::sDidSynthesizeAssetRegistryStub = true; }

    auto FCkAsRecoveryDispatcher::Is_BootstrapMode() -> bool
    { return NOT ck_angelscript_generator_dispatcher::sBootstrapComplete; }

    auto FCkAsRecoveryDispatcher::Mark_BootstrapComplete() -> void
    {
        ck_angelscript_generator_dispatcher::sBootstrapComplete = true;
        // Bootstrap-mode counts must not penalize mid-session attempts on entity
        // scripts authored after the editor reached main screen.
        ck_angelscript_generator_dispatcher::sCyclesRun = 0;
        ck_angelscript_generator_dispatcher::sPerSignatureRecoveryCount.Reset();
        ck_angelscript_generator_dispatcher::sBlacklistedSignatures.Reset();
        ck_angelscript_generator_dispatcher::sQuarantinedEspCanonicals.Reset();
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
        if (BootstrapMode && ck_angelscript_generator_dispatcher::sCyclesRun >= MaxCycles)
        {
            ck_angelscript_generator_dispatcher::Log_TerminalBanner_MaxCyclesExceeded();
            ck_angelscript_generator_dispatcher::Show_TerminalToast(FText::Format(
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
            ck_angelscript_generator_dispatcher::sCyclesRun + 1, MaxCycles, Roots.Num(), Errors.Num());

        if (Roots.Num() == 0)
        {
            ck_angelscript_generator_dispatcher::Log_TerminalBanner_NoRoots(Diagnostics);
            return;
        }

        const auto Plan = BuildActionPlan(Roots);
        ck_angelscript_generator_dispatcher::sPendingActions.Append(Plan);

        if (BootstrapMode)
        {
            Log(TEXT("[SelfHeal] Queued {} recovery action(s) for bootstrap modal-tick apply ")
                TEXT("(queue depth now {})."), Plan.Num(), ck_angelscript_generator_dispatcher::sPendingActions.Num());

            // This runs BEFORE Hazelight's modal opens; the notification manager
            // queues the toast and renders it on the modal's tick, so the user
            // sees both at once.
            ck_angelscript_generator_dispatcher::Show_InProgressToast();
            ck_angelscript_generator_dispatcher::Ensure_ModalTickSubscribed();
        }
        else
        {
            Log(TEXT("[SelfHeal] Queued {} recovery action(s) for mid-session ticker apply ")
                TEXT("(queue depth now {})."), Plan.Num(), ck_angelscript_generator_dispatcher::sPendingActions.Num());

            // Skip in-progress toast — see UI surfacing section header.
            ck_angelscript_generator_dispatcher::Ensure_TickerSubscribed();
        }
#else
        Warning(TEXT("[SelfHeal] OnAngelscriptReloadHadErrors invoked without WITH_ANGELSCRIPT_CK — no-op."));
#endif
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
