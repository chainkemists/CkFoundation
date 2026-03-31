#include "CkCVar_K2Node_Bind.h"

#include "CkCVar_TypeDetection.h"
#include "CkCVar/Utils/CkCVar_Utils.h"
#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include <Editor.h>
#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include <K2Node_CustomEvent.h>
#include <KismetCompiler.h>
#include <Kismet2/BlueprintEditorUtils.h>

// --------------------------------------------------------------------------------------------------------------------

namespace CVar_Bind_Pins
{
    const FName CVarRef_Pin  = TEXT("CVar");
    const FName Policy_Pin   = TEXT("CallbackPolicy");
    const FName OnChanged_Pin = TEXT("OnChanged");
    const FName NewValue_Pin = TEXT("NewValue");
    const FName Handle_Pin   = TEXT("CallbackHandle");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_CVar_Bind::
    GetNodeTitle(ENodeTitleType::Type InTitleType) const
    -> FText
{
    return FText::FromString(TEXT("[Ck] CVar Bind To Changed ⚙️"));
}

auto
    UCk_K2Node_CVar_Bind::
    GetNodeTitleColor() const
    -> FLinearColor
{
    return FLinearColor(0.804f, 0.498f, 0.196f);
}

auto
    UCk_K2Node_CVar_Bind::
    GetIconAndTint(FLinearColor& OutColor) const
    -> FSlateIcon
{
    OutColor = FLinearColor::White;
    static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
    return Icon;
}

auto
    UCk_K2Node_CVar_Bind::
    GetMenuCategory() const
    -> FText
{
    return FText::FromString(TEXT("Ck|Utils|CVar"));
}

auto
    UCk_K2Node_CVar_Bind::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return FText::FromString(TEXT("[Ck] CVar Bind To Changed ⚙️"));
}

auto
    UCk_K2Node_CVar_Bind::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_CVar_Bind::
    DoAllocate_DefaultPins()
    -> void
{
    // Input: CVarRef with dropdown
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct,
        FCk_CVarRef::StaticStruct(), CVar_Bind_Pins::CVarRef_Pin);

    // Input: callback policy
    {
        auto* PolicyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte,
            StaticEnum<ECk_CVar_InitialCallbackPolicy>(), CVar_Bind_Pins::Policy_Pin);
        PolicyPin->DefaultValue = TEXT("FireImmediately");
    }

    // Output: callback handle (belongs to main exec flow, so above On Changed)
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct,
        FCk_CVarCallbackHandle::StaticStruct(), CVar_Bind_Pins::Handle_Pin);

    // Output: "On Changed" exec pin
    {
        auto* OnChangedPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, CVar_Bind_Pins::OnChanged_Pin);
        OnChangedPin->PinFriendlyName = FText::FromString(TEXT("On Changed"));
    }

    // Output: typed "New Value" pin
    if (_DetectedType.IsSet())
    {
        switch (_DetectedType.GetValue())
        {
            case ECk_CVarType::Int32:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, CVar_Bind_Pins::NewValue_Pin);
                break;
            case ECk_CVarType::Float:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, CVar_Bind_Pins::NewValue_Pin);
                break;
            case ECk_CVarType::Bool:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, CVar_Bind_Pins::NewValue_Pin);
                break;
            case ECk_CVarType::String:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, CVar_Bind_Pins::NewValue_Pin);
                break;
        }
    }
    else
    {
        CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, CVar_Bind_Pins::NewValue_Pin);
    }
}

auto
    UCk_K2Node_CVar_Bind::
    DoPinDefaultValueChanged(
        UEdGraphPin* InPin)
    -> void
{
    if (InPin != nullptr && InPin->PinName == CVar_Bind_Pins::CVarRef_Pin)
    {
        const auto OldType = _DetectedType;
        UpdateDetectedType();

        if (OldType != _DetectedType)
        {
            auto WeakThis = TWeakObjectPtr<UCk_K2Node_CVar_Bind>(this);
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
    UCk_K2Node_CVar_Bind::
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

    // 1. Create internal CustomEvent
    auto* CustomEventNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, InSourceGraph);
    CustomEventNode->CustomFunctionName = *InCompilerContext.GetGuid(CustomEventNode);
    CustomEventNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CustomEventNode, this);

    // Add typed "NewValue" output on CustomEvent
    auto NewValuePinType = FEdGraphPinType{};
    switch (_DetectedType.GetValue())
    {
        case ECk_CVarType::Int32:
            NewValuePinType.PinCategory = UEdGraphSchema_K2::PC_Int;
            break;
        case ECk_CVarType::Float:
            NewValuePinType.PinCategory = UEdGraphSchema_K2::PC_Real;
            NewValuePinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
            break;
        case ECk_CVarType::Bool:
            NewValuePinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
            break;
        case ECk_CVarType::String:
            NewValuePinType.PinCategory = UEdGraphSchema_K2::PC_String;
            break;
    }
    auto* CustomEventNewValuePin = CustomEventNode->CreateUserDefinedPin(
        TEXT("NewValue"), NewValuePinType, EGPD_Output);

    // 2. Create CallFunction node for INTERNAL_Bind_[Type]
    auto* CallNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    CallNode->FunctionReference.SetExternalMember(
        GetInternalFunctionNameForType(),
        UCk_Utils_CVar_UE::StaticClass());
    CallNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallNode, this);

    // 3. Connect CustomEvent delegate → CallFunction delegate input
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_OutputDelegate(*CustomEventNode),
                CallNode->FindPin(TEXT("InCallback"))
            },
        }) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    // 4. Wire "On Changed" exec → CustomEvent exec
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(CVar_Bind_Pins::OnChanged_Pin, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*CustomEventNode)
            },
        },
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    // 5. Wire "New Value" output → CustomEvent output
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(CVar_Bind_Pins::NewValue_Pin, ECk_EditorGraph_PinDirection::Output, *this),
                CustomEventNewValuePin
            },
        },
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    // 6. Wire exec flow
    InCompilerContext.MovePinLinksToIntermediate(
        *FindPinChecked(UEdGraphSchema_K2::PN_Execute),
        *CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
    InCompilerContext.MovePinLinksToIntermediate(
        *FindPinChecked(UEdGraphSchema_K2::PN_Then),
        *CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));

    // 7. Wire CVarRef input
    auto* MyCVarPin = FindPinChecked(CVar_Bind_Pins::CVarRef_Pin);
    auto* CallCVarPin = CallNode->FindPin(TEXT("InRef"));
    if (CallCVarPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyCVarPin, *CallCVarPin);
        if (MyCVarPin->LinkedTo.Num() == 0 && NOT MyCVarPin->DefaultValue.IsEmpty())
        {
            CallCVarPin->DefaultValue = MyCVarPin->DefaultValue;
        }
    }

    // 8. Wire Policy input
    auto* MyPolicyPin = FindPinChecked(CVar_Bind_Pins::Policy_Pin);
    auto* CallPolicyPin = CallNode->FindPin(TEXT("InPolicy"));
    if (CallPolicyPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyPolicyPin, *CallPolicyPin);
        if (MyPolicyPin->LinkedTo.Num() == 0 && NOT MyPolicyPin->DefaultValue.IsEmpty())
        {
            CallPolicyPin->DefaultValue = MyPolicyPin->DefaultValue;
        }
    }

    // 9. Wire output handle
    auto* MyHandlePin = FindPinChecked(CVar_Bind_Pins::Handle_Pin);
    auto* CallReturnPin = CallNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    if (CallReturnPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyHandlePin, *CallReturnPin);
    }

    BreakAllNodeLinks();
}

auto
    UCk_K2Node_CVar_Bind::
    GetInternalFunctionNameForType() const
    -> FName
{
    if (NOT _DetectedType.IsSet())
    {
        return NAME_None;
    }

    switch (_DetectedType.GetValue())
    {
        case ECk_CVarType::Int32:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Bind_Int32);
        case ECk_CVarType::Float:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Bind_Float);
        case ECk_CVarType::Bool:   return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Bind_Bool);
        case ECk_CVarType::String: return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Bind_String);
        default:                   return NAME_None;
    }
}

auto
    UCk_K2Node_CVar_Bind::
    UpdateDetectedType()
    -> void
{
    auto* CVarPin = FindPin(CVar_Bind_Pins::CVarRef_Pin);
    if (CVarPin == nullptr)
    {
        _DetectedType.Reset();
        return;
    }

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

// --------------------------------------------------------------------------------------------------------------------
