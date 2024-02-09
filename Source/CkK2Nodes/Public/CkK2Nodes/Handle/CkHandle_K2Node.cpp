#include "CkHandle_K2Node.h"

#include "CkCore\Validation\CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Typesafe.h"
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

    if (const auto* Action = GetClass(); InActionRegistrar.IsOpenForRegistration(Action))
    {
        const auto& CustomizeLambda = [](UEdGraphNode* InNewNode, bool InIsTemplateNode, FName InFunctionName) -> void
        {
            auto* Node = CastChecked<UCkHandle_K2Node>(InNewNode);
            const auto* Function = UCk_Utils_Handle_UE::StaticClass()->FindFunctionByName(InFunctionName);
            check(ck::IsValid(Function));

            Node->SetFromFunction(Function);
        };

        auto* MakeNodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(ck::IsValid(MakeNodeSpawner));

        MakeNodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
            CustomizeLambda, GET_FUNCTION_NAME_CHECKED(UCk_Utils_Handle_UE, Get_RawHandle));

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
    if (const auto* ValuePin = FindPinChecked(FName(TEXT("InHandle")));
        InMyPin == ValuePin && InMyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if (InOtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct)
        {
            OutReason = TEXT("Value must be a struct!");
            return true;
        }

        if (const auto* StructType = Cast<UScriptStruct>(InOtherPin->PinType.PinSubCategoryObject.Get());
            ck::IsValid(StructType))
        {
            if (NOT StructType->IsChildOf(FCk_Handle_TypeSafe::StaticStruct()))
            {
                OutReason = ck::Format_UE(TEXT("Value must be a child of {}!"), ck::Get_RuntimeTypeToString<FCk_Handle_TypeSafe>());
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
