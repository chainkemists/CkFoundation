#include "CkUnrealVariables_K2Node.h"

#include "CkCore\Validation\CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

#include <EdGraphSchema_K2.h>
#include <BlueprintNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_Variables_GetInstancedStruct::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);

    if (const auto* Action = GetClass(); InActionRegistrar.IsOpenForRegistration(Action))
    {
        const auto& CustomizeLambda = [](UEdGraphNode* InNewNode, bool InIsTemplateNode, FName InFunctionName) -> void
        {
            auto* Node = CastChecked<UCk_K2Node_Variables_GetInstancedStruct>(InNewNode);
            const auto* Function = UCk_Utils_Variables_InstancedStruct_UE::StaticClass()->FindFunctionByName(InFunctionName);
            check(ck::IsValid(Function));

            Node->SetFromFunction(Function);
        };

        auto* GetValueByName_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(GetValueByName_NodeSpawner));

        GetValueByName_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Variables_InstancedStruct_UE, INTERNAL__Get_ByName));

        InActionRegistrar.AddBlueprintAction(Action, GetValueByName_NodeSpawner);

        auto* GetValueByTag_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(GetValueByName_NodeSpawner));

        GetValueByTag_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Variables_InstancedStruct_UE, INTERNAL__Get));

        InActionRegistrar.AddBlueprintAction(Action, GetValueByTag_NodeSpawner);
    }
}

auto
    UCk_K2Node_Variables_GetInstancedStruct::
    IsNodePure() const
    -> bool
{
    return true;
}

auto
    UCk_K2Node_Variables_GetInstancedStruct::
    IsConnectionDisallowed(
        const UEdGraphPin* InMyPin,
        const UEdGraphPin* InOtherPin,
        FString& OutReason) const
    -> bool
{
    if (const auto* ValuePin = FindPinChecked(FName(TEXT("OutValue")));
        InMyPin == ValuePin && InMyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if (InOtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct)
        {
            OutReason = TEXT("Value must be a struct.");
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_Variables_SetInstancedStruct::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);

    if (const auto* Action = GetClass(); InActionRegistrar.IsOpenForRegistration(Action))
    {
        const auto& CustomizeLambda = [](UEdGraphNode* InNewNode, bool InIsTemplateNode, FName InFunctionName) -> void
        {
            auto* Node = CastChecked<UCk_K2Node_Variables_SetInstancedStruct>(InNewNode);
            const auto* Function = UCk_Utils_Variables_InstancedStruct_UE::StaticClass()->FindFunctionByName(InFunctionName);
            check(ck::IsValid(Function));

            Node->SetFromFunction(Function);
        };

        auto* SetValueByName_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(SetValueByName_NodeSpawner));

        SetValueByName_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Variables_InstancedStruct_UE, INTERNAL__Set_ByName));

        InActionRegistrar.AddBlueprintAction(Action, SetValueByName_NodeSpawner);

        auto* SetValueByTag_NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(SetValueByName_NodeSpawner));

        SetValueByTag_NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Variables_InstancedStruct_UE, INTERNAL__Set));

        InActionRegistrar.AddBlueprintAction(Action, SetValueByTag_NodeSpawner);
    }
}

auto
    UCk_K2Node_Variables_SetInstancedStruct::
    IsNodePure() const
    -> bool
{
    return false;
}

auto
    UCk_K2Node_Variables_SetInstancedStruct::
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
            OutReason = TEXT("Value must be a struct.");
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
