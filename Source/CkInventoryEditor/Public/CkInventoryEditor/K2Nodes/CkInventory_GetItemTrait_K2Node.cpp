#include "CkInventory_GetItemTrait_K2Node.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/Item/CkItem_Definition.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <GraphEditorSettings.h>
#include <K2Node_CallFunction.h>
#include <K2Node_IfThenElse.h>
#include <K2Node_DynamicCast.h>
#include <KismetCompiler.h>

#include <Kismet2/BlueprintEditorUtils.h>

#include <Editor.h>

#define LOCTEXT_NAMESPACE "K2Node_GetItemTrait"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2_node_get_item_trait
{
    static auto PinName_ItemHandle = TEXT("Item Handle");
    static auto PinName_Valid = TEXT("Valid");
    static auto PinName_Invalid = TEXT("Invalid");
    static auto PinName_Trait = TEXT("Trait");
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkInventory_GetItemTrait_K2Node::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
                                ? PropertyChangedEvent.Property->GetFName()
                                : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCkInventory_GetItemTrait_K2Node, _TraitClass))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto UCkInventory_GetItemTrait_K2Node::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCkInventory_GetItemTrait_K2Node::PinDefaultValueChanged(
    UEdGraphPin* InPin) -> void
{
    Super::PinDefaultValueChanged(InPin);
}

auto UCkInventory_GetItemTrait_K2Node::Get_TraitClassFromPin() const -> UClass*
{
    if (ck::Is_NOT_Valid(_TraitClass))
    { return nullptr; }

    // Reject the base abstract class
    if (_TraitClass == UCk_ItemTrait::StaticClass())
    { return nullptr; }

    return _TraitClass.Get();
}

auto UCkInventory_GetItemTrait_K2Node::GetNodeTitle(
    ENodeTitleType::Type InTitleType) const -> FText
{
    if (ck::IsValid(Get_TraitClassFromPin()))
    {
        return FText::FromString(FString::Printf(TEXT("[Ck][Item] Get Trait: %s"), *Get_TraitClassFromPin()->GetDisplayNameText().ToString()));
    }

    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkInventory_GetItemTrait_K2Node"),
        TEXT("[Ck][Item] Get Item Trait")
    );
}

auto UCkInventory_GetItemTrait_K2Node::GetIconAndTint(
    FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

auto UCkInventory_GetItemTrait_K2Node::GetMenuCategory() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkInventory_GetItemTrait_K2Node"),
        TEXT("Ck|Utils|Item")
    );
}

auto UCkInventory_GetItemTrait_K2Node::IsNodePure() const -> bool
{
    return false;
}

auto UCkInventory_GetItemTrait_K2Node::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin*>& InOldPins) -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto UCkInventory_GetItemTrait_K2Node::AllocateDefaultPins() -> void
{
    // Create IN execution pin
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

    // Create Valid and Invalid OUT execution pins
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2_node_get_item_trait::PinName_Valid);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2_node_get_item_trait::PinName_Invalid);

    // Create Item Handle input pin
    {
        auto HandlePinParams = FCreatePinParams{};
        HandlePinParams.bIsReference = true;

        CreatePin(
            EGPD_Input,
            UEdGraphSchema_K2::PC_Struct,
            FCk_Handle_Item::StaticStruct(),
            ck_k2_node_get_item_trait::PinName_ItemHandle,
            HandlePinParams
        );
    }

    // Create Trait output pin (typed to the selected trait class or base)
    {
        const auto* TraitClass = Get_TraitClassFromPin();
        auto* Pin = CreatePin(
            EGPD_Output,
            UEdGraphSchema_K2::PC_Object,
            TraitClass ? const_cast<UClass*>(TraitClass) : UCk_ItemTrait::StaticClass(),
            ck_k2_node_get_item_trait::PinName_Trait);

        Pin->PinFriendlyName = FText::FromString(TEXT("Trait"));
    }
}

auto UCkInventory_GetItemTrait_K2Node::DoAllocate_DefaultPins() -> void
{
    // Not used - we override AllocateDefaultPins directly
}

