#include "CkEnsure.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/MessageDialog/CkMessageDialog_Utils.h"
#include "CkCore/Settings/CkCore_Settings.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"

#include <CoreMinimal.h>
#include <Windows/WindowsPlatformApplicationMisc.h> // required for clipboard copy


// --------------------------------------------------------------------------------------------------------------------
namespace ck::ensure
{
    static auto EnsureIsFromScript = false;

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

            // Sanitize the line content
            auto SanitizedLine = Lines[i];
            SanitizedLine = SanitizedLine.Replace(TEXT("`"), TEXT("'"));  // Replace backticks
            SanitizedLine = SanitizedLine.Replace(TEXT("<"), TEXT("&lt;")); // Escape < characters
            SanitizedLine = SanitizedLine.Replace(TEXT(">"), TEXT("&gt;")); // Escape > characters

            Result += ck::Format_UE(TEXT("<{}>{}</>"), InTagName, SanitizedLine);
        }

        return Result;
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
        OutBreakInCode = false;
        OutBreakInScript = false;

        UCk_Utils_Ensure_UE::Request_IncrementEnsureCountAtFileAndLine(InFile, InLine);

        if (NOT EnsureIsFromScript && UCk_Utils_Ensure_UE::Get_IsEnsureIgnored(InFile, InLine))
        { return; }

        const auto IsMessageOnly = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;

        const auto& Title = ck::Format_UE(TEXT("Frame#[{}] PIE-ID[{}]"), GFrameCounter, UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld());
        const auto& StackTraceWith2Skips = IsMessageOnly ?
            TEXT("[StackTrace DISABLED]") :
            UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(2);
        const auto& BpStackTrace = IsMessageOnly ?
            TEXT("[BP StackTrace DISABLED]") :
            UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{});
        const auto& AsStackTrace = IsMessageOnly ?
            TEXT("[AS StackTrace DISABLED]") :
            UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(ck::type_traits::AsString{});
        const auto& MessagePlusBpCallStack = ck::Format_UE(
            TEXT("[{}] {}\n{}\n\n == BP CallStack ==\n{}\n\n == AS CallStack ==\n{}"),
            UE::GetPlayInEditorID() - 1 < 0 ? TEXT("Server") : TEXT("Client"),
            InExpressionText,
            InMessage,
            BpStackTrace,
            AsStackTrace);

        const auto& MessagePlusBpCallStackStr = FText::FromString(MessagePlusBpCallStack);

        if (EnsureIsFromScript && UCk_Utils_Ensure_UE::Get_IsEnsureIgnored_WithCallstack(BpStackTrace + AsStackTrace))
        { return; }

        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)
        {
            ck::core::Error(TEXT("{}"), MessagePlusBpCallStack);
            if (NOT EnsureIsFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
            return;
        }

    #if WITH_EDITOR
        // ReSharper disable once CppInconsistentNaming
        UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage
        (
            FCk_Utils_EditorOnly_PushNewEditorMessage_Params
            {
                TEXT("CkEnsures"),
                FCk_MessageSegments
                {
                    {
                        FCk_TokenizedMessage{MessagePlusBpCallStack}.Set_TargetObject(nullptr)
                    }
                }
            }
            .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)
            .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::DoNotDisplay)
            .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::DoNotFocus)
        );
        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::LogOnly)
        { return; }
    #else
        // ReSharper disable once CppInconsistentNaming
        #define _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE(_Category_, _Msg_, _ContextObject_)
        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::LogOnly)
        {
            if (NOT EnsureIsFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
            UE_LOG(CkCore, Error, TEXT("%s"), *MessagePlusBpCallStack);
            return;
        }
    #endif

        if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::MessageLog)
        {
            // Try Blueprint first, then Angelscript
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr, MessagePlusBpCallStackStr);
            if (BpStackTrace.IsEmpty() && NOT AsStackTrace.IsEmpty())
            {
                UCk_Utils_Debug_StackTrace_UE::Try_BreakInAngelscript(nullptr, MessagePlusBpCallStackStr);
            }
            if (NOT EnsureIsFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
            return;
        }

        const auto& ServerClientText = UE::GetPlayInEditorID() - 1 < 0 ? TEXT("Server") : TEXT("Client");

        // Process the callstack content to wrap each line
        const auto& WrappedBpStackTrace = Request_WrapMultilineTextWithRichTextTags(
            BpStackTrace.IsEmpty() ? TEXT("") : BpStackTrace,
            TEXT("EnsureCallstackContent"));

        const auto& WrappedAsStackTrace = Request_WrapMultilineTextWithRichTextTags(
            AsStackTrace.IsEmpty() ? TEXT("") : AsStackTrace,
            TEXT("EnsureCallstackContent"));

        const auto& WrappedCppStackTrace = Request_WrapMultilineTextWithRichTextTags(
            StackTraceWith2Skips.IsEmpty() ? TEXT("") : StackTraceWith2Skips,
            TEXT("EnsureCppCallstackContent"));

        const auto& CallstackPlusMessage = ck::Format_UE(
            TEXT("<EnsureFillerText>Frame#[{}] PIE-ID[{}]</>\n")
            TEXT("<EnsureServerClient>[{}]</> <EnsureExpression>{}</>\n")
            TEXT("<EnsureMessage>Message: {}</>\n\n")
            TEXT("<EnsureCallstackHeader>== BP CallStack ==</>\n")
            TEXT("{}\n\n")
            TEXT("<EnsureCallstackHeader>== AS CallStack ==</>\n")
            TEXT("{}\n\n")
            TEXT("<EnsureCppCallstackHeader>== CallStack ==</>\n")
            TEXT("{}\n"),
            GFrameCounter,
            UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld(),
            ServerClientText,
            InExpressionText,
            InMessage,
            WrappedBpStackTrace,
            WrappedAsStackTrace,
            WrappedCppStackTrace);
        const auto& DialogMessage = FText::FromString(CallstackPlusMessage);

        // Check stack availability
        const auto HasBpStack = NOT BpStackTrace.IsEmpty();
        const auto HasAsStack = NOT AsStackTrace.IsEmpty();

        using DialogButton = UCk_Utils_MessageDialog_UE::DialogButton;
        auto Buttons = TArray<DialogButton>{};

        Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore Once")), FSimpleDelegate::CreateLambda([&]()
        {})}.Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f}));

        Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore All")), FSimpleDelegate::CreateLambda([&]()
        {
            if (NOT EnsureIsFromScript)
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLine(InFile, InLine); }
            else
            { UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(BpStackTrace + AsStackTrace); }
        })}.Set_Color(FLinearColor{1.0f, 0.62f, 0.27f, 1.0f}).Set_IsPrimary(true).Set_ShouldFocus(true));

        Buttons.Add(DialogButton{FText::FromString(TEXT("Break")), {}}
        .Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f})
        .Set_EnableDisable(StackTraceWith2Skips.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));

        if (GIsEditor)
        {
            // Add Break in BP button
            Buttons.Add(DialogButton{FText::FromString(TEXT("Break in BP")), FSimpleDelegate::CreateLambda([&]()
            {
                UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);
            })}.Set_Color(FLinearColor{0.34f, 0.34f, 0.59f, 1.0f})
            .Set_EnableDisable(HasBpStack ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable));

            // Add Break in AS button
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

        if (const auto& ButtonIndex = UCk_Utils_MessageDialog_UE::CustomDialog(DialogMessage, FText::FromString(Title), Buttons);
            ButtonIndex == 2)
        {
            OutBreakInCode = true;
            OutBreakInScript = HasBpStack || HasAsStack;
        }
    }

    auto
      Do_BreakInScript()
      -> void
    {
      // Try Blueprint first
      const auto BpStackTrace =
          UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
              ck::type_traits::AsString{});
      if (NOT BpStackTrace.IsEmpty()) {
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);
        return;
      }

      // If no BP stack, try Angelscript
      const auto AsStackTrace =
          UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
              ck::type_traits::AsString{});
      if (NOT AsStackTrace.IsEmpty()) {
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInAngelscript(nullptr);
      }
    }

    auto
      Do_Push_EnsureIsFromScript()
      -> void
    {
        EnsureIsFromScript = true;
    }

    auto
      Do_Pop_EnsureIsFromScript()
      -> void
    {
        EnsureIsFromScript = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------