#include "CkMessaging_K2Node.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkMessaging/Public/CkMessaging/CkMessaging_Utils.h"

#include <GraphEditorSettings.h>
#include <K2Node_ConstructObjectFromClass.h>
#include <K2Node_MakeStruct.h>
#include <K2Node_BreakStruct.h>
#include <K2Node_DynamicCast.h>
#include <K2Node_VariableGet.h>
#include <K2Node_CustomEvent.h>
#include <KismetCompiler.h>
#include <Misc/UObjectToken.h>
#include <EdGraphSchema_K2.h>
#include <BlueprintNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Kismet/GameplayStatics.h>
#include <StructUtilsFunctionLibrary.h>

#define LOCTEXT_NAMESPACE "K2Node_Messaging"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2node_messaging
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_MessageReceived = TEXT("MessageReceived");
    static auto PinName_StopListening = TEXT("StopListening");
}

auto
   UCk_K2Node_Messaging_Broadcast::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);

    if (const auto* Action = GetClass();
        InActionRegistrar.IsOpenForRegistration(Action))
    {
        const auto& CustomizeLambda = [](UEdGraphNode* InNewNode, bool InIsTemplateNode, FName InFunctionName) -> void
        {
            auto* Node = CastChecked<UCk_K2Node_Messaging_Broadcast>(InNewNode);
            const auto* Function = UCk_Utils_Messaging_UE::StaticClass()->FindFunctionByName(InFunctionName);
            check(ck::IsValid(Function));

            Node->SetFromFunction(Function);
        };

        auto* Broadcast_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(Broadcast_NodeSpawner));

        Broadcast_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, INTERNAL__Broadcast));

        InActionRegistrar.AddBlueprintAction(Action, Broadcast_NodeSpawner);
    }
}

auto
    UCk_K2Node_Messaging_Broadcast::
    IsNodePure() const
    -> bool
{
    return false;
}

auto
    UCk_K2Node_Messaging_Broadcast::
    IsConnectionDisallowed(
        const UEdGraphPin* InMyPin,
        const UEdGraphPin* InOtherPin,
        FString& OutReason) const
    -> bool
{
    if (const auto* ValuePin = FindPinChecked(FName(TEXT("InValue")));
        InMyPin == ValuePin && InMyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if (InOtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct)
        {
            OutReason = TEXT("Value must be a Struct");
            return true;
        }

        if (InOtherPin->PinType.ContainerType != EPinContainerType::None)
        {
            OutReason = TEXT("Value cannot be an Array/Set/Map");
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_Message_Broadcast::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
                                ? PropertyChangedEvent.Property->GetFName()
                                : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Message_Broadcast, _MessageDefinition))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto
    UCk_K2Node_Message_Broadcast::
    ShouldShowNodeProperties() const
    -> bool
{
    return true;
}

auto
    UCk_K2Node_Message_Broadcast::
    PreloadRequiredAssets()
    -> void
{
    Super::PreloadRequiredAssets();

    PreloadObject(_MessageDefinition);
}

auto
    UCk_K2Node_Message_Broadcast::
    GetNodeTitle(
        ENodeTitleType::Type TitleType) const
    -> FText
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Broadcast"),
            TEXT("[Ck][Messaging] Broadcast New Message (INVALID Definition)")
        );
    }

    const auto& MessageDefinitionCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Message_Definition_PDA>(_MessageDefinition);
    if (ck::Is_NOT_Valid(MessageDefinitionCDO->Get_MessageName()))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Broadcast"),
            TEXT("[Ck][Messaging] Broadcast New Message (INVALID Message Name)")
        );
    }

    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Broadcast"),
        *ck::Format_UE(TEXT("[Ck][Messaging] Broadcast New Message ({})"), MessageDefinitionCDO->Get_MessageName())
    );
}

auto
    UCk_K2Node_Message_Broadcast::
    GetIconAndTint(
        FLinearColor& OutColor) const
    -> FSlateIcon
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

