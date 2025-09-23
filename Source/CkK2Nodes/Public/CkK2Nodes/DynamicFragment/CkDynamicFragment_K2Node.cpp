#include "CkDynamicFragment_K2Node.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkDynamic/CkDynamic_Utils.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <GraphEditorSettings.h>
#include <K2Node_BreakStruct.h>
#include <K2Node_IfThenElse.h>
#include <KismetCompiler.h>

#include <Kismet/BlueprintInstancedStructLibrary.h>

#include <Kismet2/BlueprintEditorUtils.h>

#define LOCTEXT_NAMESPACE "K2Node_DynamicFragment"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2_node_dynamic_fragment
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_Valid = TEXT("Valid");
    static auto PinName_Invalid = TEXT("Invalid");
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_K2Node::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
                                ? PropertyChangedEvent.Property->GetFName()
                                : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCkDynamicFragment_K2Node, _FragmentType))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto UCkDynamicFragment_K2Node::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCkDynamicFragment_K2Node::GetNodeTitle(
    ENodeTitleType::Type InTitleType) const -> FText
{
    if (ck::Is_NOT_Valid(_FragmentType))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCkDynamicFragment_K2Node"),
            TEXT("[Ck][DynamicFragment] Get Fragment\n(Select Fragment Type)")
        );
    }

    const auto* StructType = _FragmentType.GetScriptStruct();
    if (ck::Is_NOT_Valid(StructType))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCkDynamicFragment_K2Node"),
            TEXT("[Ck][DynamicFragment] Get Fragment\n(INVALID Fragment Type)")
        );
    }

    const auto& StructName = StructType->GetDisplayNameText();
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_K2Node"),
        *ck::Format_UE(TEXT("[Ck][DynamicFragment] Get Fragment\n({})"), StructName.ToString())
    );
}

auto UCkDynamicFragment_K2Node::GetIconAndTint(
    FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

auto UCkDynamicFragment_K2Node::GetMenuCategory() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_K2Node"),
        TEXT("Ck|Utils|DynamicFragment")
    );
}

auto UCkDynamicFragment_K2Node::IsNodePure() const -> bool
{
    return false;
}

auto UCkDynamicFragment_K2Node::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& InOldPins) -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto UCkDynamicFragment_K2Node::AllocateDefaultPins() -> void
{
    // Create IN execution pin
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    
    // Create Valid and Invalid OUT execution pins
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2_node_dynamic_fragment::PinName_Valid);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2_node_dynamic_fragment::PinName_Invalid);
    
    // Create Handle input pin
    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;
    
    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_dynamic_fragment::PinName_Handle,
        HandlePinParams
    );
    
    // Create pins from the fragment struct
    CreatePinsFromFragmentStruct();
}

auto UCkDynamicFragment_K2Node::DoAllocate_DefaultPins() -> void
{
    // Not used - we override AllocateDefaultPins directly
}

