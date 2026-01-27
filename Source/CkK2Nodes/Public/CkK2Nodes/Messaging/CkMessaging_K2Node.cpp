#include "CkMessaging_K2Node.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkMessaging/Public/CkMessaging/CkMessaging_Utils.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <GraphEditorSettings.h>
#include <K2Node_BreakStruct.h>
#include <K2Node_CustomEvent.h>
#include <K2Node_MakeStruct.h>
#include <KismetCompiler.h>

#include <Kismet/BlueprintInstancedStructLibrary.h>

#include <Kismet2/BlueprintEditorUtils.h>

#include <Subsystems/AssetEditorSubsystem.h>

#define LOCTEXT_NAMESPACE "K2Node_Messaging"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2_node_messaging
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_HandleToUnbind = TEXT("HandleToUnbind");
    static auto PinName_OutHandle = TEXT("OutHandle");
    static auto PinName_OutListener = TEXT("OutMessageListener");
    static auto PinName_MessageReceived = TEXT("MessageReceived");
    static auto PinName_StopListening = TEXT("StopListening");
    static auto PinName_CompactPayload = TEXT("Payload");
    static auto PinName_CompactPayloadOut = TEXT("OutPayload");

    static const auto BroadcastNodeColor = FLinearColor(1.0f, 0.5f, 0.0f);
    static const auto ListenNodeColor = FLinearColor(0.0f, 0.7f, 0.7f);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_K2Node_Message_Base::GetMessageNameFromStruct() const -> FGameplayTag
{
    if (ck::Is_NOT_Valid(_MessagePayload))
    {
        return FGameplayTag{};
    }

    const auto* StructType = _MessagePayload.GetScriptStruct();
    if (ck::Is_NOT_Valid(StructType))
    {
        return FGameplayTag{};
    }

    const auto& StructName = StructType->GetName();
    const auto& TagString = ck::Format_UE(TEXT("Message.{}"), StructName);

    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(*TagString, TEXT("Added via the Messaging Node"));
}

auto UCk_K2Node_Message_Base::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Message_Base, _MessagePayload) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Message_Base, _PayloadMode))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto UCk_K2Node_Message_Base::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCk_K2Node_Message_Base::PreloadRequiredAssets() -> void
{
    Super::PreloadRequiredAssets();
}

auto UCk_K2Node_Message_Base::IsNodePure() const -> bool
{
    return false;
}

auto UCk_K2Node_Message_Base::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto UCk_K2Node_Message_Base::GetJumpTargetForDoubleClick() const -> UObject*
{
    return nullptr;
}

auto UCk_K2Node_Message_Base::JumpToDefinition() const -> void
{
}