auto
    UCk_K2Node_Message_Broadcast::
    GetNodeContextMenuActions(
        UToolMenu* Menu,
        UGraphNodeContextMenuContext* Context) const
    -> void
{
    Super::GetNodeContextMenuActions(Menu, Context);

    FToolMenuSection& Section = Menu->AddSection("CkFoundation", FText::FromString(TEXT("Messaging")));
    Section.AddMenuEntry(
        "RefreshNode",
        FText::FromString(TEXT("Refresh")),
        FText::FromString(TEXT("Refresh this node")),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateUObject(const_cast<UCk_K2Node_Message_Broadcast*>(this), &UCk_K2Node_Message_Broadcast::RefreshTemplateMessageDefinition),
            FCanExecuteAction(),
            FIsActionChecked()
        )
    );
}

auto
    UCk_K2Node_Message_Broadcast::
    GetMenuCategory() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Broadcast"),
        TEXT("Ck|Utils|Messaging")
    );
}

auto
    UCk_K2Node_Message_Broadcast::
    IsNodePure() const
    -> bool
{
    return false;
}

auto
    UCk_K2Node_Message_Broadcast::
    AutowireNewNode(
        UEdGraphPin* FromPin)
    -> void
{
    if (ck::Is_NOT_Valid(FromPin, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    Super::AutowireNewNode(FromPin);
}

auto
    UCk_K2Node_Message_Broadcast::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    AllocateDefaultPins();
    CreatePinsFromMessageDefinition();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_Message_Broadcast::
    ReconstructNode()
    -> void
{
    Super::ReconstructNode();
    CreatePinsFromMessageDefinition();
}

auto
    UCk_K2Node_Message_Broadcast::
    DoAllocate_DefaultPins()
    -> void
{
    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2node_messaging::PinName_Handle
    );

    CreatePinsFromMessageDefinition();
}

auto
    UCk_K2Node_Message_Broadcast::
    DoExpandNode(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)
    -> void
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Missing Message Definition", "Invalid Message Definition. @@").ToString(), this);
        return;
    }

    const auto& MessageDefinitionCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Message_Definition_PDA>(_MessageDefinition);
    if (ck::Is_NOT_Valid(MessageDefinitionCDO->Get_MessageName()))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Definition Name", "Message Definition @@ has an Invalid Message Name. @@").ToString(), _MessageDefinition, this);
        return;
    }

    // Setup SpawnObject Node
    auto* SpawnObject_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    SpawnObject_Node->FunctionReference.SetExternalMember
    (
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject),
        UGameplayStatics::StaticClass()
    );
    SpawnObject_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(SpawnObject_Node, this);

    if (auto* ObjectClassPin = SpawnObject_Node->FindPin(TEXT("ObjectClass"));
        ck::IsValid(ObjectClassPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*ObjectClassPin, _MessageDefinition->GetClassPathName().ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*SpawnObject_Node);
    }

    // Setup BroadcastMessage Node
    auto* BroadcastMessage_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    BroadcastMessage_Node->FunctionReference.SetExternalMember
    (
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, Broadcast),
        UCk_Utils_Messaging_UE::StaticClass()
    );
    BroadcastMessage_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BroadcastMessage_Node, this);

    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition, TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageName_InputPin = BroadcastMessage_Node->FindPin(TEXT("InMessageName"));
            ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(MessageDefinitionCDO->_MessageName)>(MessageDefinitionCDO);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName_PropertyValue->ToString());
                UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BroadcastMessage_Node);
            }
        }
    }

    const auto& SpawnObjectResultPin  = UCk_Utils_EditorGraph_UE::Get_Pin_Result(*SpawnObject_Node);

    if (ck::IsValid(SpawnObjectResultPin))
    {
        (*SpawnObjectResultPin)->PinType.PinSubCategoryObject = _MessageDefinition->GetAuthoritativeClass();
    }

    // Link the 'N' SetByVar nodes that were spawned for each properties different from the CDO
    auto* LastThenFollowingSetByVarPin = FKismetCompilerUtilities::GenerateAssignmentNodes
    (
        InCompilerContext,
        InSourceGraph,
        SpawnObject_Node,
        this,
        *SpawnObjectResultPin,
        _MessageDefinition
    );

    // Setup MakeInstancedStruct Node
    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UStructUtilsFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UStructUtilsFunctionLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);

    // Setup MakeMessageDefinition Struct Node
    auto* MakeMessageDefinitionStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeMessageDefinitionStruct_Node->StructType = FCk_Message_Definition::StaticStruct();
    MakeMessageDefinitionStruct_Node->AllocateDefaultPins();
    MakeMessageDefinitionStruct_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeMessageDefinitionStruct_Node, this);

    // Connect everything together
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection
    (
        InCompilerContext,
        {
            {
                LastThenFollowingSetByVarPin,
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*MakeInstancedStruct_Node),
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*SpawnObject_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("_MessageObject"), ECk_EditorGraph_PinDirection::Input, *MakeMessageDefinitionStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(FCk_Message_Definition::StaticStruct()->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeMessageDefinitionStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Input, *MakeInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*BroadcastMessage_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("InPayload"), ECk_EditorGraph_PinDirection::Input, *BroadcastMessage_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; };

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins
    (
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                BroadcastMessage_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*SpawnObject_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*BroadcastMessage_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    BreakAllNodeLinks();
}

auto
    UCk_K2Node_Message_Broadcast::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Broadcast"),
        TEXT("[Ck][Messaging] Broadcast New Message...")
    );
}

