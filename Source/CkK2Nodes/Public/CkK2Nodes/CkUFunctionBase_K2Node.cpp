#include "CkUFunctionBase_K2Node.h"

#include <Internationalization/Internationalization.h>
#include <KismetCompiler.h>
#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <K2Node_CallFunction.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_UFunction_Base::
    AllocateDefaultPins()
    -> void
{
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    DoAllocate_DefaultPins();

    Super::AllocateDefaultPins();
}

auto
    UCk_K2Node_UFunction_Base::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);

    const auto& Action = GetClass();

    if (NOT InActionRegistrar.IsOpenForRegistration(Action))
    { return; }

    auto* Spawner = UCk_K2NodeSpawner_UFunction_Base::Create
    (
        GetClass(),
        DoGet_Menu_NodeTitle(),
        DoGet_Menu_SlateIcon()
    );
    check(Spawner != nullptr);

    InActionRegistrar.AddBlueprintAction(Action, Spawner);
}

auto
    UCk_K2Node_UFunction_Base::
    ExpandNode(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph)
 -> void
{
    Super::ExpandNode(InCompilerContext, InSourceGraph);

    const auto& NodeValidity = DoValidateNodePins(&InCompilerContext);
    DoExpandNode(InCompilerContext, InSourceGraph, NodeValidity);
}

auto
    UCk_K2Node_UFunction_Base::
    PinDefaultValueChanged(
        UEdGraphPin* InPin)
    -> void
{
    Super::PinDefaultValueChanged(InPin);

    DoPinDefaultValueChanged(InPin);
    ReconstructNode();
    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2NodeSpawner_UFunction_Base::
    Create(
        TSubclassOf<UK2Node> InNodeClass,
        FText InNodeName,
        FSlateIcon InSlateIcon,
        UObject* InOuter)
    -> UCk_K2NodeSpawner_UFunction_Base*
{
    if (ck::Is_NOT_Valid(InOuter, ck::IsValid_Policy_NullptrOnly{}))
    {
        InOuter = GetTransientPackage();
    }

    auto* NodeSpawner = NewObject<ThisType>(InOuter);
    NodeSpawner->NodeClass = InNodeClass;

    auto& MenuSignature = NodeSpawner->DefaultMenuSignature;

    MenuSignature.MenuName = InNodeName;
    MenuSignature.Icon     = InSlateIcon;

    return NodeSpawner;
}

// --------------------------------------------------------------------------------------------------------------------

