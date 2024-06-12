#include "CkAbilityCue_K2Node.h"

#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"

#include "CkCore\Validation\CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include <EdGraphSchema_K2.h>
#include <BlueprintNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>

// --------------------------------------------------------------------------------------------------------------------

auto
   UCk_K2Node_AbilityCue_MakeParamsWithCustomData::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);

    if (const auto* Action = GetClass(); InActionRegistrar.IsOpenForRegistration(Action))
    {
        const auto& CustomizeLambda = [](UEdGraphNode* InNewNode, bool InIsTemplateNode, FName InFunctionName) -> void
        {
            auto* Node = CastChecked<UCk_K2Node_AbilityCue_MakeParamsWithCustomData>(InNewNode);
            const auto* Function = UCk_Utils_AbilityCue_UE::StaticClass()->FindFunctionByName(InFunctionName);
            check(ck::IsValid(Function));

            Node->SetFromFunction(Function);
        };

        auto* Broadcast_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(Broadcast_NodeSpawner));

        Broadcast_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_AbilityCue_UE, INTERNAL__Make_AbilityCue_Params_With_CustomData));

        InActionRegistrar.AddBlueprintAction(Action, Broadcast_NodeSpawner);
    }
}

auto
    UCk_K2Node_AbilityCue_MakeParamsWithCustomData::
    IsNodePure() const
    -> bool
{
    return true;
}

auto
    UCk_K2Node_AbilityCue_MakeParamsWithCustomData::
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
   UCk_K2Node_AbilityCue_MakeWithCustomData::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);

    if (const auto* Action = GetClass(); InActionRegistrar.IsOpenForRegistration(Action))
    {
        const auto& CustomizeLambda = [](UEdGraphNode* InNewNode, bool InIsTemplateNode, FName InFunctionName) -> void
        {
            auto* Node = CastChecked<UCk_K2Node_AbilityCue_MakeWithCustomData>(InNewNode);
            const auto* Function = UCk_Utils_AbilityCue_UE::StaticClass()->FindFunctionByName(InFunctionName);
            check(ck::IsValid(Function));

            Node->SetFromFunction(Function);
        };

        auto* Broadcast_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(Broadcast_NodeSpawner));

        Broadcast_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_AbilityCue_UE, INTERNAL__Make_AbilityCue_With_CustomData));

        InActionRegistrar.AddBlueprintAction(Action, Broadcast_NodeSpawner);
    }
}

auto
    UCk_K2Node_AbilityCue_MakeWithCustomData::
    IsNodePure() const
    -> bool
{
    return true;
}

auto
    UCk_K2Node_AbilityCue_MakeWithCustomData::
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
