#include "CkEditorOnly_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <GameplayTagsManager.h>

#if WITH_EDITOR
#include <Editor.h>
#include <GameplayTagsEditorModule.h>
#include <UnrealEd.h>
#include <UnrealEdGlobals.h>

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
    Request_AddGameplayTagToIni(
        FName TagName,
        const FString& Comment)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(TagName),
        TEXT("Cannot add Gameplay Tag [{}] to INI because the TagName is Invalid!"), TagName)
    { return {}; }

    const auto& Manager = UGameplayTagsManager::Get();

    if (Manager.RequestGameplayTag(TagName, false).IsValid())
    { return Manager.RequestGameplayTag(TagName); }

#if WITH_EDITOR

    auto& GameplayTagsEditorModule = FModuleManager::LoadModuleChecked<IGameplayTagsEditorModule>("GameplayTagsEditor");
    GameplayTagsEditorModule.AddNewGameplayTagToINI(TagName.ToString(), Comment);

    UGameplayTagsManager::Get().DoneAddingNativeTags();
    UGameplayTagsManager::Get().ConstructGameplayTagTree();

    CK_ENSURE_IF_NOT(Manager.RequestGameplayTag(TagName, false).IsValid(),
        TEXT("Failed to add Gameplay Tag [{}] to INI!"), TagName)
    { return {}; }

    return Manager.RequestGameplayTag(TagName);
#else
    CK_TRIGGER_ENSURE(TEXT("Cannot add Gameplay Tag [{}] to INI outside of the Editor!"));
    return {};
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Get_IsCommandletOrCooking()
    -> bool
{
    return FApp::IsUnattended() || IsRunningCommandlet() ||  IsRunningCookCommandlet() || Get_IsCookingByTheBook();
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
    return Get_DoesBlueprintImplementInterface(InBlueprint, InInterfaceClass.Get(), InIncludeInherited);
#else
    return {};
#endif
}

auto
    UCk_Utils_EditorOnly_UE::
    Get_DoesBlueprintImplementInterface(
        const UBlueprint* InBlueprint,
        UClass* InInterfaceClass,
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
    Cast_BlueprintClassToInterface(
        // ReSharper disable once CppPassValueParameterByConstReference
        TSubclassOf<UBlueprint> InInterfaceClass)
    -> TSubclassOf<UInterface>
{
    return InInterfaceClass.Get();
}

void UCk_Utils_EditorOnly_UE::UpdateActorCDOFromEditorInstance(AActor* EditorActorInstance)
{
    if (!EditorActorInstance || !IsValid(EditorActorInstance))
    {
        return;
    }

    // Get the CDO of the actor
    AActor* ActorCDO = EditorActorInstance->GetClass()->GetDefaultObject<AActor>();
    if (!ActorCDO)
    {
        return;
    }

    // Get all components from the editor instance
    TArray<UActorComponent*> EditorComponents;
    EditorActorInstance->GetComponents<UActorComponent>(EditorComponents);

    // Get all components from the CDO using the proper method
    TArray<const UActorComponent*> CDOComponents;
    AActor::GetActorClassDefaultComponents(EditorActorInstance->GetClass(), CDOComponents);

    // Update each component in the CDO with values from the editor instance
    for (UActorComponent* EditorComponent : EditorComponents)
    {
        if (!IsValid(EditorComponent))
        {
            continue;
        }

        // Find the corresponding component in the CDO
        const UActorComponent* CDOComponent = FindCorrespondingCDOComponent(EditorComponent, CDOComponents);

        if (CDOComponent && IsValid(CDOComponent))
        {
            CopyComponentProperties(EditorComponent, const_cast<UActorComponent*>(CDOComponent));
        }
    }

    // Mark the CDO as modified
    ActorCDO->MarkPackageDirty();

    // Refresh property views
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyEditorModule.UpdatePropertyViews({EditorActorInstance});
}

const UActorComponent* UCk_Utils_EditorOnly_UE::FindCorrespondingCDOComponent(
    UActorComponent* EditorComponent,
    const TArray<const UActorComponent*>& CDOComponents)
{
    if (!EditorComponent)
    {
        return nullptr;
    }

    // First try to match by component name
    FName ComponentName = EditorComponent->GetFName();
    for (const UActorComponent* CDOComponent : CDOComponents)
    {
        if (CDOComponent && CDOComponent->GetFName() == ComponentName)
        {
            // Also check if they're the same class
            if (CDOComponent->GetClass() == EditorComponent->GetClass())
            {
                return CDOComponent;
            }
        }
    }

    // If name matching fails, try to match by class and creation order
    UClass* ComponentClass = EditorComponent->GetClass();
    int32 EditorComponentIndex = 0;

    // Find the index of this component among components of the same class in editor instance
    AActor* EditorActor = EditorComponent->GetOwner();
    if (EditorActor)
    {
        TArray<UActorComponent*> SameClassComponents;
        EditorActor->GetComponents(ComponentClass, SameClassComponents);

        EditorComponentIndex = SameClassComponents.IndexOfByKey(EditorComponent);
    }

    // Find the component at the same index in CDO components
    TArray<const UActorComponent*> CDOSameClassComponents;
    for (const UActorComponent* CDOComponent : CDOComponents)
    {
        if (CDOComponent && CDOComponent->GetClass() == ComponentClass)
        {
            CDOSameClassComponents.Add(CDOComponent);
        }
    }

    if (CDOSameClassComponents.IsValidIndex(EditorComponentIndex))
    {
        return CDOSameClassComponents[EditorComponentIndex];
    }

    return nullptr;
}

void UCk_Utils_EditorOnly_UE::CopyComponentProperties(
    UActorComponent* SourceComponent,
    UActorComponent* TargetComponent)
{
    if (!SourceComponent || !TargetComponent || SourceComponent->GetClass() != TargetComponent->GetClass())
    {
        return;
    }

    UClass* ComponentClass = SourceComponent->GetClass();

    // Iterate through all properties of the component class
    for (FProperty* Property = ComponentClass->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
    {
        // Skip properties that shouldn't be copied
        if (ShouldSkipProperty(Property))
        {
            continue;
        }

        // Copy the property value
        Property->CopyCompleteValue(
            Property->ContainerPtrToValuePtr<void>(TargetComponent),
            Property->ContainerPtrToValuePtr<void>(SourceComponent)
        );
    }

    // Mark the target component as modified
    TargetComponent->MarkPackageDirty();
}

bool UCk_Utils_EditorOnly_UE::ShouldSkipProperty(FProperty* Property)
{
    if (!Property)
    {
        return true;
    }

    // Skip properties that are:
    // - Transient (not saved)
    // - Native (engine-managed)
    // - Deprecated
    // - Editor-only metadata
    if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient))
    {
        return true;
    }

    // Skip object references that might cause issues
    if (Property->IsA<FObjectProperty>() || Property->IsA<FWeakObjectProperty>() || Property->IsA<FSoftObjectProperty>())
    {
        return true;
    }

    // Skip delegate properties
    if (Property->IsA<FDelegateProperty>() || Property->IsA<FMulticastDelegateProperty>())
    {
        return true;
    }

    return false;
}

