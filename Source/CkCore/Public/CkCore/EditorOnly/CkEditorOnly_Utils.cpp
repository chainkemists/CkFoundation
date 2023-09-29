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
    const auto& loggerToUse     = InParams.Get_LoggerToUse();
    const auto& messageSeverity = InParams.Get_MessageSeverity();
    const auto& displayPolicy   = InParams.Get_DisplayPolicy();
    const auto& forceOpenWindow = displayPolicy == ECk_EditorMessage_DisplayPolicy::ToastNotificationAndOpenMessageLogWindow;

    CK_ENSURE_IF_NOT(ck::IsValid(loggerToUse), TEXT("Cannot push a new Editor Message because the supplied Logger Name to use is Invalid!"))
    { return; }

    const auto& FormatMessage = [&](TSharedPtr<FTokenizedMessage> InMessageToFormat) -> void
    {
        const auto& messageSegments = InParams.Get_MessageSegments();

        for (const auto& segment : messageSegments.Get_MessageSegments())
        {
            if (ck::IsValid(segment.Get_TargetObject()))
            {
                InMessageToFormat->AddToken(FUObjectToken::Create(segment.Get_TargetObject()));
            }

            if (NOT segment.Get_DocumentationLink().IsEmpty())
            {
                InMessageToFormat->AddToken(FDocumentationToken::Create(segment.Get_DocumentationLink()));
            }

            InMessageToFormat->AddToken(FTextToken::Create(FText::FromString(segment.Get_Message())));
        }
    };

    auto& messageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));

    if (NOT messageLogModule.IsRegisteredLogListing(loggerToUse))
    {
        FMessageLogInitializationOptions initOptions;
        initOptions.bShowFilters = true;

        messageLogModule.RegisterLogListing(loggerToUse, FText::FromName(loggerToUse), initOptions);
    }

    FMessageLog messageLog(loggerToUse);
    static constexpr auto forceNotify = true;

    switch (messageSeverity)
    {
        case ECk_EditorMessage_Severity::Info:
        {
            const auto& messageToFormat = messageLog.Info();
            FormatMessage(messageToFormat);
            messageLog.Notify(messageToFormat->ToText(), EMessageSeverity::Info, forceNotify);
            messageLog.Open(EMessageSeverity::Info, forceOpenWindow);
            break;
        }
        case ECk_EditorMessage_Severity::Error:
        {
            const auto& messageToFormat = messageLog.Error();
            FormatMessage(messageToFormat);
            messageLog.Notify(messageToFormat->ToText(), EMessageSeverity::Error, forceNotify);
            messageLog.Open(EMessageSeverity::Error, forceOpenWindow);
            break;
        }
        case ECk_EditorMessage_Severity::PerformanceWarning:
        {
            const auto& messageToFormat = messageLog.PerformanceWarning();
            FormatMessage(messageToFormat);
            messageLog.Notify(messageToFormat->ToText(), EMessageSeverity::PerformanceWarning, forceNotify);
            messageLog.Open(EMessageSeverity::PerformanceWarning, forceOpenWindow);
            break;
        }
        case ECk_EditorMessage_Severity::Warning:
        {
            const auto& messageToFormat = messageLog.Warning();
            FormatMessage(messageToFormat);
            messageLog.Notify(messageToFormat->ToText(), EMessageSeverity::Warning, forceNotify);
            messageLog.Open(EMessageSeverity::Warning, forceOpenWindow);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(messageSeverity);
            break;
        }
    }
#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------