auto
    UCk_K2Node_Message_Broadcast::
    RefreshTemplateMessageDefinition()
    -> void
{
    ReconstructNode();
    GetGraph()->NotifyGraphChanged();
}

auto
    UCk_K2Node_Message_Broadcast::
    CreatePinsFromMessageDefinition()
    -> void
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    { return; }

    const auto& MessageDefinitionCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Message_Definition_PDA>(_MessageDefinition);

    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

    for (TFieldIterator<FProperty> PropIt(_MessageDefinition, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        auto* Property = *PropIt;

        const bool IsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const bool IsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
        const bool IsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        if (Property->HasAnyPropertyFlags(CPF_Parm) ||
            NOT FBlueprintEditorUtils::PropertyStillExists(Property) ||
            NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            NOT IsExposedToSpawn ||
            NOT IsSettableExternally ||
            IsDelegate ||
            ck::IsValid(FindPin(Property->GetFName()), ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        if (auto* Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName());
            ck::IsValid(Pin, ck::IsValid_Policy_NullptrOnly{}))
        {
            K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);

            if (K2Schema->PinDefaultValueIsEditable(*Pin))
            {
                auto DefaultValueAsString = FString{};
                const auto& DefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(MessageDefinitionCDO), DefaultValueAsString, this);
                check(DefaultValueSet);

                K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
            }

            K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_Message_Listen::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
                                ? PropertyChangedEvent.Property->GetFName()
                                : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Message_Listen, _MessageDefinition))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto
    UCk_K2Node_Message_Listen::
    ShouldShowNodeProperties() const
    -> bool
{
    return true;
}

auto
    UCk_K2Node_Message_Listen::
    PreloadRequiredAssets()
    -> void
{
    Super::PreloadRequiredAssets();

    PreloadObject(_MessageDefinition);
}

auto
    UCk_K2Node_Message_Listen::
    GetNodeTitle(
        ENodeTitleType::Type TitleType) const
    -> FText
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Listen"),
            TEXT("[Ck][Messaging] Listen For New Message (INVALID Definition)")
        );
    }

    const auto& MessageDefinitionCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Message_Definition_PDA>(_MessageDefinition);
    if (ck::Is_NOT_Valid(MessageDefinitionCDO->Get_MessageName()))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Listen"),
            TEXT("[Ck][Messaging] Listen For New Message (INVALID Message Name)")
        );
    }

    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Listen"),
        *ck::Format_UE(TEXT("[Ck][Messaging] Listen For New Message ({})"), MessageDefinitionCDO->Get_MessageName())
    );
}

auto
    UCk_K2Node_Message_Listen::
    GetIconAndTint(
        FLinearColor& OutColor) const
    -> FSlateIcon
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.LatentActionIcon"));
}

