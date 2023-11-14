#include "CkEditorOnly_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#if WITH_EDITOR
#include <Logging/TokenizedMessage.h>
#include <Logging/MessageLog.h>
#include <Misc/UObjectToken.h>
#include <MessageLog/Public/MessageLogModule.h>
#include <MessageLog/Public/IMessageLogListing.h>
#include <Editor.h>
#include <UnrealEd.h>
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
    const auto& DisplayPolicy   = InParams.Get_DisplayPolicy();
    const auto& ForceOpenWindow = DisplayPolicy == ECk_EditorMessage_DisplayPolicy::ToastNotificationAndOpenMessageLogWindow;

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
    static constexpr auto _ForceNotify = true;

    switch (MessageSeverity)
    {
        case ECk_EditorMessage_Severity::Info:
        {
            const auto& MessageToFormat = MessageLog.Info();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::Info, _ForceNotify);
            MessageLog.Open(EMessageSeverity::Info, ForceOpenWindow);
            break;
        }
        case ECk_EditorMessage_Severity::Error:
        {
            const auto& MessageToFormat = MessageLog.Error();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::Error, _ForceNotify);
            MessageLog.Open(EMessageSeverity::Error, ForceOpenWindow);
            break;
        }
        case ECk_EditorMessage_Severity::PerformanceWarning:
        {
            const auto& MessageToFormat = MessageLog.PerformanceWarning();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::PerformanceWarning, _ForceNotify);
            MessageLog.Open(EMessageSeverity::PerformanceWarning, ForceOpenWindow);
            break;
        }
        case ECk_EditorMessage_Severity::Warning:
        {
            const auto& MessageToFormat = MessageLog.Warning();
            FormatMessage(MessageToFormat);
            MessageLog.Notify(MessageToFormat->ToText(), EMessageSeverity::Warning, _ForceNotify);
            MessageLog.Open(EMessageSeverity::Warning, ForceOpenWindow);
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

// --------------------------------------------------------------------------------------------------------------------
