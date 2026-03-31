#include "CkCVar_K2Node_Set.h"

#include "CkCVar_TypeDetection.h"
#include "CkCVar/Utils/CkCVar_Utils.h"

#include <Editor.h>
#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include <KismetCompiler.h>
#include <Kismet2/BlueprintEditorUtils.h>

// --------------------------------------------------------------------------------------------------------------------

namespace CVar_Set_Pins
{
    const FName CVarRef_Pin = TEXT("CVar");
    const FName Value_Pin   = TEXT("Value");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_CVar_Set::
    GetNodeTitle(ENodeTitleType::Type InTitleType) const
    -> FText
{
    return FText::FromString(TEXT("[Ck] CVar Set ⚙️"));
}

auto
    UCk_K2Node_CVar_Set::
    GetNodeTitleColor() const
    -> FLinearColor
{
    return FLinearColor(0.804f, 0.498f, 0.196f);
}

auto
    UCk_K2Node_CVar_Set::
    GetIconAndTint(FLinearColor& OutColor) const
    -> FSlateIcon
{
    OutColor = FLinearColor::White;
    static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
    return Icon;
}

auto
    UCk_K2Node_CVar_Set::
    GetMenuCategory() const
    -> FText
{
    return FText::FromString(TEXT("Ck|Utils|CVar"));
}

auto
    UCk_K2Node_CVar_Set::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return FText::FromString(TEXT("[Ck] CVar Set ⚙️"));
}

auto
    UCk_K2Node_CVar_Set::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    const auto PropertyName = PropertyChangedEvent.Property != nullptr
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_CVar_Set, _ManualType))
    {
        if (_IsRuntimeCVar)
        {
            ReconstructNode();
            GetGraph()->NotifyGraphChanged();
        }
    }
}

auto
    UCk_K2Node_CVar_Set::
    PinConnectionListChanged(
        UEdGraphPin* InPin)
    -> void
{
    Super::PinConnectionListChanged(InPin);

    if (InPin != nullptr && InPin->PinName == CVar_Set_Pins::CVarRef_Pin)
    {
        const auto OldType = _DetectedType;
        UpdateDetectedType();

        if (OldType != _DetectedType)
        {
            auto WeakThis = TWeakObjectPtr<UCk_K2Node_CVar_Set>(this);
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
    UCk_K2Node_CVar_Set::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    for (auto* OldPin : InOldPins)
    {
        if (OldPin != nullptr && OldPin->PinName == CVar_Set_Pins::CVarRef_Pin)
        {
            if (OldPin->LinkedTo.Num() > 0)
            {
                _IsRuntimeCVar = true;
                _DetectedType = _ManualType;
            }
            else
            {
                _IsRuntimeCVar = false;
                const auto& DefaultString = OldPin->GetDefaultAsString();
                if (NOT DefaultString.IsEmpty())
                {
                    auto Ref = FCk_CVarRef{};
                    FCk_CVarRef::StaticStruct()->ImportText(
                        *DefaultString, &Ref, nullptr, PPF_SerializedAsImportText, GError,
                        FCk_CVarRef::StaticStruct()->GetName(), true);

                    if (Ref.IsValid())
                    {
                        _DetectedType = ck::cvar::DetectCVarType(Ref.Get_Name());
                    }
                }
            }
            break;
        }
    }

    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_CVar_Set::
    DoAllocate_DefaultPins()
    -> void
{
    // CVarRef input pin
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct,
        FCk_CVarRef::StaticStruct(), CVar_Set_Pins::CVarRef_Pin);

    // Value input pin (typed based on detection)
    CreateValuePinForType();
}

auto
    UCk_K2Node_CVar_Set::
    DoPinDefaultValueChanged(
        UEdGraphPin* InPin)
    -> void
{
    if (InPin != nullptr && InPin->PinName == CVar_Set_Pins::CVarRef_Pin)
    {
        const auto OldType = _DetectedType;
        UpdateDetectedType();

        if (OldType != _DetectedType)
        {
            auto WeakThis = TWeakObjectPtr<UCk_K2Node_CVar_Set>(this);
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
    UCk_K2Node_CVar_Set::
    DoExpandNode(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)
    -> void
{
    UpdateDetectedType();

    if (InNodeValidity == ECk_ValidInvalid::Invalid || NOT _DetectedType.IsSet())
    {
        InCompilerContext.MessageLog.Error(
            *FText::FromString(TEXT("Cannot determine CVar type. Select a valid CVar. @@")).ToString(), this);
        return;
    }

    if (_DetectedType.GetValue() == ECk_CVarType::Command)
    {
        InCompilerContext.MessageLog.Error(
            *FText::FromString(TEXT("Cannot Set value of a console command. Use Set with console variables only. @@")).ToString(), this);
        return;
    }

    auto* CallNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    CallNode->FunctionReference.SetExternalMember(
        GetInternalFunctionNameForType(),
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

    // Wire CVarRef
    auto* MyCVarPin = FindPinChecked(CVar_Set_Pins::CVarRef_Pin);
    auto* CallCVarPin = CallNode->FindPin(TEXT("InRef"));
    if (CallCVarPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyCVarPin, *CallCVarPin);
        if (MyCVarPin->LinkedTo.Num() == 0 && NOT MyCVarPin->DefaultValue.IsEmpty())
        {
            CallCVarPin->DefaultValue = MyCVarPin->DefaultValue;
        }
    }

    // Wire Value
    auto* MyValuePin = FindPinChecked(CVar_Set_Pins::Value_Pin);
    auto* CallValuePin = CallNode->FindPin(TEXT("InValue"));
    if (CallValuePin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyValuePin, *CallValuePin);
        if (MyValuePin->LinkedTo.Num() == 0 && NOT MyValuePin->DefaultValue.IsEmpty())
        {
            CallValuePin->DefaultValue = MyValuePin->DefaultValue;
        }
    }

    BreakAllNodeLinks();
}

auto
    UCk_K2Node_CVar_Set::
    GetInternalFunctionNameForType() const
    -> FName
{
    if (NOT _DetectedType.IsSet())
    {
        return NAME_None;
    }

    switch (_DetectedType.GetValue())
    {
        case ECk_CVarType::Int32:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Set_Int32);
        case ECk_CVarType::Float:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Set_Float);
        case ECk_CVarType::Bool:   return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Set_Bool);
        case ECk_CVarType::String: return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Set_String);
        default:                   return NAME_None;
    }
}

auto
    UCk_K2Node_CVar_Set::
    UpdateDetectedType()
    -> void
{
    auto* CVarPin = FindPin(CVar_Set_Pins::CVarRef_Pin);
    if (CVarPin == nullptr)
    {
        _IsRuntimeCVar = false;
        _DetectedType.Reset();
        return;
    }

    // Runtime CVar: pin has a linked connection, type must be selected manually
    if (CVarPin->LinkedTo.Num() > 0)
    {
        _IsRuntimeCVar = true;
        _DetectedType = _ManualType;
        return;
    }

    _IsRuntimeCVar = false;

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
    UCk_K2Node_CVar_Set::
    CreateValuePinForType()
    -> void
{
    if (NOT _DetectedType.IsSet())
    {
        CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, CVar_Set_Pins::Value_Pin);
        return;
    }

    switch (_DetectedType.GetValue())
    {
        case ECk_CVarType::Int32:
            CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, CVar_Set_Pins::Value_Pin);
            break;
        case ECk_CVarType::Float:
            CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, CVar_Set_Pins::Value_Pin);
            break;
        case ECk_CVarType::Bool:
            CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, CVar_Set_Pins::Value_Pin);
            break;
        case ECk_CVarType::String:
            CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, CVar_Set_Pins::Value_Pin);
            break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
