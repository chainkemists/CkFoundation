#include "CkEditorOnly_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#if WITH_EDITOR
#include <UnrealEdGlobals.h>

#include <Editor.h>
#include <UnrealEd.h>
#include <Editor/UnrealEdEngine.h>
#include <Kismet2/BlueprintEditorUtils.h>
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
    Get_DebugStringForWorld(
        const UObject* InContextObject)
    -> FString
{
#if WITH_EDITOR
    if (ck::IsValid(InContextObject))
    {
        if (const auto& ContextWorld = GEngine->GetWorldFromContextObject(InContextObject, EGetWorldErrorMode::ReturnNull);
            ck::IsValid(ContextWorld))
        {
            return GetDebugStringForWorld(ContextWorld);
        }
    }

    if (NOT ck::IsValid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
    { return TEXT("GEngine has been destroyed"); }

    const auto WorldContextFromPie = GEngine->GetWorldContextFromPIEInstance(UE::GetPlayInEditorID());

    if (NOT ck::IsValid(WorldContextFromPie, ck::IsValid_Policy_NullptrOnly{}))
    { return TEXT("GEngine is VALID but WorldContextFromPIE is NOT valid"); }

    return GetDebugStringForWorld(WorldContextFromPie->World());
#else
    return TEXT("N/A");
#endif
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
#else
    return false;
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Get_PieNetModeNamePrefix(
        const UObject* InContextObject)
    -> FString
{
    static FString UnknownPieNetModeNamePrefix = TEXT("Unknown");

#if WITH_EDITOR
    if (ck::Is_NOT_Valid(GEngine))
    { return UnknownPieNetModeNamePrefix; }

    const auto& ContextWorld = GEngine->GetWorldFromContextObject(InContextObject, EGetWorldErrorMode::ReturnNull);

    if (ck::Is_NOT_Valid(ContextWorld))
    { return UnknownPieNetModeNamePrefix; }

    if (ContextWorld->WorldType != EWorldType::PIE)
    { return UnknownPieNetModeNamePrefix; }

    switch(ContextWorld->GetNetMode())
    {
        case NM_Client:
        {
            return ck::Format_UE(TEXT("Client #{}"), UE::GetPlayInEditorID());
        }
        case NM_DedicatedServer:
        case NM_ListenServer:
        {
            return TEXT("Server");
        }
        case NM_Standalone:
        case NM_MAX:
        default:
        {
            return UnknownPieNetModeNamePrefix;
        }
    }
#else
    return UnknownPieNetModeNamePrefix;
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Get_DoesBlueprintImplementInterface(
        const UBlueprint* InBlueprint,
        TSubclassOf<UInterface> InInterfaceClass,
        bool InIncludeInherited)
    -> bool
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InInterfaceClass) || ck::Is_NOT_Valid(InBlueprint))
    { return {}; }

    return FBlueprintEditorUtils::ImplementsInterface(InBlueprint, InIncludeInherited, InInterfaceClass);
#else
    return {};
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_AbortPIE()
    -> void
{
#if WITH_EDITOR
    if (ck::IsValid(GEditor))
    {
        GEditor->RequestEndPlayMap();
    }
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_ImplementNewInterface(
        UBlueprint* InBlueprint,
        TSubclassOf<UInterface> InInterfaceClass)
    -> ECk_SucceededFailed
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InInterfaceClass))
    { return ECk_SucceededFailed::Failed; }

    if (constexpr auto IncludeInherited = true;
        Get_DoesBlueprintImplementInterface(InBlueprint, InInterfaceClass, IncludeInherited))
    { return ECk_SucceededFailed::Failed; }

    if (NOT FBlueprintEditorUtils::ImplementNewInterface(InBlueprint, InInterfaceClass->GetClassPathName()))
    { return ECk_SucceededFailed::Failed; }

    return ECk_SucceededFailed::Succeeded;
#else
    return ECk_SucceededFailed::Failed;
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_RemoveInterface(
        UBlueprint* InBlueprint,
        TSubclassOf<UInterface> InInterfaceClass)
    -> void
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InInterfaceClass))
    { return; }

    if (constexpr auto IncludeInherited = true;
        NOT Get_DoesBlueprintImplementInterface(InBlueprint, InInterfaceClass, IncludeInherited))
    { return; }

    FBlueprintEditorUtils::RemoveInterface(InBlueprint, InInterfaceClass->GetClassPathName());
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Request_AddActorComponentToBlueprint(
        UBlueprint* InBlueprint,
        TSubclassOf<UActorComponent> InComponentClass)
    -> bool
{
#if WITH_EDITOR
    if (ck::Is_NOT_Valid(InComponentClass) || ck::Is_NOT_Valid(InBlueprint))
    { return {}; }

    if (const auto& BlueprintClass = InBlueprint->GeneratedClass;
        NOT BlueprintClass->IsChildOf(AActor::StaticClass()))
    { return {}; }

    const auto& BSCS = InBlueprint->SimpleConstructionScript;
    if (ck::Is_NOT_Valid(BSCS))
    { return {}; }

    if (ck::algo::AnyOf(BSCS->GetAllNodes(), [&](const auto& InNode){ return ck::IsValid(InNode) && InNode->ComponentClass == InComponentClass; }))
    { return {}; }

    const auto& NewNode = BSCS->CreateNode(InComponentClass);
    if (ck::Is_NOT_Valid(NewNode))
    { return {}; }

    NewNode->SetVariableName(FName(*InComponentClass->GetName()));
    BSCS->AddNode(NewNode);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(InBlueprint);

    return true;
#else
    return {};
#endif

}

auto
    UCk_Utils_EditorOnly_UE::
    Cast_BlueprintClassToInterface(
        // ReSharper disable once CppPassValueParameterByConstReference
        TSubclassOf<UBlueprint> InInterfaceClass)
    -> TSubclassOf<UInterface>
{
    return InInterfaceClass.Get();
}

// --------------------------------------------------------------------------------------------------------------------