auto UCk_K2Node_Message_Base::CreatePinsFromMessageDefinition() -> void
{
    if (ck::Is_NOT_Valid(_MessagePayload))
    {
        return;
    }

    _PinsGeneratedFromPayload.Reset();

    if (IsCompactPayloadMode())
    {
        const auto* StructType = _MessagePayload.GetScriptStruct();
        if (ck::Is_NOT_Valid(StructType))
        {
            return;
        }

        const auto& PinDirection = UCk_Utils_EditorGraph_UE::Get_PinDirection_ToUnreal(DoGet_MessageDefinitionPinsDirection());
        const auto& PinName = (PinDirection == EGPD_Input)
            ? ck_k2_node_messaging::PinName_CompactPayload
            : ck_k2_node_messaging::PinName_CompactPayloadOut;

        auto* Pin = CreatePin(PinDirection, UEdGraphSchema_K2::PC_Struct, const_cast<UScriptStruct*>(StructType), PinName);

        if (ck::IsValid(Pin, ck::IsValid_Policy_NullptrOnly{}))
        {
            Pin->PinFriendlyName = FText::FromString(TEXT("Payload"));
            Pin->PinToolTip = ck::Format_UE(TEXT("The message payload of type {}"), StructType->GetName());
            _PinsGeneratedFromPayload.Add(Pin);
        }

        return;
    }

    const auto& CreatePinFromProperty = [this](const FProperty* InProperty, const uint8* InContainer)
        {
            const auto& PinDirection = UCk_Utils_EditorGraph_UE::Get_PinDirection_ToUnreal(DoGet_MessageDefinitionPinsDirection());
            auto* Pin = CreatePin(PinDirection, NAME_None, InProperty->GetFName());

            if (ck::Is_NOT_Valid(Pin, ck::IsValid_Policy_NullptrOnly{}))
            {
                return;
            }

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
            _PinsGeneratedFromPayload.Add(Pin);
        };

    auto* StructData = _MessagePayload.GetMemory();
    auto* StructType = _MessagePayload.GetScriptStruct();

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        auto* Property = *It;

        if (Property->HasAnyPropertyFlags(CPF_Parm) ||
            NOT FBlueprintEditorUtils::PropertyStillExists(Property) ||
            NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            ck::IsValid(FindPin(Property->GetFName()), ck::IsValid_Policy_NullptrOnly{}))
        {
            continue;
        }

        CreatePinFromProperty(Property, StructData);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// UCk_K2Node_Message_Broadcast
// --------------------------------------------------------------------------------------------------------------------

auto UCk_K2Node_Message_Broadcast::GetNodeTitle(ENodeTitleType::Type TitleType) const -> FText
{
    if (ck::Is_NOT_Valid(_MessagePayload))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCk_K2Node_Message_Broadcast"),
            TEXT("[Ck] Broadcast Message 📢\n(INVALID Message Payload)"));
    }

    const auto& MessageName = GetMessageNameFromStruct();
    if (ck::Is_NOT_Valid(MessageName))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCk_K2Node_Message_Broadcast"),
            TEXT("[Ck] Broadcast Message 📢\n(INVALID Struct Type)"));
    }

    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCk_K2Node_Message_Broadcast"),
        *ck::Format_UE(TEXT("[Ck] Broadcast Message 📢\n({})"), MessageName));
}

auto UCk_K2Node_Message_Broadcast::GetNodeTitleColor() const -> FLinearColor
{
    return ck_k2_node_messaging::BroadcastNodeColor;
}

auto UCk_K2Node_Message_Broadcast::GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = ck_k2_node_messaging::BroadcastNodeColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

auto UCk_K2Node_Message_Broadcast::CreateVisualWidget() -> TSharedPtr<SGraphNode>
{
    return SNew(SCk_GraphNode_Message_Broadcast, this);
}

auto UCk_K2Node_Message_Broadcast::GetMenuCategory() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Message_Broadcast"), TEXT("Ck|Utils|Messaging"));
}

auto UCk_K2Node_Message_Broadcast::DoAllocate_DefaultPins() -> void
{
    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;

    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_messaging::PinName_Handle,
        HandlePinParams);

    CreatePinsFromMessageDefinition();
}

auto UCk_K2Node_Message_Broadcast::DoExpandNode(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    ECk_ValidInvalid InNodeValidity) -> void
{
    if (ck::Is_NOT_Valid(_MessagePayload))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Payload", "Invalid Message Payload. @@").ToString(), this);
        return;
    }

    const auto& MessageName = GetMessageNameFromStruct();
    if (ck::Is_NOT_Valid(MessageName))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Struct Type", "Invalid Struct Type. @@").ToString(), this);
        return;
    }

    if (IsCompactPayloadMode())
    {
        DoExpandNode_Compact(InCompilerContext, InSourceGraph);
    }
    else
    {
        DoExpandNode_Expanded(InCompilerContext, InSourceGraph);
    }
}