// Alternative simpler version for specific component types
template<typename T>
void UCk_Utils_EditorOnly_UE::UpdateSpecificComponentCDO(AActor* EditorActorInstance)
{
    static_assert(std::is_base_of_v<UActorComponent, T>, "T must be derived from UActorComponent");

    if (!EditorActorInstance)
    {
        return;
    }

    T* EditorComponent = EditorActorInstance->FindComponentByClass<T>();
    if (!EditorComponent)
    {
        return;
    }

    AActor* ActorCDO = EditorActorInstance->GetClass()->GetDefaultObject<AActor>();
    if (!ActorCDO)
    {
        return;
    }

    // Get CDO components using the proper method
    TArray<const UActorComponent*> CDOComponents;
    AActor::GetActorClassDefaultComponents(EditorActorInstance->GetClass(), CDOComponents);

    // Find the specific component type in CDO
    T* CDOComponent = nullptr;
    for (const UActorComponent* Component : CDOComponents)
    {
        if (T* TypedComponent = Cast<T>(Component))
        {
            CDOComponent = TypedComponent;
            break;
        }
    }

    if (!CDOComponent)
    {
        return;
    }

    // Copy properties from editor instance to CDO
    CopyComponentProperties(EditorComponent, CDOComponent);
}

// Additional utility function to get CDO component by class
template<typename T>
T* UCk_Utils_EditorOnly_UE::GetCDOComponentByClass(AActor* ActorCDO)
{
    static_assert(std::is_base_of_v<UActorComponent, T>, "T must be derived from UActorComponent");

    if (!ActorCDO)
    {
        return nullptr;
    }

    TArray<const UActorComponent*> CDOComponents;
    AActor::GetActorClassDefaultComponents(ActorCDO->GetClass(), CDOComponents);

    for (const UActorComponent* Component : CDOComponents)
    {
        if (T* TypedComponent = Cast<T>(Component))
        {
            return TypedComponent;
        }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
