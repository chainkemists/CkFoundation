#include "CkDynamicFragment_Add_K2Node.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkDynamic/Public/CkDynamic/CkDynamic_Utils.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <GraphEditorSettings.h>
#include <K2Node_MakeStruct.h>
#include <KismetCompiler.h>

#include <Kismet/BlueprintInstancedStructLibrary.h>

#include <Kismet2/BlueprintEditorUtils.h>

#define LOCTEXT_NAMESPACE "K2Node_DynamicFragment_Add"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2_node_dynamic_fragment_add
{
    static auto PinName_Handle = TEXT("Handle");
    static auto PinName_OutHandle = TEXT("OutHandle");
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkDynamicFragment_Add_K2Node::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCkDynamicFragment_Add_K2Node, _FragmentType))
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
    if (ck::Is_NOT_Valid(_FragmentType))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCkDynamicFragment_Add_K2Node"),
            TEXT("[Ck][DynamicFragment] Add Fragment\n(Select Fragment Type)")
        );
    }

    const auto* StructType = _FragmentType.GetScriptStruct();
    if (ck::Is_NOT_Valid(StructType))
    {
        return CK_UTILS_IO_GET_LOCTEXT(
            TEXT("UCkDynamicFragment_Add_K2Node"),
            TEXT("[Ck][DynamicFragment] Add Fragment\n(INVALID Fragment Type)")
        );
    }

    const auto& StructName = StructType->GetDisplayNameText();
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkDynamicFragment_Add_K2Node"),
        *ck::Format_UE(TEXT("[Ck][DynamicFragment] Add Fragment\n({})"), StructName.ToString())
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

auto UCkDynamicFragment_Add_K2Node::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& InOldPins) -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto UCkDynamicFragment_Add_K2Node::AllocateDefaultPins() -> void
{
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    auto HandlePinParams = FCreatePinParams{};
    HandlePinParams.bIsReference = true;

    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_dynamic_fragment_add::PinName_Handle,
        HandlePinParams
    );

    CreatePinsFromFragmentStruct();

    CreatePin(
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        ck_k2_node_dynamic_fragment_add::PinName_OutHandle
    );
}

auto UCkDynamicFragment_Add_K2Node::DoAllocate_DefaultPins() -> void
{
}

auto UCkDynamicFragment_Add_K2Node::DoExpandNode(
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

    auto* MakeStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeStruct_Node->StructType = const_cast<UScriptStruct*>(StructType);
    MakeStruct_Node->AllocateDefaultPins();
    MakeStruct_Node->bMadeAfterOverridePinRemoval = true;
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeStruct_Node, this);

    UCk_Utils_EditorGraph_UE::ForEach_NodePins(*MakeStruct_Node, [&](UEdGraphPin* InGraphPin)
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

    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeInstancedStruct_Node, this);

    auto* AddFragment_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    AddFragment_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_DynamicFragment_UE, Add_Fragment),
        UCk_Utils_DynamicFragment_UE::StaticClass()
    );
    AddFragment_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(AddFragment_Node, this);

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
    ) == ECk_SucceededFailed::Failed) {
        return;
    }

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
    ) == ECk_SucceededFailed::Failed) {
        return;
    }

    BreakAllNodeLinks();
}

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

auto UCkDynamicFragment_Add_K2Node::CreatePinsFromFragmentStruct() -> void
{
    if (ck::Is_NOT_Valid(_FragmentType))
    {
        return;
    }

    const auto& CreatePinFromProperty = [this](const FProperty* InProperty, const uint8* InContainer)
        {
            auto* Pin = CreatePin(EGPD_Input, NAME_None, InProperty->GetFName());

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
        {
            continue;
        }

        CreatePinFromProperty(Property, StructData);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE