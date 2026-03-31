#include "CkCVar_K2Node_Get.h"

#include "CkCVar_TypeDetection.h"
#include "CkCVar/Utils/CkCVar_Utils.h"

#include <Editor.h>
#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include <KismetCompiler.h>
#include <Kismet2/BlueprintEditorUtils.h>

// --------------------------------------------------------------------------------------------------------------------

namespace CVar_Get_Pins
{
    const FName CVarRef_Pin = TEXT("CVar");
    const FName Value_Pin   = TEXT("Value");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_CVar_Get::
    GetNodeTitle(ENodeTitleType::Type InTitleType) const
    -> FText
{
    return FText::FromString(TEXT("[Ck] CVar Get ⚙️"));
}

auto
    UCk_K2Node_CVar_Get::
    GetNodeTitleColor() const
    -> FLinearColor
{
    return FLinearColor(0.804f, 0.498f, 0.196f);
}

auto
    UCk_K2Node_CVar_Get::
    GetIconAndTint(FLinearColor& OutColor) const
    -> FSlateIcon
{
    OutColor = FLinearColor::White;
    static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
    return Icon;
}

auto
    UCk_K2Node_CVar_Get::
    GetMenuCategory() const
    -> FText
{
    return FText::FromString(TEXT("Ck|Utils|CVar"));
}

auto
    UCk_K2Node_CVar_Get::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return FText::FromString(TEXT("[Ck] CVar Get ⚙️"));
}

auto
    UCk_K2Node_CVar_Get::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_CVar_Get::
    DoAllocate_DefaultPins()
    -> void
{
    // CVarRef input pin (gets the dropdown via pin factory)
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct,
        FCk_CVarRef::StaticStruct(), CVar_Get_Pins::CVarRef_Pin);

    // Value output pin (typed based on detection)
    CreateValuePinForType();
}

auto
    UCk_K2Node_CVar_Get::
    DoPinDefaultValueChanged(
        UEdGraphPin* InPin)
    -> void
{
    if (InPin != nullptr && InPin->PinName == CVar_Get_Pins::CVarRef_Pin)
    {
        const auto OldType = _DetectedType;
        UpdateDetectedType();

        if (OldType != _DetectedType)
        {
            // Defer reconstruction — we're inside a Slate widget callback,
            // and ReconstructNode destroys the widget that triggered this.
            auto WeakThis = TWeakObjectPtr<UCk_K2Node_CVar_Get>(this);
            GEditor->GetTimerManager()->SetTimerForNextTick([WeakThis]()
            {
                if (auto* Node = WeakThis.Get())
                {
                    Node->ReconstructNode();
                }
            });
        }
    }
}

auto
    UCk_K2Node_CVar_Get::
    DoExpandNode(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)
    -> void
{
    if (InNodeValidity == ECk_ValidInvalid::Invalid || NOT _DetectedType.IsSet())
    {
        InCompilerContext.MessageLog.Error(
            *FText::FromString(TEXT("Cannot determine CVar type. Select a valid CVar. @@")).ToString(), this);
        return;
    }

    if (_DetectedType.GetValue() == ECk_CVarType::Command)
    {
        InCompilerContext.MessageLog.Error(
            *FText::FromString(TEXT("Cannot Get value of a console command. Use Get with console variables only. @@")).ToString(), this);
        return;
    }

    auto* CallNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    CallNode->FunctionReference.SetExternalMember(
        GetInternalFunctionNameForType(),
        UCk_Utils_CVar_UE::StaticClass());
    CallNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallNode, this);

    // Wire CVarRef input
    auto* MyCVarPin = FindPinChecked(CVar_Get_Pins::CVarRef_Pin);
    auto* CallCVarPin = CallNode->FindPin(TEXT("InRef"));
    if (CallCVarPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyCVarPin, *CallCVarPin);
        if (MyCVarPin->LinkedTo.Num() == 0 && NOT MyCVarPin->DefaultValue.IsEmpty())
        {
            CallCVarPin->DefaultValue = MyCVarPin->DefaultValue;
        }
    }

    // Wire Value output
    auto* MyValuePin = FindPinChecked(CVar_Get_Pins::Value_Pin);
    auto* CallReturnPin = CallNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    if (CallReturnPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyValuePin, *CallReturnPin);
    }

    BreakAllNodeLinks();
}

auto
    UCk_K2Node_CVar_Get::
    GetInternalFunctionNameForType() const
    -> FName
{
    if (NOT _DetectedType.IsSet())
    {
        return NAME_None;
    }

    switch (_DetectedType.GetValue())
    {
        case ECk_CVarType::Int32:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Get_Int32);
        case ECk_CVarType::Float:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Get_Float);
        case ECk_CVarType::Bool:   return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Get_Bool);
        case ECk_CVarType::String: return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Get_String);
        default:                   return NAME_None;
    }
}

auto
    UCk_K2Node_CVar_Get::
    UpdateDetectedType()
    -> void
{
    auto* CVarPin = FindPin(CVar_Get_Pins::CVarRef_Pin);
    if (CVarPin == nullptr)
    {
        _DetectedType.Reset();
        return;
    }

    // Parse CVarRef from pin default
    const auto& DefaultString = CVarPin->GetDefaultAsString();
    if (DefaultString.IsEmpty())
    {
        _DetectedType.Reset();
        return;
    }

    auto Ref = FCk_CVarRef{};
    FCk_CVarRef::StaticStruct()->ImportText(
        *DefaultString, &Ref, nullptr, PPF_SerializedAsImportText, GError,
        FCk_CVarRef::StaticStruct()->GetName(), true);

    if (NOT Ref.IsValid())
    {
        _DetectedType.Reset();
        return;
    }

    _DetectedType = ck::cvar::DetectCVarType(Ref.Get_Name());
}

auto
    UCk_K2Node_CVar_Get::
    CreateValuePinForType()
    -> void
{
    if (NOT _DetectedType.IsSet())
    {
        CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, CVar_Get_Pins::Value_Pin);
        return;
    }

    switch (_DetectedType.GetValue())
    {
        case ECk_CVarType::Int32:
            CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, CVar_Get_Pins::Value_Pin);
            break;
        case ECk_CVarType::Float:
            CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, CVar_Get_Pins::Value_Pin);
            break;
        case ECk_CVarType::Bool:
            CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, CVar_Get_Pins::Value_Pin);
            break;
        case ECk_CVarType::String:
            CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, CVar_Get_Pins::Value_Pin);
            break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
