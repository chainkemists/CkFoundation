#include "CkEnsure.h"

#include "CkCore/Ensure/CkEnsure_Log.h"
#include "CkCore/AngelScript/CkAngelscriptDebugger.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/MessageDialog/CkMessageDialog_Utils.h"
#include "CkCore/Settings/CkCore_Settings.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"

#include <CoreMinimal.h>
#include <Windows/WindowsPlatformApplicationMisc.h> // required for clipboard copy

#if WITH_ANGELSCRIPT_CK
#include <as_context.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
namespace ck::ensure
{
    static thread_local int32 EnsureFromScriptDepth = 0;

    auto Request_GetCurrentScriptSiteIdentity() -> FString
    {
    #if WITH_ANGELSCRIPT_CK
        if (FAngelscriptManager::IsInitialized())
        {
            if (auto* Context = FAngelscriptManager::GetCurrentScriptContext(); Context != nullptr)
            {
                if (auto* Function = Context->GetFunction(0); Function != nullptr)
                {
                    const auto FunctionName = FString{StringCast<TCHAR>(Function->GetName()).Get()};
                    const auto ObjectTypeName = Function->GetObjectType() != nullptr
                        ? FString{StringCast<TCHAR>(Function->GetObjectType()->GetName()).Get()}
                        : FString{TEXT("Global")};

                    auto Column = 0;
                    const char* Section = nullptr;
                    const auto Line = Context->GetLineNumber(0, &Column, &Section);
                    const auto SectionName = Section != nullptr
                        ? FString{StringCast<TCHAR>(Section).Get()}
                        : FString{TEXT("UnknownSection")};

                    return ck::Format_UE(
                        TEXT("AS:{}::{}@{}:{}:{}"),
                        ObjectTypeName,
                        FunctionName,
                        SectionName,
                        Line,
                        Column);
                }
            }
        }
    #endif

    #if !CK_DISABLE_STACK_TRACE
        // Blueprint VM frames and their UObject nodes are game-thread state. AngelScript's active context above is
        // thread-local and can still provide a stable worker-script function/line without touching UObjects.
        if (IsInGameThread())
        {
            if (const auto* BlueprintContextTracker = FBlueprintContextTracker::TryGet(); BlueprintContextTracker != nullptr)
            {
                const auto& ScriptStack = BlueprintContextTracker->GetCurrentScriptStack();
                if (NOT ScriptStack.IsEmpty())
                {
                    const auto* const Frame = ScriptStack.Last();
                    if (Frame != nullptr && Frame->Node != nullptr)
                    {
                        auto BytecodeOffset = static_cast<int64>(-1);
                        const auto* const ScriptStart = Frame->Node->Script.GetData();
                        const auto ScriptSize = Frame->Node->Script.Num();
                        const auto CodeAddress = reinterpret_cast<UPTRINT>(Frame->Code);
                        const auto ScriptStartAddress = reinterpret_cast<UPTRINT>(ScriptStart);
                        const auto ScriptEndAddress = ScriptStartAddress + ScriptSize;
                        if (Frame->Code != nullptr
                            && ScriptStart != nullptr
                            && CodeAddress >= ScriptStartAddress
                            && CodeAddress <= ScriptEndAddress)
                        {
                            BytecodeOffset = static_cast<int64>(CodeAddress - ScriptStartAddress);
                        }

                        return ck::Format_UE(
                            TEXT("BP:{}@Bytecode[{}]"),
                            GetPathNameSafe(Frame->Node),
                            BytecodeOffset);
                    }
                }
            }
        }
    #endif

        return FString{TEXT("Script::Unknown")};
    }

    auto Request_ReportFirstOccurrenceFromWorkerThread(
        const FString& InMessage,
        const FString& InExpressionText,
        TFunctionRef<void(const FString&)> InReportEmitter) -> void
    {
        constexpr auto FramesToSkip_GetStackTrace_EnsureImpl_HandleFail = 3;
        const auto& StackTrace = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(
            FramesToSkip_GetStackTrace_EnsureImpl_HandleFail);

        InReportEmitter(ck::Format_UE(
            TEXT("[Worker] {}\n{}\n\n == CallStack ==\n{}"),
            InExpressionText,
            InMessage,
            StackTrace));
    }

    auto Request_TrimEngineBoilerplateFrames(const FString& InStackTrace) -> FString
    {
        if (InStackTrace.IsEmpty())
        { return InStackTrace; }

        static const auto TailPatterns = TArray<FString>
        {
            TEXT("TGraphTask<"),
            TEXT("UE::Tasks::Private::FTaskBase"),
            TEXT("FNamedTaskThread::"),
            TEXT("FTaskGraphCompatibilityImplementation::"),
            TEXT("FTickTaskSequencer::"),
            TEXT("FTickTaskManager::"),
            TEXT("UWorld::Tick"),
            TEXT("UEditorEngine::Tick"),
            TEXT("UUnrealEdEngine::Tick"),
            TEXT("FEngineLoop::Tick"),
            TEXT("GuardedMain"),
            TEXT("LaunchWindowsStartup"),
            TEXT("WinMain"),
            TEXT("__scrt_common_main_seh"),
            TEXT("UnknownFunction"),
        };

        auto Lines = TArray<FString>{};
        InStackTrace.ParseIntoArrayLines(Lines, false);

        for (auto i = 0; i < Lines.Num(); ++i)
        {
            for (const auto& Pattern : TailPatterns)
            {
                if (Lines[i].Contains(Pattern))
                {
                    Lines.SetNum(i);
                    return FString::Join(Lines, TEXT("\n"));
                }
            }
        }

        return InStackTrace;
    }

    auto Request_WrapMultilineTextWithRichTextTags(const FString& InText, const FString& InTagName) -> FString
    {
        if (InText.IsEmpty())
        {
            return ck::Format_UE(TEXT("<{}>(empty)</>"), InTagName);
        }

        auto Lines = TArray<FString>{};
        InText.ParseIntoArrayLines(Lines);

        auto Result = FString{};
        for (int32 i = 0; i < Lines.Num(); ++i)
        {
            if (i > 0)
            {
                Result += TEXT("\n");
            }

            auto SanitizedLine = Lines[i];
            SanitizedLine = SanitizedLine.Replace(TEXT("`"), TEXT("'"));
            SanitizedLine = SanitizedLine.Replace(TEXT("<"), TEXT("&lt;"));
            SanitizedLine = SanitizedLine.Replace(TEXT(">"), TEXT("&gt;"));

            Result += ck::Format_UE(TEXT("<{}>{}</>"), InTagName, SanitizedLine);
        }

        return Result;
    }

    auto
        Ensure_Impl_Internal(
            const FString& InMessage,
            const FString& InExpressionText,
            const FName& InFile,
            int32 InLine,
            bool& OutBreakInCode,
            bool& OutBreakInScript,
            TFunctionRef<void(const FString&)> InWorkerReportEmitter,
            TFunctionRef<void(const FCk_Utils_EditorOnly_PushNewEditorMessage_Params&)> InEditorMessageEmitter)
        -> void
    {
        OutBreakInCode = false;
        OutBreakInScript = false;

        if (ck::Is_AngelscriptDebugger_Paused())
        { return; }

        const auto IsEnsureFromScript = Get_IsEnsureFromScript();
        const auto& Record = UCk_Utils_Ensure_UE::Request_RecordEnsureOccurrence(
            FCk_EnsureSignature
            {
                InFile,
                InLine,
                InExpressionText,
                IsEnsureFromScript ? Request_GetCurrentScriptSiteIdentity() : FString{},
            });

        if (NOT Record.IsFirstOccurrence)
        { return; }

        if (NOT IsInGameThread())
        {
            Request_ReportFirstOccurrenceFromWorkerThread(
                InMessage,
                InExpressionText,
                InWorkerReportEmitter);
            return;
        }

        if (NOT IsEnsureFromScript && UCk_Utils_Ensure_UE::Get_IsEnsureIgnored(InFile, InLine))
        { return; }

        const auto IsMessageOnly = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;

        const auto& Title = ck::Format_UE(TEXT("Frame#[{}] PIE-ID[{}]"), GFrameCounter, UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld());
        constexpr auto FramesToSkip_GetStackTrace_EnsureImpl_HandleFail = 3;
        const auto& StackTraceFromEnsureCaller = IsMessageOnly ?
            TEXT("[StackTrace DISABLED]") :
            UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(FramesToSkip_GetStackTrace_EnsureImpl_HandleFail);
        const auto& BpStackTrace = IsMessageOnly ?
            TEXT("[BP StackTrace DISABLED]") :
            UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{});
        const auto& AsStackTrace = IsMessageOnly ?
            TEXT("[AS StackTrace DISABLED]") :
            UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(ck::type_traits::AsString{});