auto UCkInventory_GetItemTrait_K2Node::DoExpandNode(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    ECk_ValidInvalid InNodeValidity) -> void
{
    namespace ns = ck_k2_node_get_item_trait;

    const auto* TraitClass = Get_TraitClassFromPin();

    if (ck::Is_NOT_Valid(TraitClass))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Invalid Trait Type", "Invalid Trait Type. @@").ToString(), this);
        return;
    }

    // ---- Get Definition from Item ----

    auto* GetDefinition_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetDefinition_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_InventoryItem_UE, Get_Definition),
        UCk_Utils_InventoryItem_UE::StaticClass()
    );
    GetDefinition_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetDefinition_Node, this);

    // ---- Get Has_ItemTrait from Definition ----

    auto* HasTrait_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    HasTrait_Node->FunctionReference.SetExternalMember(
        FName(TEXT("Has_ItemTrait")),
        UCk_InventoryItem_Definition::StaticClass()
    );
    HasTrait_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(HasTrait_Node, this);

    if (auto* ClassPin = HasTrait_Node->FindPin(TEXT("InTraitClass"));
        ck::IsValid(ClassPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        ClassPin->DefaultObject = const_cast<UClass*>(TraitClass);
    }

    // ---- Get_ItemTrait from Definition ----

    auto* GetTrait_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetTrait_Node->FunctionReference.SetExternalMember(
        FName(TEXT("Get_ItemTrait")),
        UCk_InventoryItem_Definition::StaticClass()
    );
    GetTrait_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetTrait_Node, this);

    if (auto* ClassPin = GetTrait_Node->FindPin(TEXT("InTraitClass"));
        ck::IsValid(ClassPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        ClassPin->DefaultObject = const_cast<UClass*>(TraitClass);
    }

    // ---- Branch node ----

    auto* BranchNode = InCompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, InSourceGraph);
    BranchNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(BranchNode, this);

    // ---- Wire: Item Handle -> GetDefinition ----

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_ItemHandle, ECk_EditorGraph_PinDirection::Input, *this),
                GetDefinition_Node->FindPin(TEXT("InItem"))
            }
        },
        ECk_EditorGraph_PinLinkType::Copy
    ) == ECk_SucceededFailed::Failed) { return; }

    // ---- Wire: Exec -> GetDefinition ----

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetDefinition_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    // ---- Wire: GetDefinition -> HasTrait (definition result) ----

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*GetDefinition_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*HasTrait_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*GetDefinition_Node),
                HasTrait_Node->FindPin(TEXT("self"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // ---- Wire: HasTrait -> Branch ----

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*HasTrait_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*BranchNode)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*HasTrait_Node),
                BranchNode->FindPin(TEXT("Condition"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // ---- Wire: Branch Then -> GetTrait ----

    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                BranchNode->FindPin(TEXT("Then")),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*GetTrait_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*GetDefinition_Node),
                GetTrait_Node->FindPin(TEXT("self"))
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // ---- Wire: GetTrait result -> our Trait output pin ----

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_Trait, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*GetTrait_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    // ---- Wire: Execution outputs ----

    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_Valid, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*GetTrait_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ns::PinName_Invalid, ECk_EditorGraph_PinDirection::Output, *this),
                BranchNode->FindPin(TEXT("Else"))
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    BreakAllNodeLinks();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCkInventory_GetItemTrait_K2Node::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(
        TEXT("UCkInventory_GetItemTrait_K2Node"),
        TEXT("[Ck][Item] Get Item Trait...")
    );
}

auto UCkInventory_GetItemTrait_K2Node::DoValidateNodePins(
    const TOptional<FKismetCompilerContext*>& InCompilerContext) const -> ECk_ValidInvalid
{
    if (ck::Is_NOT_Valid(Get_TraitClassFromPin()))
    {
        if (InCompilerContext.IsSet())
        {
            InCompilerContext.GetValue()->MessageLog.Error(
                *LOCTEXT("No Trait Type Selected", "No Item Trait Type selected. @@").ToString(), this
            );
        }
        return ECk_ValidInvalid::Invalid;
    }

    return ECk_ValidInvalid::Valid;
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
