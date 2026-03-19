#include "CkDynamicFragment_Add_K2Node.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"
#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_K2NodeHelpers.h"
#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_SGraphNode.h"

#include "CkDynamic/CkDynamic_Utils.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <GraphEditorSettings.h>
#include <K2Node_MakeStruct.h>
#include <KismetCompiler.h>

#include <Kismet/BlueprintInstancedStructLibrary.h>

#include <Kismet2/BlueprintEditorUtils.h>

#include <Editor.h>

#define LOCTEXT_NAMESPACE "K2Node_DynamicFragment_Add"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2_node_dynamic_fragment_add
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_FragmentSelector = TEXT("Fragment Type");
    static auto PinName_OutHandle = TEXT("OutHandle");
    static auto PinName_CompactPayload = TEXT("Fragment");
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
                                ? PropertyChangedEvent.Property->GetFName()
                                : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCkDynamicFragment_Add_K2Node, _PayloadMode))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto UCkDynamicFragment_Add_K2Node::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCkDynamicFragment_Add_K2Node::GetNodeTitle(
    ENodeTitleType::Type InTitleType) const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_Add_K2Node"),
        TEXT("[Ck][DynamicFragment] Add Fragment")
    );
}

auto UCkDynamicFragment_Add_K2Node::GetIconAndTint(
    FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

auto UCkDynamicFragment_Add_K2Node::GetMenuCategory() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_Add_K2Node"),
        TEXT("Ck|Utils|DynamicFragment")
    );
}

auto UCkDynamicFragment_Add_K2Node::IsNodePure() const -> bool
{
    return false;
}

auto UCkDynamicFragment_Add_K2Node::CreateVisualWidget() -> TSharedPtr<SGraphNode>
{
    return SNew(SCk_GraphNode_WithPayloadBanner, this);
}

auto UCkDynamicFragment_Add_K2Node::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& InOldPins) -> void
{
    AllocateDefaultPins();

    for (auto* OldPin : InOldPins)
    {
        if (OldPin->PinName == ck_k2_node_dynamic_fragment_add::PinName_FragmentSelector)
        {
            if (auto* NewPin = FindPin(OldPin->PinName))
            {
                NewPin->DefaultValue = OldPin->DefaultValue;
            }
            break;
        }
    }

    CreatePinsFromFragmentStruct();
    RestoreSplitPins(InOldPins);
}

auto UCkDynamicFragment_Add_K2Node::AllocateDefaultPins() -> void
{
    // Create execution pins
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    // Create fragment selector pin (on-node dropdown)
    ck::FStructTypeSelectorHelpers::CreateSelectorPin(
        *this,
        ck_k2_node_dynamic_fragment_add::PinName_FragmentSelector);

    // Create Handle input pin
    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;

    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_dynamic_fragment_add::PinName_Handle,
        HandlePinParams
    );

    // Create pins from the fragment struct
    CreatePinsFromFragmentStruct();
    
    // Create OutHandle output pin
    CreatePin(
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_dynamic_fragment_add::PinName_OutHandle
    );
}

auto UCkDynamicFragment_Add_K2Node::DoAllocate_DefaultPins() -> void
{
    // Not used - we override AllocateDefaultPins directly
}