        // UE::GetPlayInEditorID() checkf's (FATAL) on the -2 sentinel that non-game-thread PIE-ID slots
        // hold, and the non-asserting accessor is not CORE_API-exported — so query it only when provably
        // safe. -1 flows to "Server", exactly what UE itself reports for worker threads.
        const auto CanQueryPieId = IsInGameThread() && NOT IsInAsyncLoadingThread();
        const auto PieId = CanQueryPieId ? UE::GetPlayInEditorID() : -1;
        const auto* const ServerClientText = PieId - 1 < 0 ? TEXT("Server") : TEXT("Client");

        const auto& MessagePlusBpCallStack = ck::Format_UE(
            TEXT("[{}] {}\n{}\n\n == BP CallStack ==\n{}\n\n == AS CallStack ==\n{}"),
            ServerClientText,
            InExpressionText,
            InMessage,
            BpStackTrace,
            AsStackTrace);

        auto CleanScriptSections = FString{};
        if (NOT BpStackTrace.IsEmpty())
        {
            CleanScriptSections += ck::Format_UE(TEXT("\n\n == BP CallStack ==\n{}"), BpStackTrace);
        }
        if (NOT AsStackTrace.IsEmpty())
        {
            CleanScriptSections += ck::Format_UE(TEXT("\n\n == AS CallStack ==\n{}"), AsStackTrace);
        }
        const auto& CleanMessagePlusBpCallStack = ck::Format_UE(
            TEXT("[{}] {}\n{}{}"),
            ServerClientText,
            InExpressionText,
            InMessage,
            CleanScriptSections);

        const auto& MessagePlusBpCallStackStr = FText::FromString(CleanMessagePlusBpCallStack);

