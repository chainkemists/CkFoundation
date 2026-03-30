#include "CkDynamicFragment_K2Node.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"
#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_K2NodeHelpers.h"
#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_SGraphNode.h"

#include "CkDynamic/Public/CkDynamic/CkDynamic_Utils.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <GraphEditorSettings.h>
#include <K2Node_BreakStruct.h>
#include <K2Node_IfThenElse.h>
#include <KismetCompiler.h>

#include <Kismet/BlueprintInstancedStructLibrary.h>

#include <Kismet2/BlueprintEditorUtils.h>

#include <Editor.h>

#define LOCTEXT_NAMESPACE "K2Node_DynamicFragment"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2_node_dynamic_fragment
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_FragmentSelector = TEXT("Fragment Type");
    static auto PinName_Valid = TEXT("Valid");
    static auto PinName_Invalid = TEXT("Invalid");
    static auto PinName_CompactPayload = TEXT("Fragment");
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_K2Node::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCkDynamicFragment_K2Node, _PayloadMode))
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
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_K2Node"),
        TEXT("[Ck][DynamicFragment] Get Fragment")
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

auto UCkDynamicFragment_K2Node::CreateVisualWidget() -> TSharedPtr<SGraphNode>
{
    return SNew(SCk_GraphNode_WithPayloadBanner, this);
}

auto UCkDynamicFragment_K2Node::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& InOldPins) -> void
{
    AllocateDefaultPins();

    // Restore the selector pin's value from old pins so CreatePinsFromFragmentStruct
    // can read the selected struct and create the appropriate output pins.
    for (auto* OldPin : InOldPins)
    {
        if (OldPin->PinName == ck_k2_node_dynamic_fragment::PinName_FragmentSelector)
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

auto UCkDynamicFragment_K2Node::AllocateDefaultPins() -> void
{
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2_node_dynamic_fragment::PinName_Valid);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2_node_dynamic_fragment::PinName_Invalid);

    // Create fragment selector pin (on-node dropdown, no filter for dynamic fragments)
    ck::FStructTypeSelectorHelpers::CreateSelectorPin(
        *this,
        ck_k2_node_dynamic_fragment::PinName_FragmentSelector);

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

    CreatePinsFromFragmentStruct();
}

auto UCkDynamicFragment_K2Node::DoAllocate_DefaultPins() -> void
{
}

