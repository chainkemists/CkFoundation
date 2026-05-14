#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"

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
                    return FString::Printf(TEXT("%s::%s(%s)"),
                        *InAction.Error.TargetNamespace,
                        *InAction.Error.FunctionName,
                        *InAction.Error.ArgsList);
                }
                case ECk_RecoveryStrategy::KickGenerator_DynamicHandle:
                {
                    return InAction.Error.MissingIdentifier;
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
            // up at OnReloadHadErrors time).
            auto Info = FNotificationInfo{SummaryText};
            Info.ExpireDuration       = 12.0f;
            Info.bUseLargeFont        = false;
            Info.bUseThrobber         = false;
            Info.bUseSuccessFailIcons = true;
            Info.SubText              = FText::FromString(Subtext);
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

            auto Info = FNotificationInfo{InMessage};
            Info.ExpireDuration       = 20.0f;
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

                Log_AppliedActions_ToMessageLog(AppliedActions);
                Show_RecoveryToast(AppliedActions);
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
        sInProgressNotification.Reset();
        // sModalTickHandle left as-is; OnModalLoopTick self-cleans on empty queue.
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
        // makes logs easier to read.)
        sCyclesRun = 0;
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