        if (IsEnsureFromScript && UCk_Utils_Ensure_UE::Get_IsEnsureIgnored_WithCallstack(BpStackTrace + AsStackTrace))
        { return; }

        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)
        {
            ck::ensure::Error(TEXT("{}"), MessagePlusBpCallStack);
            if (NOT IsEnsureFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
            return;
        }

    #if WITH_EDITOR
        ck::ensure::Error(TEXT("{}"), MessagePlusBpCallStack);

        const auto IsInPIE = GIsPlayInEditorWorld;
        const auto ShouldShowEditorNotification = GIsEditor && NOT IsInPIE;

        if (ShouldShowEditorNotification)
        {
            // The callstack is omitted deliberately — it triggers UI bugs in the notification widget.
            const auto& SimpleMessage = ck::Format_UE(
                TEXT("[{}] {}\n{}"),
                ServerClientText,
                InExpressionText,
                InMessage);

            InEditorMessageEmitter(
                FCk_Utils_EditorOnly_PushNewEditorMessage_Params
                {
                    TEXT("CkEnsures"),
                    FCk_MessageSegments
                    {
                        {
                            FCk_TokenizedMessage{SimpleMessage}.Set_TargetObject(nullptr)
                        }
                    }
                }
                .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)
                .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::DoNotDisplay)
                .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::DoNotFocus)
            );
        }

        // -unattended / commandlet contexts share the LogOnly fate: the Error log already ran above, but
        // FSlateApplication::AddModalWindow would spin on Sleep with no UI to dismiss it.
        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::LogOnly
            || FApp::IsUnattended()
            || IsRunningCommandlet())
        { return; }

        if (ShouldShowEditorNotification)
        { return; }
    #else
        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::LogOnly)
        {
            if (NOT IsEnsureFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
            UE_LOG(CkEnsure, Error, TEXT("%s"), *MessagePlusBpCallStack);
            return;
        }
    #endif

        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::MessageLog)
        {
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr, MessagePlusBpCallStackStr);
            if (BpStackTrace.IsEmpty() && NOT AsStackTrace.IsEmpty())
            {
                UCk_Utils_Debug_StackTrace_UE::Try_BreakInAngelscript(nullptr, MessagePlusBpCallStackStr);
            }
            if (NOT IsEnsureFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
            return;
        }

        const auto& TrimmedCppStack = Request_TrimEngineBoilerplateFrames(StackTraceFromEnsureCaller);

        auto DialogSections = FString{};
        if (NOT BpStackTrace.IsEmpty())
        {
            const auto& WrappedBp = Request_WrapMultilineTextWithRichTextTags(BpStackTrace, TEXT("EnsureCallstackContent"));
            DialogSections += ck::Format_UE(
                TEXT("\n<EnsureCallstackHeader>== BP CallStack ==</>\n{}\n"),
                WrappedBp);
        }
        if (NOT AsStackTrace.IsEmpty())
        {
            const auto& WrappedAs = Request_WrapMultilineTextWithRichTextTags(AsStackTrace, TEXT("EnsureCallstackContent"));
            DialogSections += ck::Format_UE(
                TEXT("\n<EnsureCallstackHeader>== AS CallStack ==</>\n{}\n"),
                WrappedAs);
        }
        if (NOT TrimmedCppStack.IsEmpty())
        {
            const auto& WrappedCpp = Request_WrapMultilineTextWithRichTextTags(TrimmedCppStack, TEXT("EnsureCppCallstackContent"));
            DialogSections += ck::Format_UE(
                TEXT("\n<EnsureCppCallstackHeader>== CallStack ==</>\n{}\n"),
                WrappedCpp);
        }

        const auto& CallstackPlusMessage = ck::Format_UE(
            TEXT("<EnsureFillerText>Frame#[{}] PIE-ID[{}]</>\n")
            TEXT("<EnsureServerClient>[{}]</> <EnsureExpression>{}</>\n")
            TEXT("<EnsureMessage>Message: {}</>\n{}"),
            GFrameCounter,
            UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld(),
            ServerClientText,
            InExpressionText,
            InMessage,
            DialogSections);
        const auto& DialogMessage = FText::FromString(CallstackPlusMessage);

        auto ClipboardSections = FString{};
        if (NOT BpStackTrace.IsEmpty())
        {
            ClipboardSections += ck::Format_UE(TEXT("\n\n## BP CallStack\n{}"), BpStackTrace);
        }
        if (NOT AsStackTrace.IsEmpty())
        {
            ClipboardSections += ck::Format_UE(TEXT("\n\n## AS CallStack\n{}"), AsStackTrace);
        }
        if (NOT TrimmedCppStack.IsEmpty())
        {
            ClipboardSections += ck::Format_UE(TEXT("\n\n## CallStack\n{}"), TrimmedCppStack);
        }

        const auto& ClipboardText = ck::Format_UE(
            TEXT("Frame#[{}] PIE-ID[{}]\n")
            TEXT("[{}] `{}`\n")
            TEXT("**Message:** {}{}\n"),
            GFrameCounter,
            UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld(),
            ServerClientText,
            InExpressionText,
            InMessage,
            ClipboardSections);
        const auto& ClipboardMessage = FText::FromString(ClipboardText);

        const auto HasBpStack = NOT BpStackTrace.IsEmpty();
        const auto HasAsStack = NOT AsStackTrace.IsEmpty();

        using DialogButton = UCk_Utils_MessageDialog_UE::DialogButton;
        auto Buttons = TArray<DialogButton>{};

        Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore Once")), FSimpleDelegate::CreateLambda([&]()
        {})}.Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f}));

        Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore All")), FSimpleDelegate::CreateLambda([&]()
        {
            if (NOT IsEnsureFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
        })}.Set_Color(FLinearColor{1.0f, 0.62f, 0.27f, 1.0f}).Set_IsPrimary(true).Set_ShouldFocus(true));

        Buttons.Add(DialogButton{FText::FromString(TEXT("  ")), {}}
        .Set_Color(FLinearColor{0.0f, 0.0f, 0.0f, 0.0f})
        .Set_EnableDisable(ECk_EnableDisable::Disable));

        Buttons.Add(DialogButton{FText::FromString(TEXT("Snooze")), FSimpleDelegate::CreateLambda([&]()
        {
            if (NOT IsEnsureFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsurePermanently_AtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsurePermanently_WithCallstack(BpStackTrace + AsStackTrace); }
        })}.Set_Color(FLinearColor{0.45f, 0.25f, 0.55f, 1.0f})
          .Set_Tooltip(FText::FromString(TEXT("Ignore this ensure until the Editor is restarted"))));

        Buttons.Add(DialogButton{FText::FromString(TEXT("Break")), {}}
        .Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f})
        .Set_EnableDisable(StackTraceFromEnsureCaller.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));

        if (GIsEditor)
        {
            Buttons.Add(DialogButton{FText::FromString(TEXT("Break in BP")), FSimpleDelegate::CreateLambda([&]()
            {
                UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);
            })}.Set_Color(FLinearColor{0.34f, 0.34f, 0.59f, 1.0f})
            .Set_EnableDisable(HasBpStack ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable));

            Buttons.Add(DialogButton{FText::FromString(TEXT("Break in AS")), FSimpleDelegate::CreateLambda([&]()
            {
                UCk_Utils_Debug_StackTrace_UE::Try_BreakInAngelscript(nullptr);
            })}.Set_Color(FLinearColor{0.59f, 0.34f, 0.34f, 1.0f})
            .Set_EnableDisable(HasAsStack ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable));
        }

        if (GIsEditor)
        {
            Buttons.Add(DialogButton{FText::FromString(TEXT("Abort PIE")), FSimpleDelegate::CreateLambda([&]()
            {
                UCk_Utils_Ensure_UE::Request_IgnoreAllEnsures();
                UCk_Utils_EditorOnly_UE::Request_AbortPIE();
            })}.Set_Color(FLinearColor{1.0f, 0.1f, 0.1f, 1.0f}));
        }

        if (const auto& ButtonIndex = UCk_Utils_MessageDialog_UE::CustomDialog(DialogMessage, ClipboardMessage, FText::FromString(Title), Buttons);
            ButtonIndex == 4)
        {
            OutBreakInCode = true;
            OutBreakInScript = HasBpStack || HasAsStack;
        }
    }

    auto
        Ensure_Impl(
            const FString& InMessage,
            const FString& InExpressionText,
            const FName& InFile,
            int32 InLine,
            bool& OutBreakInCode,
            bool& OutBreakInScript)
        -> void
    {
        Ensure_Impl_Internal(
            InMessage,
            InExpressionText,
            InFile,
            InLine,
            OutBreakInCode,
            OutBreakInScript,
            [](const FString& InReport)
            {
                Error(TEXT("{}"), InReport);
            },
            [](const FCk_Utils_EditorOnly_PushNewEditorMessage_Params& InParams)
            {
                UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage(InParams);
            });
    }