auto UCkDynamicFragment_K2Node::DoExpandNode(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    ECk_ValidInvalid InNodeValidity) -> void
{
    if (ck::Is_NOT_Valid(_FragmentType))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Fragment Type", "Invalid Fragment Type. @@").ToString(), this);
        return;
    }
    
    const auto* StructType = _FragmentType.GetScriptStruct();
    if (ck::Is_NOT_Valid(StructType))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Struct Type", "Invalid Struct Type. @@").ToString(), this);
        return;
    }
    
    // Create Has_Fragment node
    auto* HasFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    HasFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Has_Fragment),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    HasFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(HasFragment_Node, this);
    
    // Set the struct type for Has_Fragment
    if (auto* StructTypePin = HasFragment_Node->FindPin(TEXT("InStructType"));
        ck::IsValid(StructTypePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        StructTypePin->DefaultObject = const_cast<UScriptStruct*>(StructType);
    }
    
    // Create branch node (if/then/else)
    auto* BranchNode = InCompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, InSourceGraph);
    BranchNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BranchNode, this);
    
    // Create Get_Fragment_TypeUnsafe node
    auto* GetFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Get_Fragment_TypeUnsafe),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    GetFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetFragment_Node, this);
    
    // Set the struct type for Get_Fragment_TypeUnsafe
    if (auto* StructTypePin = GetFragment_Node->FindPin(TEXT("InStructType"));
        ck::IsValid(StructTypePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        StructTypePin->DefaultObject = const_cast<UScriptStruct*>(StructType);
    }
    
    // Create GetInstancedStructValue node to extract the actual struct data
    auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetInstancedStruct_Node->SetFromFunction(
        UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)
        )
    );
    GetInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);
    
    // Create BreakStruct node to expand the fragment data
    auto* BreakStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(this, InSourceGraph);
    BreakStruct_Node->StructType = const_cast<UScriptStruct*>(StructType);
    BreakStruct_Node->AllocateDefaultPins();
    BreakStruct_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BreakStruct_Node, this);
    
    // Connect the input Handle to both Has_Fragment and Get_Fragment_TypeUnsafe
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                HasFragment_Node->FindPin(TEXT("InHandle"))
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment::PinName_Handle, ECk_EditorGraph_PinDirection::Input, *this),
                GetFragment_Node->FindPin(TEXT("InHandle"))
            }
        },
        ECk_EditorGraph_PinLinkType::Copy
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect execution flow: IN -> Has_Fragment
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*HasFragment_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect Has_Fragment output to Branch condition
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*HasFragment_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*BranchNode)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*HasFragment_Node),
                BranchNode->FindPin(TEXT("Condition"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect Branch Then (Valid) to Get_Fragment_TypeUnsafe
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                BranchNode->FindPin(TEXT("Then")),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetFragment_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect Get_Fragment_TypeUnsafe result to GetInstancedStructValue
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*GetFragment_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*GetFragment_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("InstancedStruct"), ECk_EditorGraph_PinDirection::Input, *GetInstancedStruct_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect GetInstancedStructValue result to BreakStruct
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(StructType->GetFName(), ECk_EditorGraph_PinDirection::Input, *BreakStruct_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Connect the output pins from BreakStruct to our node's output pins
    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*BreakStruct_Node, [&](UEdGraphPin* InGraphPin)
    {
        if (InGraphPin->Direction != EGPD_Output)
        { return; }
        
        const auto& GraphPinName = InGraphPin->PinName;
        
        if (const auto& ThisNodeMatchingOutputPin = UCk_Utils_EditorGraph_UE::Get_Pin(GraphPinName, ECk_EditorGraph_PinDirection::Output, *this);
            ck::IsValid(ThisNodeMatchingOutputPin))
        {
            UCk_Utils_EditorGraph_UE::Request_LinkPins(
                InCompilerContext,
                {
                    { ThisNodeMatchingOutputPin, InGraphPin }
                },
                ECk_EditorGraph_PinLinkType::Move
            );
        }
    });
    
    // Connect the execution output pins
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment::PinName_Valid, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Valid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment::PinName_Invalid, ECk_EditorGraph_PinDirection::Output, *this),
                BranchNode->FindPin(TEXT("Else"))
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }
    
    // Handle the NotValid exec from GetInstancedStructValue (should not happen if Has_Fragment returned true, but handle it)
    // Route it to the Invalid output
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("NotValid"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node),
                BranchNode->FindPin(TEXT("Else"))
            }
        },
        ECk_EditorGraph_PinLinkType::Copy
    ) == ECk_SucceededFailed::Failed) { return; }
    
    BreakAllNodeLinks();
}

auto UCkDynamicFragment_K2Node::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_K2Node"),
        TEXT("[Ck][DynamicFragment] Get Fragment...")
    );
}

auto UCkDynamicFragment_K2Node::DoValidateNodePins(
    const TOptional<FKismetCompilerContext*>& InCompilerContext) const -> ECk_ValidInvalid
{
    if (ck::Is_NOT_Valid(_FragmentType))
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

auto UCkDynamicFragment_K2Node::CreatePinsFromFragmentStruct() -> void
{
    if (ck::Is_NOT_Valid(_FragmentType))
    { return; }
    
    const auto& CreatePinFromProperty = [this](const FProperty* InProperty, const uint8* InContainer)
    {
        auto* Pin = CreatePin(EGPD_Output, NAME_None, InProperty->GetFName());
        
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
    
    auto* StructData = _FragmentType.GetMemory();
    auto* StructType = _FragmentType.GetScriptStruct();
    
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

#undef LOCTEXT_NAMESPACE