auto UCk_K2Node_Message_Broadcast::DoExpandNode_Expanded(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph) -> void
{
    const auto& MessageName = GetMessageNameFromStruct();

    auto* BroadcastMessage_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    BroadcastMessage_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, Broadcast),
        UCk_Utils_Messaging_UE::StaticClass());
    BroadcastMessage_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BroadcastMessage_Node, this);

    if (auto* MessageName_InputPin = BroadcastMessage_Node->FindPin(TEXT("InMessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BroadcastMessage_Node);
    }

    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);

    auto* MakeMessageDefinition_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    auto ScriptStruct = const_cast<UScriptStruct*>(_MessagePayload.GetScriptStruct());
    MakeMessageDefinition_Node->StructType = ScriptStruct;
    MakeMessageDefinition_Node->AllocateDefaultPins();
    MakeMessageDefinition_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeMessageDefinition_Node, this);

    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*MakeMessageDefinition_Node, [&](UEdGraphPin* InGraphPin)
        {
            if (InGraphPin->Direction != EGPD_Input)
            {
                return;
            }

            const auto& GraphPinName = InGraphPin->PinName;

            if (const auto& ThisNodeMatchingInputPin = UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Input, *this);
                ck::IsValid(ThisNodeMatchingInputPin))
            {
                if ((*ThisNodeMatchingInputPin)->LinkedTo.IsEmpty())
                {
                    UCk_Utils_EditorGraph_UE::Request_CopyPinValues(
                        InCompilerContext,
                        { { ThisNodeMatchingInputPin, InGraphPin } });
                    return;
                }

                UCk_Utils_EditorGraph_UE::Request_LinkPins(
                    InCompilerContext,
                    { { ThisNodeMatchingInputPin, InGraphPin } },
                    ECk_EditorGraph_PinLinkType::Move);
            }
        });

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
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
        }) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
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
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    BreakAllNodeLinks();
}

auto UCk_K2Node_Message_Broadcast::DoExpandNode_Compact(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph) -> void
{
    const auto& MessageName = GetMessageNameFromStruct();
    auto* ScriptStruct = const_cast<UScriptStruct*>(_MessagePayload.GetScriptStruct());

    auto* BroadcastMessage_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    BroadcastMessage_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, Broadcast),
        UCk_Utils_Messaging_UE::StaticClass());
    BroadcastMessage_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BroadcastMessage_Node, this);

    if (auto* MessageName_InputPin = BroadcastMessage_Node->FindPin(TEXT("InMessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BroadcastMessage_Node);
    }

    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);

    auto* ValuePin = MakeInstancedStruct_Node->FindPin(TEXT("Value"));
    if (ck::IsValid(ValuePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        ValuePin->PinType.PinSubCategoryObject = ScriptStruct;
    }

    auto* PayloadPin = FindPin(ck_k2_node_messaging::PinName_CompactPayload);
    if (ck::Is_NOT_Valid(PayloadPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.MessageLog.Error(*FText::FromString(TEXT("Could not find Payload pin")).ToString(), this);
        return;
    }

    if (PayloadPin->LinkedTo.Num() > 0 && ck::IsValid(ValuePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.MovePinLinksToIntermediate(*PayloadPin, *ValuePin);
    }
    else
    {
        InCompilerContext.MessageLog.Error(*FText::FromString(TEXT("Payload pin must be connected in Compact mode")).ToString(), this);
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*BroadcastMessage_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("InPayload"), ECk_EditorGraph_PinDirection::Input, *BroadcastMessage_Node)
            }
        }) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
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
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    BreakAllNodeLinks();
}

auto UCk_K2Node_Message_Broadcast::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Message_Broadcast"), TEXT("[Ck] Broadcast Message 📢..."));
}

auto UCk_K2Node_Message_Broadcast::DoGet_MessageDefinitionPinsDirection() const -> ECk_EditorGraph_PinDirection
{
    return ECk_EditorGraph_PinDirection::Input;
}

// --------------------------------------------------------------------------------------------------------------------
// UCk_K2Node_Message_Listen
// --------------------------------------------------------------------------------------------------------------------

auto UCk_K2Node_Message_Listen::GetNodeTitle(ENodeTitleType::Type TitleType) const -> FText
{
    if (ck::Is_NOT_Valid(_MessagePayload))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCk_K2Node_Message_Listen"),
            TEXT("[Ck] Listen For Message 👂\n(INVALID Message Payload)"));
    }

    const auto& MessageName = GetMessageNameFromStruct();
    if (ck::Is_NOT_Valid(MessageName))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCk_K2Node_Message_Listen"),
            TEXT("[Ck] Listen For Message 👂\n(INVALID Struct Type)"));
    }

    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCk_K2Node_Message_Listen"),
        *ck::Format_UE(TEXT("[Ck] Listen For Message 👂\n({})"), MessageName));
}

auto UCk_K2Node_Message_Listen::GetNodeTitleColor() const -> FLinearColor
{
    return ck_k2_node_messaging::ListenNodeColor;
}

auto UCk_K2Node_Message_Listen::GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = ck_k2_node_messaging::ListenNodeColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.LatentActionIcon"));
}