#if WITH_DEV_AUTOMATION_TESTS
    auto
        Ensure_Impl_ForTesting(
            const FString& InMessage,
            const FString& InExpressionText,
            const FName& InFile,
            int32 InLine,
            bool& OutBreakInCode,
            bool& OutBreakInScript,
            TFunctionRef<void(const FString&)> InWorkerReportEmitter,
            TFunctionRef<void(const FCk_Utils_EditorOnly_PushNewEditorMessage_Params&)> InEditorMessageEmitter)
        -> void
    {
        Ensure_Impl_Internal(
            InMessage,
            InExpressionText,
            InFile,
            InLine,
            OutBreakInCode,
            OutBreakInScript,
            InWorkerReportEmitter,
            InEditorMessageEmitter);
    }
#endif

    auto
        Do_HandleFail(
            const FString& InMessage,
            const FString& InExpressionText,
            const FName& InFile,
            int32 InLine)
        -> bool
    {
        auto ShouldBreakInCode = false;
        auto ShouldBreakInScript = false;
        Ensure_Impl(InMessage, InExpressionText, InFile, InLine, ShouldBreakInCode, ShouldBreakInScript);

        if (ShouldBreakInCode && ShouldBreakInScript)
        { Do_BreakInScript(); }

        return ShouldBreakInCode;
    }

    auto
      Do_BreakInScript()
      -> void
    {
      const auto BpStackTrace =
          UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
              ck::type_traits::AsString{});
      if (NOT BpStackTrace.IsEmpty()) {
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);
        return;
      }

      const auto AsStackTrace =
          UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
              ck::type_traits::AsString{});
      if (NOT AsStackTrace.IsEmpty()) {
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInAngelscript(nullptr);
      }
    }

    auto
      Get_IsEnsureFromScript()
      -> bool
    {
        return EnsureFromScriptDepth > 0;
    }

    auto
      Do_Push_EnsureIsFromScript()
      -> void
    {
        ++EnsureFromScriptDepth;
    }

    auto
      Do_Pop_EnsureIsFromScript()
      -> void
    {
        if (EnsureFromScriptDepth > 0)
        { --EnsureFromScriptDepth; }
    }
}

// --------------------------------------------------------------------------------------------------------------------