auto
    UCk_K2Node_Message_Listen::
    GetNodeContextMenuActions(
        UToolMenu* Menu,
        UGraphNodeContextMenuContext* Context) const
    -> void
{
    Super::GetNodeContextMenuActions(Menu, Context);

    FToolMenuSection& Section = Menu->AddSection("CkFoundation", FText::FromString(TEXT("Messaging")));
    Section.AddMenuEntry(
        "RefreshNode",
        FText::FromString(TEXT("Refresh")),
        FText::FromString(TEXT("Refresh this node")),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateUObject(const_cast<UCk_K2Node_Message_Listen*>(this), &UCk_K2Node_Message_Listen::RefreshTemplateMessageDefinition),
            FCanExecuteAction(),
            FIsActionChecked()
        )
    );
}

auto
    UCk_K2Node_Message_Listen::
    GetMenuCategory() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Listen"),
        TEXT("Ck|Utils|Messaging")
    );
}

auto
    UCk_K2Node_Message_Listen::
    IsNodePure() const
    -> bool
{
    return false;
}

auto
    UCk_K2Node_Message_Listen::
    AutowireNewNode(
        UEdGraphPin* FromPin)
    -> void
{
    if (ck::Is_NOT_Valid(FromPin, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    Super::AutowireNewNode(FromPin);
}

auto
    UCk_K2Node_Message_Listen::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    AllocateDefaultPins();
    CreatePinsFromMessageDefinition();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_Message_Listen::
    DoAllocate_DefaultPins()
    -> void
{
    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2node_messaging::PinName_Handle
    );

    CreatePin
    (
        EGPD_Output,
        UEdGraphSchema_K2::PC_Exec,
        ck_k2node_messaging::PinName_MessageReceived
    );

    CreatePinsFromMessageDefinition();

    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Exec,
        ck_k2node_messaging::PinName_StopListening
    );
}

auto
    UCk_K2Node_Message_Listen::
    GetCornerIcon() const
    -> FName
{
    return TEXT("Graph.Latent.LatentIcon");
}

auto
    UCk_K2Node_Message_Listen::
    ReconstructNode()
    -> void
{
    Super::ReconstructNode();
    CreatePinsFromMessageDefinition();
}

