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

auto
    CkEnsure_Impl(
        const FString& InMessage,
        const FString& InExpressionText,
        const FName& InFile,
        int32 InLine)
    -> void
{
    UCk_Utils_Ensure_UE::Request_IncrementEnsureCountAtFileAndLine(InFile, InLine);

    if (UCk_Utils_Ensure_UE::Get_IsEnsureIgnored(InFile, InLine))
    { return; }

    const auto IsMessageOnly = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;

    const auto& Title = ck::Format_UE(TEXT("Frame#[{}] PIE-ID[{}]"), GFrameCounter, UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld());
    const auto& StackTraceWith2Skips = IsMessageOnly ?
        TEXT("[StackTrace DISABLED]") :
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(2);
    const auto& BpStackTrace = IsMessageOnly ?
        TEXT("[BP StackTrace DISABLED]") :
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{});
    const auto& MessagePlusBpCallStack = ck::Format_UE(
        TEXT("[{}] {}\n{}\n\n == BP CallStack ==\n{}"),
        GPlayInEditorID - 1 < 0 ? TEXT("Server") : TEXT("Client"),
        InExpressionText,
        InMessage,
        BpStackTrace);

    const auto& MessagePlusBpCallStackStr = FText::FromString(MessagePlusBpCallStack);

    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)
    {
        ck::core::Error(TEXT("{}"), MessagePlusBpCallStack);
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, MessagePlusBpCallStackStr, InLine);
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
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, FText::FromString(MessagePlusBpCallStack), InLine);
        UE_LOG(CkCore, Error, TEXT("%s"), *MessagePlusBpCallStack);
        return;
    }
#endif

    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::MessageLog)
    {
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr, MessagePlusBpCallStackStr);
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, MessagePlusBpCallStackStr, InLine);
        return;
    }

    const auto& CallstackPlusMessage = ck::Format_UE(
        TEXT("{}\nExpression: {}\nMessage: {}\n\n == BP CallStack ==\n{}\n == CallStack ==\n{}\n"),
        Title,
        InExpressionText,
        InMessage,
        BpStackTrace,
        StackTraceWith2Skips);
    const auto& DialogMessage = FText::FromString(CallstackPlusMessage);

    using DialogButton = UCk_Utils_MessageDialog_UE::DialogButton;
    auto Buttons = TArray<DialogButton>{};

    Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore Once")), FSimpleDelegate::CreateLambda([&]()
    {})}.Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f}));

    Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore All")), FSimpleDelegate::CreateLambda([&]()
    {
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, DialogMessage, InLine);
    })}.Set_Color(FLinearColor{1.0f, 0.62f, 0.27f, 1.0f}).Set_IsPrimary(true).Set_ShouldFocus(true));


    Buttons.Add(DialogButton{FText::FromString(TEXT("Break")), {}}
    .Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f})
    .Set_EnableDisable(StackTraceWith2Skips.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));

    if (GIsEditor)
    {
    Buttons.Add(DialogButton{FText::FromString(TEXT("Break in BP")), FSimpleDelegate::CreateLambda([&]()
    {
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);
    })}.Set_Color(FLinearColor{0.34f, 0.34f, 0.59f, 1.0f})
    .Set_EnableDisable(BpStackTrace.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));
    }

    if (GIsEditor)
    {
        Buttons.Add(DialogButton{FText::FromString(TEXT("Abort PIE")), FSimpleDelegate::CreateLambda([&]()
        {
            UCk_Utils_Ensure_UE::Request_IgnoreAllEnsures();
            UCk_Utils_EditorOnly_UE::Request_AbortPIE();
        })}.Set_Color(FLinearColor{1.0f, 0.1f, 0.1f, 1.0f}));
    }

    auto ButtonIndex = UCk_Utils_MessageDialog_UE::CustomDialog(DialogMessage, FText::FromString(Title), Buttons);
    if (ButtonIndex == 2)
    {
        ensureAlwaysMsgf(false, TEXT("[DEBUG BREAK HIT]"));
        if (NOT BpStackTrace.IsEmpty())
        {
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------