auto UCk_K2Node_Message_Listen::CreateVisualWidget() -> TSharedPtr<SGraphNode>
{
    return SNew(SCk_GraphNode_Message_Listen, this);
}

auto UCk_K2Node_Message_Listen::GetMenuCategory() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Message_Listen"), TEXT("Ck|Utils|Messaging"));
}

auto UCk_K2Node_Message_Listen::DoAllocate_DefaultPins() -> void
{
    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;

    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_messaging::PinName_Handle,
        HandlePinParams);

    CreatePin(
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle_MessageListener::StaticStruct(),
        ck_k2_node_messaging::PinName_OutListener);

    CreatePin(
        EGPD_Output,
        UEdGraphSchema_K2::PC_Exec,
        ck_k2_node_messaging::PinName_MessageReceived);

    CreatePin(
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_messaging::PinName_OutHandle);

    CreatePinsFromMessageDefinition();

    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Exec,
        ck_k2_node_messaging::PinName_StopListening);

    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_messaging::PinName_HandleToUnbind,
        HandlePinParams);
}

auto UCk_K2Node_Message_Listen::GetCornerIcon() const -> FName
{
    return TEXT("Graph.Latent.LatentIcon");
}

auto UCk_K2Node_Message_Listen::DoExpandNode(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    ECk_ValidInvalid InNodeValidity) -> void
{
    if (ck::Is_NOT_Valid(_MessagePayload))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Message Payload", "Invalid Message Payload. @@").ToString(), this);
        return;
    }

    const auto& MessageName = GetMessageNameFromStruct();
    if (ck::Is_NOT_Valid(MessageName))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Struct Type", "Invalid Struct Type. @@").ToString(), this);
        return;
    }

    if (IsCompactPayloadMode())
    {
        DoExpandNode_Compact(InCompilerContext, InSourceGraph);
    }
    else
    {
        DoExpandNode_Expanded(InCompilerContext, InSourceGraph);
    }
}

