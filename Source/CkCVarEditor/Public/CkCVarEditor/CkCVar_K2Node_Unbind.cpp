#include "CkCVar_K2Node_Unbind.h"

#include "CkCVar/Utils/CkCVar_Utils.h"

#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include <KismetCompiler.h>

// --------------------------------------------------------------------------------------------------------------------

namespace CVar_Unbind_Pins
{
    const FName Handle_Pin = TEXT("CallbackHandle");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_CVar_Unbind::
    GetNodeTitle(ENodeTitleType::Type InTitleType) const
    -> FText
{
    return FText::FromString(TEXT("[Ck][CVar] Unbind"));
}

auto
    UCk_K2Node_CVar_Unbind::
    GetMenuCategory() const
    -> FText
{
    return FText::FromString(TEXT("Ck|CVar"));
}

auto
    UCk_K2Node_CVar_Unbind::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return FText::FromString(TEXT("[Ck][CVar] Unbind"));
}

auto
    UCk_K2Node_CVar_Unbind::
    DoAllocate_DefaultPins()
    -> void
{
    // Callback handle input
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct,
        FCk_CVarCallbackHandle::StaticStruct(), CVar_Unbind_Pins::Handle_Pin);
}

auto
    UCk_K2Node_CVar_Unbind::
    DoExpandNode(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)
    -> void
{
    if (InNodeValidity == ECk_ValidInvalid::Invalid)
    {
        return;
    }

    auto* CallNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    CallNode->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Unbind),
        UCk_Utils_CVar_UE::StaticClass());
    CallNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallNode, this);

    // Wire exec
    InCompilerContext.MovePinLinksToIntermediate(
        *FindPinChecked(UEdGraphSchema_K2::PN_Execute),
        *CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
    InCompilerContext.MovePinLinksToIntermediate(
        *FindPinChecked(UEdGraphSchema_K2::PN_Then),
        *CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));

    // Wire handle
    auto* MyHandlePin = FindPinChecked(CVar_Unbind_Pins::Handle_Pin);
    auto* CallHandlePin = CallNode->FindPin(TEXT("InHandle"));
    if (CallHandlePin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyHandlePin, *CallHandlePin);
        if (MyHandlePin->LinkedTo.Num() == 0 && NOT MyHandlePin->DefaultValue.IsEmpty())
        {
            CallHandlePin->DefaultValue = MyHandlePin->DefaultValue;
        }
    }

    BreakAllNodeLinks();
}

// --------------------------------------------------------------------------------------------------------------------
