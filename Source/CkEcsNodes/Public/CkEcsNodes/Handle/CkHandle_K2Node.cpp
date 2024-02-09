#include "CkHandle_K2Node.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include <EdGraphSchema_K2.h>
#include <BlueprintNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>
#include <StructUtilsFunctionLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkHandle_K2Node::
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
    -> void
{
    Super::GetMenuActions(InActionRegistrar);
    UClass* Action = GetClass();

    if (InActionRegistrar.IsOpenForRegistration(Action))
    {
        auto CustomizeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, const FName FunctionName)
        {
            UCkHandle_K2Node* Node = CastChecked<UCkHandle_K2Node>(NewNode);
            UFunction* Function = UCk_Utils_Handle_UE::StaticClass()->FindFunctionByName(FunctionName);
            check(Function);
            Node->SetFromFunction(Function);
        };

        UBlueprintNodeSpawner* MakeNodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(MakeNodeSpawner != nullptr);
        MakeNodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Handle_UE, Get_RawHandle));
        InActionRegistrar.AddBlueprintAction(Action, MakeNodeSpawner);
    }
}

auto
    UCkHandle_K2Node::
    IsNodePure() const
    -> bool
{
    return true;
}

auto
    UCkHandle_K2Node::
    IsConnectionDisallowed(
        const UEdGraphPin* InMyPin,
        const UEdGraphPin* InOtherPin,
        FString& OutReason) const
    -> bool
{
    const UEdGraphPin* ValuePin = FindPinChecked(FName(TEXT("InHandle")));

    if (InMyPin == ValuePin && InMyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if (InOtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct)
        {
            OutReason = TEXT("Value must be a struct.");
            return true;
        }

        // TODO: Add necessary additional checks here
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
