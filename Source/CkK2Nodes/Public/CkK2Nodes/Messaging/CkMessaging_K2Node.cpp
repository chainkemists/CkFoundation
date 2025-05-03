#include "CkMessaging_K2Node.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkMessaging/Public/CkMessaging/CkMessaging_Utils.h"

#include <GraphEditorSettings.h>
#include <K2Node_MakeStruct.h>
#include <K2Node_BreakStruct.h>
#include <K2Node_CustomEvent.h>
#include <KismetCompiler.h>
#include <EdGraphSchema_K2.h>
#include <BlueprintNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Kismet/BlueprintInstancedStructLibrary.h>

#define LOCTEXT_NAMESPACE "K2Node_Messaging"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2node_messaging
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_HandleToUnbind = TEXT("HandleToUnbind");
    static auto PinName_OutHandle = TEXT("OutHandle");
    static auto PinName_OutListener = TEXT("OutMessageListener");
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
    UCk_K2Node_Message_Base::
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
    UCk_K2Node_Message_Base::
    ShouldShowNodeProperties() const
    -> bool
{
    return true;
}

auto
    UCk_K2Node_Message_Base::
    PreloadRequiredAssets()
    -> void
{
    Super::PreloadRequiredAssets();

    PreloadObject(_MessageDefinition);
}

auto
    UCk_K2Node_Message_Base::
    IsNodePure() const
    -> bool
{
    return false;
}

auto
    UCk_K2Node_Message_Base::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_Message_Base::
    GetJumpTargetForDoubleClick() const
    -> UObject*
{
    return _MessageDefinition;
}

auto
    UCk_K2Node_Message_Base::
    JumpToDefinition() const
    -> void
{
    if (ck::IsValid(_MessageDefinition))
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(_MessageDefinition);
    }
}

auto
    UCk_K2Node_Message_Base::
    CreatePinsFromMessageDefinition()
    -> void
{
    if (ck::Is_NOT_Valid(_MessageDefinition))
    { return; }

    const auto& MessagePayload = _MessageDefinition->Get_MessagePayload();

    if (ck::Is_NOT_Valid(MessagePayload))
    { return; }

    const auto& CreatePinFromProperty = [this](const FProperty* InProperty, const uint8* InContainer)
    {
        const auto& PinDirection = UCk_Utils_EditorGraph_UE::Get_PinDirection_ToUnreal(DoGet_MessageDefinitionPinsDirection());
        auto* Pin = CreatePin(PinDirection, NAME_None, InProperty->GetFName());

        if (ck::Is_NOT_Valid(Pin, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        Pin->PinFriendlyName = InProperty->GetDisplayNameText();
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

        K2Schema->ConvertPropertyToPinType(InProperty, Pin->PinType);

        if (K2Schema->PinDefaultValueIsEditable(*Pin))
        {
            auto DefaultValueAsString = FString{};
            const auto& DefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(InProperty, InContainer, DefaultValueAsString, this);
            check(DefaultValueSet);

            K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
        }

        K2Schema->ConstructBasicPinTooltip(*Pin, InProperty->GetToolTipText(), Pin->PinToolTip);
    };

    auto* StructData = MessagePayload.GetMemory();
    auto* StructType = MessagePayload.GetScriptStruct();

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        auto* Property = *It;

        if (Property->HasAnyPropertyFlags(CPF_Parm) ||
            NOT FBlueprintEditorUtils::PropertyStillExists(Property) ||
            NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            ck::IsValid(FindPin(Property->GetFName()), ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        CreatePinFromProperty(Property, StructData);
    }
}

// --------------------------------------------------------------------------------------------------------------------

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
            TEXT("[Ck][Messaging] Broadcast New Message\n(INVALID Message Definition)")
        );
    }

    const auto& MessageName = _MessageDefinition->Get_MessageName();
    if (ck::Is_NOT_Valid(_MessageDefinition->Get_MessageName()))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Broadcast"),
            TEXT("[Ck][Messaging] Broadcast New Message\n(INVALID Message Name)")
        );
    }

    if (ck::Is_NOT_Valid(_MessageDefinition->Get_MessagePayload()))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Broadcast"),
            TEXT("[Ck][Messaging] Broadcast New Message\n(INVALID Message Payload)")
        );
    }

    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Broadcast"),
        *ck::Format_UE(TEXT("[Ck][Messaging] Broadcast New Message\n({})"), MessageName)
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
    DoAllocate_DefaultPins()
    -> void
{
    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;

    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2node_messaging::PinName_Handle,
        HandlePinParams
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

    const auto& MessageName = _MessageDefinition->Get_MessageName();
    if (ck::Is_NOT_Valid(MessageName))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Definition Name", "Message Definition @@ has an Invalid Message Name. @@").ToString(), _MessageDefinition, this);
        return;
    }

    const auto& MessagePayload = _MessageDefinition->Get_MessagePayload();
    if (ck::Is_NOT_Valid(MessagePayload))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Definition Payload", "Message Definition @@ has an Invalid Message Payload. @@").ToString(), _MessageDefinition, this);
        return;
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

    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition->GetClass(), TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageName_InputPin = BroadcastMessage_Node->FindPin(TEXT("InMessageName"));
            ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(_MessageDefinition->_MessageName)>(_MessageDefinition);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName_PropertyValue->ToString());
                UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BroadcastMessage_Node);
            }
        }
    }

    // Setup MakeInstancedStruct Node
    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);

    // Setup MessagePayload MakeStruct Node
    auto* MakeMessageDefinition_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    auto ScriptStruct = const_cast<UScriptStruct*>(_MessageDefinition->Get_MessagePayload().GetScriptStruct());
    MakeMessageDefinition_Node->StructType = ScriptStruct;
    MakeMessageDefinition_Node->AllocateDefaultPins();
    MakeMessageDefinition_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeMessageDefinition_Node, this);

    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*MakeMessageDefinition_Node, [&](UEdGraphPin* InGraphPin)
    {
        if (InGraphPin->Direction != EGPD_Input)
        { return; }

        const auto& GraphPinName = InGraphPin->PinName;

        if (const auto& ThisNodeMatchingInputPin = UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Input, *this);
            ck::IsValid(ThisNodeMatchingInputPin))
        {
            if ((*ThisNodeMatchingInputPin)->LinkedTo.IsEmpty())
            {
                UCk_Utils_EditorGraph_UE::Request_CopyPinValues
                (
                    InCompilerContext,
                    {
                        { ThisNodeMatchingInputPin, InGraphPin, },
                    }
                );

                return;
            }

            UCk_Utils_EditorGraph_UE::Request_LinkPins
            (
                InCompilerContext,
                {
                    { ThisNodeMatchingInputPin, InGraphPin, },
                },
                ECk_EditorGraph_PinLinkType::Move
            );
        }
    });

    // Connect everything together
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection
    (
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ScriptStruct->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeMessageDefinition_Node),
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
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*MakeInstancedStruct_Node)
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
    DoGet_MessageDefinitionPinsDirection() const
    -> ECk_EditorGraph_PinDirection
{
    return ECk_EditorGraph_PinDirection::Input;
}

// --------------------------------------------------------------------------------------------------------------------

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
            TEXT("[Ck][Messaging] Listen For New Message\n(INVALID Message Definition)")
        );
    }

    const auto& MessageName = _MessageDefinition->Get_MessageName();
    if (ck::Is_NOT_Valid(MessageName))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Listen"),
            TEXT("[Ck][Messaging] Listen For New Message\n(INVALID Message Name)")
        );
    }

    if (ck::Is_NOT_Valid(_MessageDefinition->Get_MessagePayload()))
    {
        return CK_UTILS_IO_GET_LOCTEXT
        (
            TEXT("UCk_K2Node_Message_Listen"),
            TEXT("[Ck][Messaging] Listen For New Message\n(INVALID Message Payload)")
        );
    }

    return CK_UTILS_IO_GET_LOCTEXT
    (
        TEXT("UCk_K2Node_Message_Listen"),
        *ck::Format_UE(TEXT("[Ck][Messaging] Listen For New Message\n({})"), MessageName)
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
    DoAllocate_DefaultPins()
    -> void
{
    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;

    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2node_messaging::PinName_Handle,
        HandlePinParams
    );

    CreatePin
    (
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle_MessageListener::StaticStruct(),
        ck_k2node_messaging::PinName_OutListener
    );

    CreatePin
    (
        EGPD_Output,
        UEdGraphSchema_K2::PC_Exec,
        ck_k2node_messaging::PinName_MessageReceived
    );

    CreatePin
    (
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2node_messaging::PinName_OutHandle
    );

    CreatePinsFromMessageDefinition();

    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Exec,
        ck_k2node_messaging::PinName_StopListening
    );

    CreatePin
    (
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2node_messaging::PinName_HandleToUnbind,
        HandlePinParams
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

    if (const auto& MessageName = _MessageDefinition->Get_MessageName();
        ck::Is_NOT_Valid(MessageName))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Definition Name", "Message Definition @@ has an Invalid Message Name. @@").ToString(), _MessageDefinition, this);
        return;
    }

    if (const auto& MessagePayload = _MessageDefinition->Get_MessagePayload();
        ck::Is_NOT_Valid(MessagePayload))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Definition Payload", "Message Definition @@ has an Invalid Message Payload. @@").ToString(), _MessageDefinition, this);
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

    // Copy MessageName value to the  Bind node's message name property
    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition->GetClass(), TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageName_InputPin = BindFunction_Node->FindPin(TEXT("InMessageName"));
            ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(_MessageDefinition->_MessageName)>(_MessageDefinition);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName_PropertyValue->ToString());
                UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BindFunction_Node);
            }
        }
    }

    // Setup FCk_Handle_MessageListener MakeStruct Node
    auto* MakeMessageListener_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeMessageListener_Node->StructType = FCk_Handle_MessageListener::StaticStruct();
    MakeMessageListener_Node->AllocateDefaultPins();
    MakeMessageListener_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeMessageListener_Node, this);

    // Copy MessageName value to the  MakeListener node's message name property
    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition->GetClass(), TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageName_InputPin = MakeMessageListener_Node->FindPin(TEXT("_MessageName"));
            ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(_MessageDefinition->_MessageName)>(_MessageDefinition);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName_PropertyValue->ToString());
                UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*MakeMessageListener_Node);
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

    if (const auto* MessageName_Property = FindFProperty<FProperty>(_MessageDefinition->GetClass(), TEXT("_MessageName"));
        ck::IsValid(MessageName_Property))
    {
        if (auto* MessageName_InputPin = UnbindFunction_Node->FindPin(TEXT("InMessageName"));
            ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto MessageName_PropertyValue = MessageName_Property->ContainerPtrToValuePtr<decltype(_MessageDefinition->_MessageName)>(_MessageDefinition);
                ck::IsValid(MessageName_PropertyValue, ck::IsValid_Policy_NullptrOnly{}))
            {
                InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName_PropertyValue->ToString());
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
        InCompilerContext.GetSchema()->TrySetDefaultValue(*Message_Pin, ck::Format_UE(TEXT("Received a Message Payload that was NOT of the expected type [{}]"), _MessageDefinition));
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*TriggerEnsure_Node);
    }

    // Setup GetInstancedStructValue Node
    auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)));
    GetInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);

    // Setup MessagePayload BreakStruct Node
    auto* BreakMessageDefinition_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(this, InSourceGraph);
    auto ScriptStruct = const_cast<UScriptStruct*>(_MessageDefinition->Get_MessagePayload().GetScriptStruct());
    BreakMessageDefinition_Node->StructType = ScriptStruct;
    BreakMessageDefinition_Node->AllocateDefaultPins();
    BreakMessageDefinition_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BreakMessageDefinition_Node, this);

    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*BreakMessageDefinition_Node, [&](UEdGraphPin* InGraphPin)
    {
        if (InGraphPin->Direction != EGPD_Output)
        { return; }

        const auto& GraphPinName = InGraphPin->PinName;

        if (const auto& ThisNodeMatchingOutputPin = UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Output, *this);
            ck::IsValid(ThisNodeMatchingOutputPin))
        {
            UCk_Utils_EditorGraph_UE::Request_LinkPins
            (
                InCompilerContext,
                {
                    { ThisNodeMatchingOutputPin, InGraphPin, },
                },
                ECk_EditorGraph_PinLinkType::Move
            );
        }
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
                UCk_Utils_EditorGraph_UE::Get_Pin(ScriptStruct->GetFName(), ECk_EditorGraph_PinDirection::Input, *BreakMessageDefinition_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*TriggerEnsure_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("NotValid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_OutputDelegate(*CustomEventNode),
                BindFunction_Node->FindPin(TEXT("InDelegate"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_OutputDelegate(*CustomEventNode),
                UnbindFunction_Node->FindPin(TEXT("InDelegate"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_OutputDelegate(*CustomEventNode),
                MakeMessageListener_Node->FindPin(TEXT("_MessageDelegate"))
            },
        }
    ) == ECk_SucceededFailed::Failed) { return; };

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins
    (
        InCompilerContext,
        {
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
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Valid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_StopListening, ECk_EditorGraph_PinDirection::Input, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*UnbindFunction_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_OutHandle, ECk_EditorGraph_PinDirection::Output, *this),
                OutHandlePin
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_OutListener, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(MakeMessageListener_Node->StructType->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeMessageListener_Node)
            },
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
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_HandleToUnbind, ECk_EditorGraph_PinDirection::Input, *this),
                UnbindFunction_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                MakeMessageListener_Node->FindPin(TEXT("_MessageListener"))
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
    DoGet_MessageDefinitionPinsDirection() const
    -> ECk_EditorGraph_PinDirection
{
    return ECk_EditorGraph_PinDirection::Output;
}

auto
    UCk_K2Node_Message_Listen::
    IsCompatibleWithGraph(
        UEdGraph const* InGraph) const
    -> bool
{
    if (ck::Is_NOT_Valid(InGraph))
    { return Super::IsCompatibleWithGraph(InGraph); }

    const auto& OuterBlueprint = InGraph->GetTypedOuter<UBlueprint>();
    if (ck::Is_NOT_Valid(OuterBlueprint))
    { return Super::IsCompatibleWithGraph(InGraph); }

    return OuterBlueprint->UbergraphPages.Contains(InGraph);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE