#include "CkEditorOnly_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#if WITH_EDITOR
#include <UnrealEdGlobals.h>

#include <Editor/UnrealEdEngine.h>
#include <Editor.h>
#include <UnrealEd.h>
#include <Logging/MessageLog.h>
#include <Logging/TokenizedMessage.h>

#include <Misc/UObjectToken.h>

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2
#include <MessageLog/Public/IMessageLogListing.h>
#include <MessageLog/Public/MessageLogModule.h>
#else
#include "Developer/MessageLog/Public/MessageLogModule.h"
#endif

#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EditorOnly_UE::
    Request_PushNewEditorMessage(
        const FCk_Utils_EditorOnly_PushNewEditorMessage_Params& InParams)
    -> void
{
#if WITH_EDITOR
    const auto& LoggerToUse     = InParams.Get_LoggerToUse();
    const auto& MessageSeverity = InParams.Get_MessageSeverity();
    const auto& ToastDisplayPolicy   = InParams.Get_ToastNotificationDisplayPolicy();
    const auto& MessageDisplayPolicy = InParams.Get_MessageLogDisplayPolicy();

    CK_ENSURE_IF_NOT(ck::IsValid(LoggerToUse), TEXT("Cannot push a new Editor Message because the supplied Logger Name to use is Invalid!"))
    { return; }

    const auto& FormatMessage = [&](const TSharedPtr<FTokenizedMessage>& InMessageToFormat) -> void
    {
        for (const auto& MessageSegments = InParams.Get_MessageSegments();
            const auto& Segment : MessageSegments.Get_MessageSegments())
        {
            if (ck::IsValid(Segment.Get_TargetObject()))
            {
                InMessageToFormat->AddToken(FUObjectToken::Create(Segment.Get_TargetObject()));
            }

            if (NOT Segment.Get_DocumentationLink().IsEmpty())
            {
                InMessageToFormat->AddToken(FDocumentationToken::Create(Segment.Get_DocumentationLink()));
            }

            InMessageToFormat->AddToken(FTextToken::Create(FText::FromString(Segment.Get_Message())));
        }
    };

    if (auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));
        NOT MessageLogModule.IsRegisteredLogListing(LoggerToUse))
    {
        FMessageLogInitializationOptions InitOptions;
        InitOptions.bShowFilters = true;

        MessageLogModule.RegisterLogListing(LoggerToUse, FText::FromName(LoggerToUse), InitOptions);
    }

    FMessageLog MessageLog(LoggerToUse);
    const auto ToastNotify = ToastDisplayPolicy == ECk_EditorMessage_ToastNotification_DisplayPolicy::Display;
    const auto MessageLogFocus = MessageDisplayPolicy == ECk_EditorMessage_MessageLog_DisplayPolicy::Focus;

    switch (MessageSeverity)
    {
        case ECk_EditorMessage_Severity::Info:
        {
            const auto& MessageToFormat = MessageLog.Info();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::Info, ToastNotify);
            MessageLog.Open(EMessageSeverity::Info, MessageLogFocus);
            break;
        }
        case ECk_EditorMessage_Severity::Error:
        {
            const auto& MessageToFormat = MessageLog.Error();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::Error, ToastNotify);
            MessageLog.Open(EMessageSeverity::Error, MessageLogFocus);
            break;
        }
        case ECk_EditorMessage_Severity::PerformanceWarning:
        {
            const auto& MessageToFormat = MessageLog.PerformanceWarning();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::PerformanceWarning, ToastNotify);
            MessageLog.Open(EMessageSeverity::PerformanceWarning, MessageLogFocus);
            break;
        }
        case ECk_EditorMessage_Severity::Warning:
        {
            const auto& MessageToFormat = MessageLog.Warning();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::Warning, ToastNotify);
            MessageLog.Open(EMessageSeverity::Warning, MessageLogFocus);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(MessageSeverity);
            break;
        }
    }
#endif // WITH_EDITOR
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_DebugPauseExecution()
    -> void
{
#if WITH_EDITOR
    if (ck::IsValid(GUnrealEd, ck::IsValid_Policy_NullptrOnly{}))
    { GUnrealEd->PlayWorld->bDebugPauseExecution = true; }
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_DebugResumeExecution()
    -> void
{
#if WITH_EDITOR
    if (ck::IsValid(GUnrealEd, ck::IsValid_Policy_NullptrOnly{}))
    { GUnrealEd->PlayWorld->bDebugPauseExecution = false; }
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_RedrawLevelEditingViewports()
    -> void
{
#if WITH_EDITOR
    if (ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{}))
    {
        constexpr auto InvalidateHitProxies = false;
        GEditor->RedrawLevelEditingViewports(InvalidateHitProxies);
    }
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Get_IsCommandletOrCooking()
    -> bool
{
    return FApp::IsUnattended() || IsRunningCommandlet() || Get_IsCookingByTheBook();
}

auto
    UCk_Utils_EditorOnly_UE::
    Get_IsCookingByTheBook()
    -> bool
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(GUnrealEd, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    const auto& CookServer = GUnrealEd->CookServer.Get();

    if (ck::Is_NOT_Valid(CookServer, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    return CookServer->IsCookByTheBookRunning();
#endif

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