auto UCkDynamicFragment_K2Node::DoExpandNode(
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

auto UCkDynamicFragment_K2Node::DoExpandNode_Expanded(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    const UScriptStruct* InStructType) -> void
{
    // Create Has_Fragment node
    auto* HasFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    HasFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Has_Fragment),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    HasFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(HasFragment_Node, this);

    if (auto* StructTypePin = HasFragment_Node->FindPin(TEXT("InStructType"));
        ck::IsValid(StructTypePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        StructTypePin->DefaultObject = const_cast<UScriptStruct*>(InStructType);
    }

    // Create branch node
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

    if (auto* StructTypePin = GetFragment_Node->FindPin(TEXT("InStructType"));
        ck::IsValid(StructTypePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        StructTypePin->DefaultObject = const_cast<UScriptStruct*>(InStructType);
    }

    // Create GetInstancedStructValue node
    auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetInstancedStruct_Node->SetFromFunction(
        UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)
        )
    );
    GetInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);

    // Create BreakStruct node
    auto* BreakStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(this, InSourceGraph);
    BreakStruct_Node->StructType = const_cast<UScriptStruct*>(InStructType);
    BreakStruct_Node->AllocateDefaultPins();
    BreakStruct_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BreakStruct_Node, this);

    // Connect Handle to Has_Fragment and Get_Fragment_TypeUnsafe
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

    // IN -> Has_Fragment
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

    // Has_Fragment -> Branch
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

    // Branch Then -> Get_Fragment_TypeUnsafe
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                BranchNode->FindPin(TEXT("Then")),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetFragment_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // Get_Fragment_TypeUnsafe -> GetInstancedStructValue
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

    // GetInstancedStructValue -> BreakStruct
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(InStructType->GetFName(), ECk_EditorGraph_PinDirection::Input, *BreakStruct_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // BreakStruct output pins -> our output pins
    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*BreakStruct_Node, [&](UEdGraphPin* InGraphPin)
    {
        if (InGraphPin->Direction != EGPD_Output)
        { return; }

        if (const auto& ThisNodeMatchingOutputPin = UCk_Utils_EditorGraph_UE::Get_Pin(InGraphPin->PinName, ECk_EditorGraph_PinDirection::Output, *this);
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

    // Execution outputs
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

    // NotValid -> Invalid
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

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_K2Node::DoExpandNode_Compact(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    const UScriptStruct* InStructType) -> void
{
    // Create Has_Fragment node
    auto* HasFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    HasFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Has_Fragment),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    HasFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(HasFragment_Node, this);

    if (auto* StructTypePin = HasFragment_Node->FindPin(TEXT("InStructType"));
        ck::IsValid(StructTypePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        StructTypePin->DefaultObject = const_cast<UScriptStruct*>(InStructType);
    }

    // Create branch node
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

    if (auto* StructTypePin = GetFragment_Node->FindPin(TEXT("InStructType"));
        ck::IsValid(StructTypePin, ck::IsValid_Policy_NullptrOnly{}))
    {
        StructTypePin->DefaultObject = const_cast<UScriptStruct*>(InStructType);
    }

    // Create GetInstancedStructValue node
    auto* GetInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetInstancedStruct_Node->SetFromFunction(
        UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)
        )
    );
    GetInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetInstancedStruct_Node, this);

    // Connect Handle to Has_Fragment and Get_Fragment_TypeUnsafe
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

    // IN -> Has_Fragment
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

    // Has_Fragment -> Branch
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

    // Branch Then -> Get_Fragment_TypeUnsafe
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                BranchNode->FindPin(TEXT("Then")),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetFragment_Node)
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // Get_Fragment_TypeUnsafe -> GetInstancedStructValue
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

    // Connect compact output pin directly to GetInstancedStructValue Value output
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2_node_dynamic_fragment::PinName_CompactPayload, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Output, *GetInstancedStruct_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    // Execution outputs
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

    // NotValid -> Invalid
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

auto UCkDynamicFragment_K2Node::PinDefaultValueChanged(
    UEdGraphPin* InPin) -> void
{
    if (InPin != nullptr && InPin->PinName == ck_k2_node_dynamic_fragment::PinName_FragmentSelector)
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
        FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
        return;
    }

    Super::PinDefaultValueChanged(InPin);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_K2Node::Get_SelectedStructType() const -> UScriptStruct*
{
    return ck::FStructTypeSelectorHelpers::GetSelectedStruct(
        *this, ck_k2_node_dynamic_fragment::PinName_FragmentSelector);
}

// --------------------------------------------------------------------------------------------------------------------

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

auto UCkDynamicFragment_K2Node::CreatePinsFromFragmentStruct() -> void
{
    if (ck::Is_NOT_Valid(Get_SelectedStructType()))
    { return; }

    const auto* StructType = Get_SelectedStructType();

    if (IsCompactMode())
    {
        auto* Pin = CreatePin(
            EGPD_Output,
            UEdGraphSchema_K2::PC_Struct,
            const_cast<UScriptStruct*>(StructType),
            ck_k2_node_dynamic_fragment::PinName_CompactPayload);

        Pin->PinFriendlyName = FText::FromString(TEXT("Fragment"));
        return;
    }

    const auto& CreatePinFromProperty = [this](const FProperty* InProperty)
    {
        auto* Pin = CreatePin(EGPD_Output, NAME_None, InProperty->GetFName());

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