auto
    UCk_K2Node_Message_Listen::
    DoExpandNode(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)
    -> void
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Missing Message Definition", "Invalid Message Definition. @@").ToString(), this);
        return;
    }

    const auto& MessageDefinitionCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Message_Definition_PDA>(_MessageDefinition);
    if (ck::Is_NOT_Valid(MessageDefinitionCDO->Get_MessageName()))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Definition Name", "Message Definition @@ has an Invalid Message Name. @@").ToString(), _MessageDefinition, this);
        return;
    }

    // Create the custom event node
    auto* CustomEventNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, InSourceGraph);
    CustomEventNode->CustomFunctionName = *InCompilerContext.GetGuid(CustomEventNode);
    CustomEventNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CustomEventNode, this);

    // Set up pins on the custom event node to match the delegate signature

    // Add pin for `Handle` parameter (assuming it's an FCk_Handle type)
    FEdGraphPinType HandlePinType;
    HandlePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    HandlePinType.PinSubCategoryObject = FCk_Handle::StaticStruct();
    auto* OutHandlePin = CustomEventNode->CreateUserDefinedPin(TEXT("OutHandle"), HandlePinType, EGPD_Output);

    // Add pin for `MessageName` parameter (assuming it's an FGameplayTag type)
    FEdGraphPinType MessageNamePinType;
    MessageNamePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    MessageNamePinType.PinSubCategoryObject = FGameplayTag::StaticStruct();
    auto* OutMessageNamePin = CustomEventNode->CreateUserDefinedPin(TEXT("OutMessageName"), MessageNamePinType, EGPD_Output);

    // Add pin for `Payload` parameter (assuming it's an FInstancedStruct type)
    FEdGraphPinType PayloadPinType;
    PayloadPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    PayloadPinType.PinSubCategoryObject = FInstancedStruct::StaticStruct();
    auto* PayloadPin = CustomEventNode->CreateUserDefinedPin(TEXT("OutPayload"), PayloadPinType, EGPD_Output);

    // Set up the `BindTo_OnBroadcast` function node
    auto* BindFunction_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    BindFunction_Node->FunctionReference.SetExternalMember
    (
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, BindTo_OnBroadcast),
        UCk_Utils_Messaging_UE::StaticClass()
    );
    BindFunction_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BindFunction_Node, this);

    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition, TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageName_InputPin = BindFunction_Node->FindPin(TEXT("InMessageName"));
            ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(MessageDefinitionCDO->_MessageName)>(MessageDefinitionCDO);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName_PropertyValue->ToString());
                UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BindFunction_Node);
            }
        }
    }

    // Set up the `UnbindFrom_OnBroadcast` function node
    auto* UnbindFunction_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    UnbindFunction_Node->FunctionReference.SetExternalMember
    (
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, UnbindFrom_OnBroadcast),
        UCk_Utils_Messaging_UE::StaticClass()
    );
    UnbindFunction_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(UnbindFunction_Node, this);

    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition, TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageNameInputPin = UnbindFunction_Node->FindPin(TEXT("InMessageName"));
            ck::IsValid(MessageNameInputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(MessageDefinitionCDO->_MessageName)>(MessageDefinitionCDO);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageNameInputPin, MessageName_PropertyValue->ToString());
                UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*UnbindFunction_Node);
            }
        }
    }

    // Setup TriggerEnsure Node
    auto* TriggerEnsure_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    TriggerEnsure_Node->FunctionReference.SetExternalMember
    (
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Ensure_UE, TriggerEnsure),
        UCk_Utils_Ensure_UE::StaticClass()
    );
    TriggerEnsure_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(TriggerEnsure_Node, this);

    if (auto* Message_Pin = TriggerEnsure_Node->FindPin(TEXT("InMsg"));
        ck::IsValid(Message_Pin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*Message_Pin, ck::Format_UE(TEXT("Received a Message that was NOT of the expected type [{}]"), _MessageDefinition));
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*TriggerEnsure_Node);
    }

    // Setup GetInstancedStructValue Node
    auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetInstancedStruct_Node->SetFromFunction(UStructUtilsFunctionLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UStructUtilsFunctionLibrary, GetInstancedStructValue)));
    GetInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);

    // Setup BreakMessageDefinition Struct Node
    auto* BreakMessageDefinitionStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(this, InSourceGraph);
    BreakMessageDefinitionStruct_Node->StructType = FCk_Message_Definition::StaticStruct();
    BreakMessageDefinitionStruct_Node->AllocateDefaultPins();
    BreakMessageDefinitionStruct_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BreakMessageDefinitionStruct_Node, this);

    // Add a cast node so we can call the getter function with a pin of the right class
    auto* CastNode = InCompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, InSourceGraph);
    CastNode->TargetType = _MessageDefinition;
    CastNode->SetPurity(false);
    CastNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CastNode, this);
    const auto& CastResultPinName = UEdGraphSchema_K2::PN_CastedValuePrefix + _MessageDefinition->GetDisplayNameText().ToString();

    const auto& IsOutputPinPredicate = [](const UEdGraphPin* InGraphPin) -> bool
    {
        return InGraphPin->Direction == EGPD_Output;
    };

    UCk_Utils_EditorGraph_UE::ForEach_NodePins_If(*this, IsOutputPinPredicate, [&](const UEdGraphPin* InGraphPin)
    {
        if (InGraphPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        { return; }

        const auto& GraphPinName = InGraphPin->PinName;

        auto* GetVariableNode = InCompilerContext.SpawnIntermediateNode<UK2Node_VariableGet>(CastNode, InSourceGraph);
        constexpr auto IsConsideredSelfContext = false;
        GetVariableNode->VariableReference.SetFromField<FProperty>(CastNode->TargetType->FindPropertyByName(GraphPinName), IsConsideredSelfContext, CastNode->TargetType);
        GetVariableNode->AllocateDefaultPins();

        UCk_Utils_EditorGraph_UE::Request_TryCreateConnection
        (
            InCompilerContext,
            {
                {
                    UCk_Utils_EditorGraph_UE::Get_Pin_Self(*GetVariableNode),
                    UCk_Utils_EditorGraph_UE::Get_Pin(*CastResultPinName, ECk_EditorGraph_PinDirection::Output, *CastNode),
                },
            }
        );

        UCk_Utils_EditorGraph_UE::Request_LinkPins
        (
            InCompilerContext,
            {
                {
                    UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Output, *this),
                    UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Output, *GetVariableNode),
                },
            },
            ECk_EditorGraph_PinLinkType::Move
        );
    });

    // Connect everything together
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection
    (
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*CustomEventNode),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetInstancedStruct_Node),
            },
            {
                PayloadPin,
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("InstancedStruct"), ECk_EditorGraph_PinDirection::Input, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(FCk_Message_Definition::StaticStruct()->GetFName(), ECk_EditorGraph_PinDirection::Input, *BreakMessageDefinitionStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("_MessageObject"), ECk_EditorGraph_PinDirection::Output, *BreakMessageDefinitionStruct_Node),
                CastNode->GetCastSourcePin()
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*CastNode),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Valid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*TriggerEnsure_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(UEdGraphSchema_K2::PN_CastFailed, ECk_EditorGraph_PinDirection::Output, *CastNode)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_OutputDelegate(*CustomEventNode),
                BindFunction_Node->FindPin(TEXT("InDelegate"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_OutputDelegate(*CustomEventNode),
                UnbindFunction_Node->FindPin(TEXT("InDelegate"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; };

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins
    (
        InCompilerContext,
        {
            /*{
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                BindFunction_Node->FindPin(TEXT("InHandle"))
            },*/
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*BindFunction_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*BindFunction_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_MessageReceived, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*CastNode)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_StopListening, ECk_EditorGraph_PinDirection::Input, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*UnbindFunction_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins
    (
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                BindFunction_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                UnbindFunction_Node->FindPin(TEXT("InHandle"))
            },
        },
        ECk_EditorGraph_PinLinkType::Copy
    ) == ECk_SucceededFailed::Failed) { return; }

    BreakAllNodeLinks();
}

auto
    UCk_K2Node_Message_Listen::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Listen"),
        TEXT("[Ck][Messaging] Listen For New Message...")
    );
}

auto
    UCk_K2Node_Message_Listen::
    RefreshTemplateMessageDefinition()
    -> void
{
    ReconstructNode();
    GetGraph()->NotifyGraphChanged();
}

bool UCk_K2Node_Message_Listen::IsCompatibleWithGraph(UEdGraph const* InGraph) const
{
    if (ck::Is_NOT_Valid(InGraph))
    { return Super::IsCompatibleWithGraph(InGraph); }


    const auto& OuterBlueprint = InGraph->GetTypedOuter<UBlueprint>();
    if (ck::Is_NOT_Valid(OuterBlueprint))
    { return Super::IsCompatibleWithGraph(InGraph); }

    return OuterBlueprint->UbergraphPages.Contains(InGraph);
}

auto
    UCk_K2Node_Message_Listen::
    CreatePinsFromMessageDefinition()
    -> void
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    { return; }

    const auto& MessageDefinitionCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Message_Definition_PDA>(_MessageDefinition);

    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

    for (TFieldIterator<FProperty> PropIt(_MessageDefinition, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        auto* Property = *PropIt;

        const bool IsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const bool IsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
        const bool IsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        if (Property->HasAnyPropertyFlags(CPF_Parm) ||
            NOT FBlueprintEditorUtils::PropertyStillExists(Property) ||
            NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            NOT IsExposedToSpawn ||
            NOT IsSettableExternally ||
            IsDelegate ||
            ck::IsValid(FindPin(Property->GetFName()), ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        if (auto* Pin = CreatePin(EGPD_Output, NAME_None, Property->GetFName());
            ck::IsValid(Pin, ck::IsValid_Policy_NullptrOnly{}))
        {
            K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);

            if (K2Schema->PinDefaultValueIsEditable(*Pin))
            {
                auto DefaultValueAsString = FString{};
                const auto& DefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(MessageDefinitionCDO), DefaultValueAsString, this);
                check(DefaultValueSet);

                K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
            }

            K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE