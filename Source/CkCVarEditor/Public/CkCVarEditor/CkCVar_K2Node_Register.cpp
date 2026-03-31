#include "CkCVar_K2Node_Register.h"

#include "CkCVar/Settings/CkCVar_Settings.h"
#include "CkCVar/Utils/CkCVar_Utils.h"
#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include <BlueprintActionDatabaseRegistrar.h>
#include <BlueprintNodeSpawner.h>
#include <EdGraphSchema_K2.h>
#include <K2Node_CallFunction.h>
#include <K2Node_CustomEvent.h>
#include <KismetCompiler.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace CVar_Register_Pins
{
    const FName Name_Pin       = TEXT("Name");
    const FName Default_Pin    = TEXT("DefaultValue");
    const FName Help_Pin       = TEXT("Help");
    const FName Policy_Pin     = TEXT("CallbackPolicy");
    const FName OnChanged_Pin  = TEXT("OnChanged");
    const FName NewValue_Pin   = TEXT("NewValue");
    const FName Handle_Pin     = TEXT("CallbackHandle");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_CVar_Register::
    GetNodeTitle(ENodeTitleType::Type InTitleType) const
    -> FText
{
    return FText::FromString(TEXT("[Ck][CVar] Register"));
}

auto
    UCk_K2Node_CVar_Register::
    GetMenuCategory() const
    -> FText
{
    return FText::FromString(TEXT("Ck|CVar"));
}

auto
    UCk_K2Node_CVar_Register::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return FText::FromString(TEXT("[Ck][CVar] Register"));
}

auto
    UCk_K2Node_CVar_Register::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    const auto PropertyName = PropertyChangedEvent.Property != nullptr
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_CVar_Register, _CVarType))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto
    UCk_K2Node_CVar_Register::
    PostReconstructNode()
    -> void
{
    Super::PostReconstructNode();

    RegisterCVarFromPinDefaults();
}

auto
    UCk_K2Node_CVar_Register::
    ReallocatePinsDuringReconstruction(
        TArray<UEdGraphPin*>& InOldPins)
    -> void
{
    AllocateDefaultPins();
    RestoreSplitPins(InOldPins);
}

auto
    UCk_K2Node_CVar_Register::
    DoAllocate_DefaultPins()
    -> void
{
    const auto IsCommand = _CVarType == ECk_CVarType::Command;

    // Input pins
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, CVar_Register_Pins::Name_Pin);

    // Typed default value pin (not applicable for commands)
    if (NOT IsCommand)
    {
        CreateValuePinForType(EGPD_Input);
    }

    // Help text pin
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, CVar_Register_Pins::Help_Pin);

    // Callback policy enum pin (not applicable for commands — they always fire on invoke)
    if (NOT IsCommand)
    {
        auto* PolicyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte,
            StaticEnum<ECk_CVar_InitialCallbackPolicy>(), CVar_Register_Pins::Policy_Pin);
        PolicyPin->DefaultValue = TEXT("FireImmediately");
    }

    // Output: callback handle (belongs to main exec flow, so above On Changed)
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct,
        FCk_CVarCallbackHandle::StaticStruct(), CVar_Register_Pins::Handle_Pin);

    // Output: "On Changed" / "On Executed" exec pin
    {
        auto* OnChangedPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, CVar_Register_Pins::OnChanged_Pin);
        OnChangedPin->PinFriendlyName = IsCommand
            ? FText::FromString(TEXT("On Executed"))
            : FText::FromString(TEXT("On Changed"));
    }

    // Output: typed "New Value" pin (not applicable for commands)
    if (NOT IsCommand)
    {
        switch (_CVarType)
        {
            case ECk_CVarType::Int32:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, CVar_Register_Pins::NewValue_Pin);
                break;
            case ECk_CVarType::Float:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, CVar_Register_Pins::NewValue_Pin);
                break;
            case ECk_CVarType::Bool:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, CVar_Register_Pins::NewValue_Pin);
                break;
            case ECk_CVarType::String:
                CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, CVar_Register_Pins::NewValue_Pin);
                break;
            default: break;
        }
    }
}

auto
    UCk_K2Node_CVar_Register::
    DoPinDefaultValueChanged(
        UEdGraphPin* InPin)
    -> void
{
    if (InPin != nullptr && InPin->PinName == CVar_Register_Pins::Name_Pin)
    {
        RegisterCVarFromPinDefaults();
    }
}

auto
    UCk_K2Node_CVar_Register::
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

    const auto IsCommand = _CVarType == ECk_CVarType::Command;

    // 1. Create internal CustomEvent
    auto* CustomEventNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, InSourceGraph);
    CustomEventNode->CustomFunctionName = *InCompilerContext.GetGuid(CustomEventNode);
    CustomEventNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CustomEventNode, this);

    // Add a typed "NewValue" output pin on the CustomEvent (not for commands)
    UEdGraphPin* CustomEventNewValuePin = nullptr;
    if (NOT IsCommand)
    {
        auto NewValuePinType = FEdGraphPinType{};
        switch (_CVarType)
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
            default: break;
        }
        CustomEventNewValuePin = CustomEventNode->CreateUserDefinedPin(
            TEXT("NewValue"), NewValuePinType, EGPD_Output);
    }

    // 2. Create CallFunction node for INTERNAL_Register_[Type] or INTERNAL_Register_Command
    auto* CallNode = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    CallNode->FunctionReference.SetExternalMember(
        GetInternalFunctionNameForType(),
        UCk_Utils_CVar_UE::StaticClass());
    CallNode->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallNode, this);

    // 3. Connect CustomEvent's output delegate → CallFunction's InCallback delegate input
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

    // 4. Wire our "On Changed"/"On Executed" exec output → CustomEvent's Then exec
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(CVar_Register_Pins::OnChanged_Pin, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*CustomEventNode)
            },
        },
        ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
    {
        return;
    }

    // 5. Wire our "New Value" output → CustomEvent's NewValue output (not for commands)
    if (NOT IsCommand && CustomEventNewValuePin != nullptr)
    {
        if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
            InCompilerContext,
            {
                {
                    UCk_Utils_EditorGraph_UE::Get_Pin(CVar_Register_Pins::NewValue_Pin, ECk_EditorGraph_PinDirection::Output, *this),
                    CustomEventNewValuePin
                },
            },
            ECk_EditorGraph_PinLinkType::Move) == ECk_SucceededFailed::Failed)
        {
            return;
        }
    }

    // 6. Wire main exec flow
    InCompilerContext.MovePinLinksToIntermediate(
        *FindPinChecked(UEdGraphSchema_K2::PN_Execute),
        *CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
    InCompilerContext.MovePinLinksToIntermediate(
        *FindPinChecked(UEdGraphSchema_K2::PN_Then),
        *CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Then));

    // 7. Wire input pins
    auto* MyNamePin = FindPinChecked(CVar_Register_Pins::Name_Pin);
    auto* CallNamePin = CallNode->FindPin(TEXT("InName"));
    if (CallNamePin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyNamePin, *CallNamePin);
        if (MyNamePin->LinkedTo.Num() == 0 && NOT MyNamePin->DefaultValue.IsEmpty())
        {
            CallNamePin->DefaultValue = MyNamePin->DefaultValue;
        }
    }

    // DefaultValue pin (not for commands)
    if (NOT IsCommand)
    {
        auto* MyDefaultPin = FindPin(CVar_Register_Pins::Default_Pin);
        auto* CallDefaultPin = CallNode->FindPin(TEXT("InDefaultValue"));
        if (MyDefaultPin != nullptr && CallDefaultPin != nullptr)
        {
            InCompilerContext.MovePinLinksToIntermediate(*MyDefaultPin, *CallDefaultPin);
            if (MyDefaultPin->LinkedTo.Num() == 0 && NOT MyDefaultPin->DefaultValue.IsEmpty())
            {
                CallDefaultPin->DefaultValue = MyDefaultPin->DefaultValue;
            }
        }
    }

    auto* MyHelpPin = FindPinChecked(CVar_Register_Pins::Help_Pin);
    auto* CallHelpPin = CallNode->FindPin(TEXT("InHelp"));
    if (CallHelpPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyHelpPin, *CallHelpPin);
        if (MyHelpPin->LinkedTo.Num() == 0 && NOT MyHelpPin->DefaultValue.IsEmpty())
        {
            CallHelpPin->DefaultValue = MyHelpPin->DefaultValue;
        }
    }

    // Policy pin (not for commands)
    if (NOT IsCommand)
    {
        auto* MyPolicyPin = FindPin(CVar_Register_Pins::Policy_Pin);
        auto* CallPolicyPin = CallNode->FindPin(TEXT("InPolicy"));
        if (MyPolicyPin != nullptr && CallPolicyPin != nullptr)
        {
            InCompilerContext.MovePinLinksToIntermediate(*MyPolicyPin, *CallPolicyPin);
            if (MyPolicyPin->LinkedTo.Num() == 0 && NOT MyPolicyPin->DefaultValue.IsEmpty())
            {
                CallPolicyPin->DefaultValue = MyPolicyPin->DefaultValue;
            }
        }
    }

    // 8. Wire output: callback handle
    auto* MyHandlePin = FindPinChecked(CVar_Register_Pins::Handle_Pin);
    auto* CallReturnPin = CallNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    if (CallReturnPin != nullptr)
    {
        InCompilerContext.MovePinLinksToIntermediate(*MyHandlePin, *CallReturnPin);
    }

    BreakAllNodeLinks();
}

auto
    UCk_K2Node_CVar_Register::
    GetInternalFunctionNameForType() const
    -> FName
{
    switch (_CVarType)
    {
        case ECk_CVarType::Int32:   return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Register_Int32);
        case ECk_CVarType::Float:   return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Register_Float);
        case ECk_CVarType::Bool:    return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Register_Bool);
        case ECk_CVarType::String:  return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Register_String);
        case ECk_CVarType::Command: return GET_FUNCTION_NAME_CHECKED(UCk_Utils_CVar_UE, INTERNAL_Register_Command);
        default:                    return NAME_None;
    }
}

auto
    UCk_K2Node_CVar_Register::
    RegisterCVarFromPinDefaults()
    -> void
{
    if (IsTemplate())
    {
        return;
    }

    auto* NamePin = FindPin(CVar_Register_Pins::Name_Pin);
    if (NamePin == nullptr || NamePin->DefaultValue.IsEmpty())
    {
        return;
    }

    const auto CVarName = FName{*NamePin->DefaultValue};
    if (CVarName == NAME_None)
    {
        return;
    }

    auto* DefaultPin = FindPin(CVar_Register_Pins::Default_Pin);
    const auto DefaultValue = (DefaultPin != nullptr) ? DefaultPin->DefaultValue : FString{};

    auto* HelpPin = FindPin(CVar_Register_Pins::Help_Pin);
    const auto HelpText = (HelpPin != nullptr) ? HelpPin->DefaultValue : FString{};

    auto Definition = FCk_CVarDefinition{CVarName, _CVarType, DefaultValue, HelpText};
    UCk_CVar_Settings_UE::Get()->RegisterDefinition(Definition);

    if (IConsoleManager::Get().FindConsoleObject(*CVarName.ToString()) == nullptr)
    {
        switch (_CVarType)
        {
            case ECk_CVarType::Int32:
                IConsoleManager::Get().RegisterConsoleVariable(*CVarName.ToString(), FCString::Atoi(*DefaultValue), *HelpText);
                break;
            case ECk_CVarType::Float:
                IConsoleManager::Get().RegisterConsoleVariable(*CVarName.ToString(), FCString::Atof(*DefaultValue), *HelpText);
                break;
            case ECk_CVarType::Bool:
                IConsoleManager::Get().RegisterConsoleVariable(*CVarName.ToString(), DefaultValue.ToBool() ? 1 : 0, *HelpText);
                break;
            case ECk_CVarType::String:
                IConsoleManager::Get().RegisterConsoleVariable(*CVarName.ToString(), *DefaultValue, *HelpText);
                break;
            case ECk_CVarType::Command:
                IConsoleManager::Get().RegisterConsoleCommand(*CVarName.ToString(), *HelpText, FConsoleCommandDelegate(), ECVF_Default);
                break;
        }
    }
}

auto
    UCk_K2Node_CVar_Register::
    CreateValuePinForType(
        EEdGraphPinDirection InDirection)
    -> void
{
    switch (_CVarType)
    {
        case ECk_CVarType::Int32:
            CreatePin(InDirection, UEdGraphSchema_K2::PC_Int, CVar_Register_Pins::Default_Pin);
            break;
        case ECk_CVarType::Float:
            CreatePin(InDirection, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, CVar_Register_Pins::Default_Pin);
            break;
        case ECk_CVarType::Bool:
            CreatePin(InDirection, UEdGraphSchema_K2::PC_Boolean, CVar_Register_Pins::Default_Pin);
            break;
        case ECk_CVarType::String:
            CreatePin(InDirection, UEdGraphSchema_K2::PC_String, CVar_Register_Pins::Default_Pin);
            break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