auto UCk_K2Node_Message_Listen::DoExpandNode_Expanded(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph) -> void
{
    const auto& MessageName = GetMessageNameFromStruct();

    auto* CustomEventNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, InSourceGraph);
    CustomEventNode->CustomFunctionName = *InCompilerContext.GetGuid(CustomEventNode);
    CustomEventNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CustomEventNode, this);

    FEdGraphPinType HandlePinType;
    HandlePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    HandlePinType.PinSubCategoryObject = FCk_Handle::StaticStruct();
    auto* HandlePin = CustomEventNode->CreateUserDefinedPin(TEXT("OutHandle"), HandlePinType, EGPD_Output);

    FEdGraphPinType MessageNamePinType;
    MessageNamePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    MessageNamePinType.PinSubCategoryObject = FGameplayTag::StaticStruct();
    CustomEventNode->CreateUserDefinedPin(TEXT("OutMessageName"), MessageNamePinType, EGPD_Output);

    FEdGraphPinType PayloadPinType;
    PayloadPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    PayloadPinType.PinSubCategoryObject = FInstancedStruct::StaticStruct();
    auto* PayloadPin = CustomEventNode->CreateUserDefinedPin(TEXT("OutPayload"), PayloadPinType, EGPD_Output);

    auto* BindFunction_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    BindFunction_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, BindTo_OnBroadcast),
        UCk_Utils_Messaging_UE::StaticClass());
    BindFunction_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BindFunction_Node, this);

    if (auto* MessageName_InputPin = BindFunction_Node->FindPin(TEXT("InMessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BindFunction_Node);
    }

    auto* MakeMessageListener_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeMessageListener_Node->StructType = FCk_Handle_MessageListener::StaticStruct();
    MakeMessageListener_Node->AllocateDefaultPins();
    MakeMessageListener_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeMessageListener_Node, this);

    if (auto* MessageName_InputPin = MakeMessageListener_Node->FindPin(TEXT("_MessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*MakeMessageListener_Node);
    }

    auto* UnbindFunction_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    UnbindFunction_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, UnbindFrom_OnBroadcast),
        UCk_Utils_Messaging_UE::StaticClass());
    UnbindFunction_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(UnbindFunction_Node, this);

    if (auto* MessageName_InputPin = UnbindFunction_Node->FindPin(TEXT("InMessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*UnbindFunction_Node);
    }

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
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
        }) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if ([[maybe_unused]] const auto IsEmptyPayload = ck::Is_NOT_Valid(_MessagePayload.GetScriptStruct()->ChildProperties))
    {
        if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
            InCompilerContext,
            { {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_MessageReceived, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*CustomEventNode)
            } },
            ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
        {
            return;
        }
    }
    else
    {
        auto* TriggerEnsure_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
        TriggerEnsure_Node->FunctionReference.SetExternalMember(
            GET_FUNCTION_NAME_CHECKED(UCk_Utils_Ensure_UE, TriggerEnsure),
            UCk_Utils_Ensure_UE::StaticClass());
        TriggerEnsure_Node->AllocateDefaultPins();
        InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(TriggerEnsure_Node, this);

        if (auto* Message_Pin = TriggerEnsure_Node->FindPin(TEXT("InMsg"));
            ck::IsValid(Message_Pin, ck::IsValid_Policy_NullptrOnly{}))
        {
            InCompilerContext.GetSchema()->TrySetDefaultValue(*Message_Pin, ck::Format_UE(TEXT("Received a Message Payload that was NOT of the expected type [{}]"), MessageName));
            UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*TriggerEnsure_Node);
        }

        auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
        GetInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)));
        GetInstancedStruct_Node->AllocateDefaultPins();
        InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);

        auto* BreakMessageDefinition_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(this, InSourceGraph);
        auto ScriptStruct = const_cast<UScriptStruct*>(_MessagePayload.GetScriptStruct());
        BreakMessageDefinition_Node->StructType = ScriptStruct;
        BreakMessageDefinition_Node->AllocateDefaultPins();
        BreakMessageDefinition_Node->bMadeAfterOverridePinRemoval = true;
        InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BreakMessageDefinition_Node, this);

        UCk_Utils_EditorGraph_UE::ForEach_NodePins(*BreakMessageDefinition_Node, [&](UEdGraphPin* InGraphPin)
            {
                if (InGraphPin->Direction != EGPD_Output)
                {
                    return;
                }

                const auto& GraphPinName = InGraphPin->PinName;

                if (const auto& ThisNodeMatchingOutputPin = UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Output, *this);
                    ck::IsValid(ThisNodeMatchingOutputPin))
                {
                    UCk_Utils_EditorGraph_UE::Request_LinkPins(
                        InCompilerContext,
                        { { ThisNodeMatchingOutputPin, InGraphPin } },
                        ECk_EditorGraph_PinLinkType::Move);
                }
            });

        if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
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
                }
            }) == ECk_SucceededFailed::Failed)
        {
            return;
        }

        if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
            InCompilerContext,
            { {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_MessageReceived, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Valid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            } },
            ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
        {
            return;
        }
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
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
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_StopListening, ECk_EditorGraph_PinDirection::Input, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*UnbindFunction_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_OutHandle, ECk_EditorGraph_PinDirection::Output, *this),
                HandlePin
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_OutListener, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(MakeMessageListener_Node->StructType->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeMessageListener_Node)
            },
        },
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                BindFunction_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_HandleToUnbind, ECk_EditorGraph_PinDirection::Input, *this),
                UnbindFunction_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                MakeMessageListener_Node->FindPin(TEXT("_MessageListener"))
            },
        },
        ECk_EditorGraph_PinLinkType::Copy) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    BreakAllNodeLinks();
}

auto UCk_K2Node_Message_Listen::DoExpandNode_Compact(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph) -> void
{
    const auto& MessageName = GetMessageNameFromStruct();

    auto* CustomEventNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, InSourceGraph);
    CustomEventNode->CustomFunctionName = *InCompilerContext.GetGuid(CustomEventNode);
    CustomEventNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CustomEventNode, this);

    FEdGraphPinType HandlePinType;
    HandlePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    HandlePinType.PinSubCategoryObject = FCk_Handle::StaticStruct();
    auto* HandlePin = CustomEventNode->CreateUserDefinedPin(TEXT("OutHandle"), HandlePinType, EGPD_Output);

    FEdGraphPinType MessageNamePinType;
    MessageNamePinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    MessageNamePinType.PinSubCategoryObject = FGameplayTag::StaticStruct();
    CustomEventNode->CreateUserDefinedPin(TEXT("OutMessageName"), MessageNamePinType, EGPD_Output);

    FEdGraphPinType PayloadPinType;
    PayloadPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    PayloadPinType.PinSubCategoryObject = FInstancedStruct::StaticStruct();
    auto* PayloadPin = CustomEventNode->CreateUserDefinedPin(TEXT("OutPayload"), PayloadPinType, EGPD_Output);

    auto* BindFunction_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    BindFunction_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, BindTo_OnBroadcast),
        UCk_Utils_Messaging_UE::StaticClass());
    BindFunction_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BindFunction_Node, this);

    if (auto* MessageName_InputPin = BindFunction_Node->FindPin(TEXT("InMessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*BindFunction_Node);
    }

    auto* MakeMessageListener_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeMessageListener_Node->StructType = FCk_Handle_MessageListener::StaticStruct();
    MakeMessageListener_Node->AllocateDefaultPins();
    MakeMessageListener_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeMessageListener_Node, this);

    if (auto* MessageName_InputPin = MakeMessageListener_Node->FindPin(TEXT("_MessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*MakeMessageListener_Node);
    }

    auto* UnbindFunction_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    UnbindFunction_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Messaging_UE, UnbindFrom_OnBroadcast),
        UCk_Utils_Messaging_UE::StaticClass());
    UnbindFunction_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(UnbindFunction_Node, this);

    if (auto* MessageName_InputPin = UnbindFunction_Node->FindPin(TEXT("InMessageName"));
        ck::IsValid(MessageName_InputPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*MessageName_InputPin, MessageName.ToString());
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*UnbindFunction_Node);
    }

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
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
        }) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    auto* ScriptStruct = const_cast<UScriptStruct*>(_MessagePayload.GetScriptStruct());

    auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)));
    GetInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);

    if (auto* ValuePin = GetInstancedStruct_Node->FindPin(TEXT("Value"));
        ck::IsValid(ValuePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        ValuePin->PinType.PinSubCategoryObject = ScriptStruct;
    }

    auto* TriggerEnsure_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    TriggerEnsure_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_Ensure_UE, TriggerEnsure),
        UCk_Utils_Ensure_UE::StaticClass());
    TriggerEnsure_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(TriggerEnsure_Node, this);

    if (auto* Message_Pin = TriggerEnsure_Node->FindPin(TEXT("InMsg"));
        ck::IsValid(Message_Pin, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.GetSchema()->TrySetDefaultValue(*Message_Pin, ck::Format_UE(TEXT("Received a Message Payload that was NOT of the expected type [{}]"), MessageName));
        UCk_Utils_EditorGraph_UE::Request_ForceRefreshNode(*TriggerEnsure_Node);
    }

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
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
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*TriggerEnsure_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("NotValid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            }
        }) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_CompactPayloadOut, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_MessageReceived, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Valid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
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
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_StopListening, ECk_EditorGraph_PinDirection::Input, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*UnbindFunction_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_OutHandle, ECk_EditorGraph_PinDirection::Output, *this),
                HandlePin
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_OutListener, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(MakeMessageListener_Node->StructType->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeMessageListener_Node)
            },
        },
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                BindFunction_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_HandleToUnbind, ECk_EditorGraph_PinDirection::Input, *this),
                UnbindFunction_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_messaging::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                MakeMessageListener_Node->FindPin(TEXT("_MessageListener"))
            },
        },
        ECk_EditorGraph_PinLinkType::Copy) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    BreakAllNodeLinks();
}

auto UCk_K2Node_Message_Listen::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Message_Listen"), TEXT("[Ck] Listen For Message 👂..."));
}

auto UCk_K2Node_Message_Listen::DoGet_MessageDefinitionPinsDirection() const -> ECk_EditorGraph_PinDirection
{
    return ECk_EditorGraph_PinDirection::Output;
}

auto UCk_K2Node_Message_Listen::IsCompatibleWithGraph(UEdGraph const* InGraph) const -> bool
{
    if (ck::Is_NOT_Valid(InGraph))
    {
        return Super::IsCompatibleWithGraph(InGraph);
    }

    const auto& OuterBlueprint = InGraph->GetTypedOuter<UBlueprint>();
    if (ck::Is_NOT_Valid(OuterBlueprint))
    {
        return Super::IsCompatibleWithGraph(InGraph);
    }

    return OuterBlueprint->UbergraphPages.Contains(InGraph);
}