auto UCkDynamicFragment_Add_K2Node::DoExpandNode(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    ECk_ValidInvalid InNodeValidity) -> void
{
    if (ck::Is_NOT_Valid(Get_SelectedStructType()))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Fragment Type", "Invalid Fragment Type. @@").ToString(), this);
        return;
    }

    const auto* StructType = Get_SelectedStructType();

    if (IsCompactMode())
    {
        DoExpandNode_Compact(InCompilerContext, InSourceGraph, StructType);
    }
    else
    {
        DoExpandNode_Expanded(InCompilerContext, InSourceGraph, StructType);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::DoExpandNode_Expanded(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    const UScriptStruct* InStructType) -> void
{
    const auto* StructType = InStructType;
    
    // Create MakeStruct node for the fragment data
    auto* MakeStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeStruct_Node->StructType = const_cast<UScriptStruct*>(StructType);
    MakeStruct_Node->AllocateDefaultPins();
    MakeStruct_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeStruct_Node, this);
    
    // Connect input pins to MakeStruct
    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*MakeStruct_Node, [&](UEdGraphPin* InGraphPin)
    {
        if (InGraphPin->Direction != EGPD_Input)
        { return; }
        
        const auto& GraphPinName = InGraphPin->PinName;
        
        if (const auto& ThisNodeMatchingInputPin = UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Input, *this);
            ck::IsValid(ThisNodeMatchingInputPin))
        {
            if ((*ThisNodeMatchingInputPin)->LinkedTo.IsEmpty())
            {
                UCk_Utils_EditorGraph_UE::Request_CopyPinValues(
                    InCompilerContext,
                    {
                        { ThisNodeMatchingInputPin, InGraphPin }
                    }
                );
                return;
            }
            
            UCk_Utils_EditorGraph_UE::Request_LinkPins(
                InCompilerContext,
                {
                    { ThisNodeMatchingInputPin, InGraphPin }
                },
                ECk_EditorGraph_PinLinkType::Move
            );
        }
    });
    
    // Create MakeInstancedStruct node
    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);
    
    // Create Add_Fragment node
    auto* AddFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    AddFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Add_Fragment),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    AddFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(AddFragment_Node, this);
    
    // Connect the nodes
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(StructType->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Input, *MakeInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*AddFragment_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*MakeInstancedStruct_Node),
                AddFragment_Node->FindPin(TEXT("InStructData"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect execution and handle pins
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*MakeInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*AddFragment_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment_add::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                AddFragment_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment_add::PinName_OutHandle, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*AddFragment_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }
    
    BreakAllNodeLinks();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::DoExpandNode_Compact(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    const UScriptStruct* InStructType) -> void
{
    namespace ns = ck_k2_node_dynamic_fragment_add;

    // Create MakeInstancedStruct from the compact input pin
    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);

    // Create Add_Fragment node
    auto* AddFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    AddFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Add_Fragment),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    AddFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(AddFragment_Node, this);

    // Connect compact input pin to MakeInstancedStruct
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_CompactPayload, ECk_EditorGraph_PinDirection::Input, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Input, *MakeInstancedStruct_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    // MakeInstancedStruct -> Add_Fragment
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*AddFragment_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*MakeInstancedStruct_Node),
                AddFragment_Node->FindPin(TEXT("InStructData"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // Connect execution and handle pins
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*MakeInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*AddFragment_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                AddFragment_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_OutHandle, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*AddFragment_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    BreakAllNodeLinks();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::PinDefaultValueChanged(
    UEdGraphPin* InPin) -> void
{
    if (InPin != nullptr && InPin->PinName == ck_k2_node_dynamic_fragment_add::PinName_FragmentSelector)
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
        FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
        return;
    }

    Super::PinDefaultValueChanged(InPin);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::Get_SelectedStructType() const -> UScriptStruct*
{
    return ck::FStructTypeSelectorHelpers::GetSelectedStruct(
        *this, ck_k2_node_dynamic_fragment_add::PinName_FragmentSelector);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_Add_K2Node"),
        TEXT("[Ck][DynamicFragment] Add Fragment...")
    );
}

auto UCkDynamicFragment_Add_K2Node::DoValidateNodePins(
    const TOptional<FKismetCompilerContext*>& InCompilerContext) const -> ECk_ValidInvalid
{
    if (ck::Is_NOT_Valid(Get_SelectedStructType()))
    {
        if (InCompilerContext.IsSet())
        {
            InCompilerContext.GetValue()->MessageLog.Error(
                *LOCTEXT("No Fragment Type Selected", "No Fragment Type selected. @@").ToString(), this
            );
        }
        return ECk_ValidInvalid::Invalid;
    }

    return ECk_ValidInvalid::Valid;
}

auto UCkDynamicFragment_Add_K2Node::CreatePinsFromFragmentStruct() -> void
{
    if (ck::Is_NOT_Valid(Get_SelectedStructType()))
    { return; }

    const auto* StructType = Get_SelectedStructType();

    if (IsCompactMode())
    {
        auto* Pin = CreatePin(
            EGPD_Input,
            UEdGraphSchema_K2::PC_Struct,
            const_cast<UScriptStruct*>(StructType),
            ck_k2_node_dynamic_fragment_add::PinName_CompactPayload);

        Pin->PinFriendlyName = FText::FromString(TEXT("Fragment"));
        return;
    }

    const auto& CreatePinFromProperty = [this](const FProperty* InProperty)
    {
        auto* Pin = CreatePin(EGPD_Input, NAME_None, InProperty->GetFName());

        if (ck::Is_NOT_Valid(Pin, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        Pin->PinFriendlyName = InProperty->GetDisplayNameText();
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

        K2Schema->ConvertPropertyToPinType(InProperty, Pin->PinType);

        K2Schema->ConstructBasicPinTooltip(*Pin, InProperty->GetToolTipText(), Pin->PinToolTip);
    };

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        auto* Property = *It;

        if (Property->HasAnyPropertyFlags(CPF_Parm) ||
            NOT FBlueprintEditorUtils::PropertyStillExists(Property) ||
            NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            ck::IsValid(FindPin(Property->GetFName()), ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        CreatePinFromProperty(Property);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