// --------------------------------------------------------------------------------------------------------------------
// SCk_GraphNode_Message_Broadcast
// --------------------------------------------------------------------------------------------------------------------

auto SCk_GraphNode_Message_Broadcast::Construct(const FArguments& InArgs, UCk_K2Node_Message_Broadcast* InNode) -> void
{
    GraphNode = InNode;
    _MessageNode = InNode;
    UpdateGraphNode();
}

auto SCk_GraphNode_Message_Broadcast::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void
{
    SGraphNode::CreateBelowPinControls(MainBox);

    if (ck::Is_NOT_Valid(_MessageNode.Get()))
    {
        return;
    }

    if (ck::Is_NOT_Valid(_MessageNode->_MessagePayload))
    {
        return;
    }

    MainBox->AddSlot()
        .AutoHeight()
        .Padding(4.0f, 2.0f)
        [
            SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
                .Padding(6.0f, 4.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                        .Text_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return FText::FromString(_MessageNode->IsCompactPayloadMode() ? TEXT("📦") : TEXT("📋"));
                                                }
                                                return FText::FromString(TEXT("?"));
                                            })
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                                        .ColorAndOpacity_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return _MessageNode->IsCompactPayloadMode()
                                                        ? FLinearColor(0.4f, 0.8f, 1.0f)
                                                        : FLinearColor(1.0f, 0.8f, 0.4f);
                                                }
                                                return FLinearColor::White;
                                            })
                                        .ToolTipText_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return FText::FromString(_MessageNode->IsCompactPayloadMode()
                                                        ? TEXT("Compact: Single struct input")
                                                        : TEXT("Expanded: Individual property inputs"));
                                                }
                                                return FText::GetEmpty();
                                            })
                                ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                        .Text_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return FText::FromString(_MessageNode->IsCompactPayloadMode()
                                                        ? TEXT("Compact")
                                                        : TEXT("Expanded"));
                                                }
                                                return FText::GetEmpty();
                                            })
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                                        .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                                ]
                        ]
                ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------
// SCk_GraphNode_Message_Listen
// --------------------------------------------------------------------------------------------------------------------

auto SCk_GraphNode_Message_Listen::Construct(const FArguments& InArgs, UCk_K2Node_Message_Listen* InNode) -> void
{
    GraphNode = InNode;
    _MessageNode = InNode;
    UpdateGraphNode();
}

auto SCk_GraphNode_Message_Listen::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void
{
    SGraphNode::CreateBelowPinControls(MainBox);

    if (ck::Is_NOT_Valid(_MessageNode.Get()))
    {
        return;
    }

    if (ck::Is_NOT_Valid(_MessageNode->_MessagePayload))
    {
        return;
    }

    MainBox->AddSlot()
        .AutoHeight()
        .Padding(4.0f, 2.0f)
        [
            SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
                .Padding(6.0f, 4.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(2.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                        .Text_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return FText::FromString(_MessageNode->IsCompactPayloadMode() ? TEXT("📦") : TEXT("📋"));
                                                }
                                                return FText::FromString(TEXT("?"));
                                            })
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                                        .ColorAndOpacity_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return _MessageNode->IsCompactPayloadMode()
                                                        ? FLinearColor(0.4f, 0.8f, 1.0f)
                                                        : FLinearColor(1.0f, 0.8f, 0.4f);
                                                }
                                                return FLinearColor::White;
                                            })
                                        .ToolTipText_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return FText::FromString(_MessageNode->IsCompactPayloadMode()
                                                        ? TEXT("Compact: Single struct output")
                                                        : TEXT("Expanded: Individual property outputs"));
                                                }
                                                return FText::GetEmpty();
                                            })
                                ]
                            + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                        .Text_Lambda([this]()
                                            {
                                                if (ck::IsValid(_MessageNode.Get()))
                                                {
                                                    return FText::FromString(_MessageNode->IsCompactPayloadMode()
                                                        ? TEXT("Compact")
                                                        : TEXT("Expanded"));
                                                }
                                                return FText::GetEmpty();
                                            })
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                                        .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                                ]
                        ]
                ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE