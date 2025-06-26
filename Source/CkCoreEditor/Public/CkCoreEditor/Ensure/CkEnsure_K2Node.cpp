#include "CkEnsure_K2Node.h"

#include "CkCore/Ensure/CkEnsure_Utils.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"

// ----------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "UCk_K2Node_Ensure"

namespace ck_k2node_ensure
{
    constexpr auto ExpressionPinName = TEXT("Expression");
    constexpr auto FormatPinName = TEXT("Format");
    constexpr auto PassedPinName = TEXT("Passed");
    constexpr auto FailedPinName = TEXT("Failed");
}

UCk_K2Node_Ensure::UCk_K2Node_Ensure(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    _NodeTooltip = LOCTEXT(
        "NodeTooltip",
        "Ensures a condition is true using a formatted message.\n  • If the expression evaluates "
        "to false, triggers an ensure with custom formatted text.\n  • Use {} to denote format "
        "arguments.\n  • Execution continues through 'Passed' if true, 'Failed' if false.\n  • The "
        "ensure dialog will only appear once per unique location unless cleared.");

    SetEnabledState(ENodeEnabledState::DevelopmentOnly, false);
}

auto UCk_K2Node_Ensure::AllocateDefaultPins() -> void
{
    Super::AllocateDefaultPins();

    // Create exec pins
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2node_ensure::PassedPinName);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ck_k2node_ensure::FailedPinName);

    // Create expression pin (boolean)
    _CachedExpressionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, ck_k2node_ensure::ExpressionPinName);
    _CachedExpressionPin->DefaultValue = TEXT("false");

    // Create format pin
    _CachedFormatPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Text, ck_k2node_ensure::FormatPinName);

    // Create argument pins
    for (const FName& PinName : _PinNames)
    {
        CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinName);
    }
}

auto UCk_K2Node_Ensure::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    const FName PropertyName =
        (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Ensure, _PinNames))
        ReconstructNode();

    Super::PostEditChangeProperty(PropertyChangedEvent);
    GetGraph()->NotifyNodeChanged(this);
}

auto UCk_K2Node_Ensure::GetNodeTitle(ENodeTitleType::Type TitleType) const -> FText
{
    return LOCTEXT("NodeTitle", "[Ck] Ensure 🛡️");
}

auto UCk_K2Node_Ensure::GetNodeTitleColor() const -> FLinearColor
{
    // Purple color to distinguish from logs and indicate validation/assertion
    return FLinearColor(0.5f, 0.2f, 0.8f);
}

auto UCk_K2Node_Ensure::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCk_K2Node_Ensure::PinConnectionListChanged(UEdGraphPin* Pin) -> void
{
    const auto FormatPin = GetFormatPin();

    Modify();

    // Clear all argument pins when format pin is connected/disconnected
    if (Pin == FormatPin && NOT FormatPin->DefaultTextValue.IsEmpty())
    {
        _PinNames.Empty();
        GetSchema()->TrySetDefaultText(*FormatPin, FText::GetEmpty());

        auto Removed = false;
        for (auto It = Pins.CreateConstIterator(); It; ++It)
        {
            auto CheckPin = *It;
            if (CheckPin != FormatPin && CheckPin != GetExpressionPin() &&
                CheckPin != GetExecPin() && CheckPin != GetPassedPin() &&
                CheckPin != GetFailedPin() && CheckPin->Direction == EGPD_Input)
            {
                CheckPin->Modify();
                CheckPin->MarkAsGarbage();
                Pins.Remove(CheckPin);
                Removed = true;
                --It;
            }
        }

        if (Removed)
            GetGraph()->NotifyNodeChanged(this);

        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
    }

    SynchronizeArgumentPinType(Pin);
}

auto UCk_K2Node_Ensure::PinDefaultValueChanged(UEdGraphPin* Pin) -> void
{
    if (const auto FormatPin = GetFormatPin(); Pin == FormatPin && FormatPin->LinkedTo.Num() == 0)
    {
        TArray<FString> ArgumentParams;
        FText::GetFormatPatternParameters(FormatPin->DefaultTextValue, ArgumentParams);

        _PinNames.Reset();

        for (const FString& Param : ArgumentParams)
        {
            const FName ParamName(*Param);
            if (NOT FindArgumentPin(ParamName))
                CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ParamName);

            _PinNames.Add(ParamName);
        }

        for (auto It = Pins.CreateIterator(); It; ++It)
        {
            auto CheckPin = *It;
            if (CheckPin != FormatPin && CheckPin != GetExpressionPin() &&
                CheckPin != GetExecPin() && CheckPin != GetPassedPin() &&
                CheckPin != GetFailedPin() && CheckPin->Direction == EGPD_Input)
            {
                const auto IsValidArgPin = ArgumentParams.ContainsByPredicate(
                    [&CheckPin](const FString& InPinName)
                    {
                        return InPinName.Equals(CheckPin->PinName.ToString(),
                                                ESearchCase::CaseSensitive);
                    });

                if (NOT IsValidArgPin)
                {
                    CheckPin->MarkAsGarbage();
                    It.RemoveCurrent();
                }
            }
        }

        GetGraph()->NotifyNodeChanged(this);
    }
}

auto UCk_K2Node_Ensure::PinTypeChanged(UEdGraphPin* Pin) -> void
{
    SynchronizeArgumentPinType(Pin);
    Super::PinTypeChanged(Pin);
}

auto UCk_K2Node_Ensure::GetTooltipText() const -> FText
{
    return _NodeTooltip;
}

auto UCk_K2Node_Ensure::GetPinDisplayName(const UEdGraphPin* Pin) const -> FText
{
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        if (Pin->PinName == ck_k2node_ensure::PassedPinName)
        {
            return LOCTEXT("PassedPinDisplayName", "Passed");
        }
        if (Pin->PinName == ck_k2node_ensure::FailedPinName)
        {
            return LOCTEXT("FailedPinDisplayName", "Failed");
        }

        return FText::GetEmpty();
    }
    if (Pin->PinName == ck_k2node_ensure::ExpressionPinName)
    {
        return LOCTEXT("ExpressionPinDisplayName", "Expression");
    }
    if (Pin->PinName == ck_k2node_ensure::FormatPinName)
    {
        return LOCTEXT("FormatPinDisplayName", "Format");
    }

    return FText::FromName(Pin->PinName);
}

auto UCk_K2Node_Ensure::GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = FLinearColor::White;
    static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
    return Icon;
}

auto UCk_K2Node_Ensure::IsNodePure() const -> bool
{
    return false;
}

auto UCk_K2Node_Ensure::NodeCausesStructuralBlueprintChange() const -> bool
{
    return true;
}

auto UCk_K2Node_Ensure::PostReconstructNode() -> void
{
    Super::PostReconstructNode();

    // Handle any necessary pin upgrades similar to FormatText
    if (NOT IsTemplate())
    {
        if (auto OuterGraph = GetGraph();
            ck::IsValid(OuterGraph) && ck::IsValid(OuterGraph->Schema))
        {
            auto NumPinsFixedUp = 0;

            const auto FormatPin = GetFormatPin();
            for (const auto& CurrentPin : Pins)
            {
                if (CurrentPin != FormatPin && CurrentPin != GetExpressionPin() &&
                    CurrentPin != GetExecPin() && CurrentPin != GetPassedPin() &&
                    CurrentPin != GetFailedPin() && CurrentPin->Direction == EGPD_Input &&
                    CurrentPin->LinkedTo.Num() == 0 && NOT CurrentPin->DefaultTextValue.IsEmpty())
                {
                    // Create a new "Make Literal Text" function
                    const FVector2D SpawnLocation =
                        FVector2D(NodePosX - 300, NodePosY + (60 * (NumPinsFixedUp + 1)));
                    auto MakeLiteralText =
                        FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CallFunction>(
                            GetGraph(),
                            SpawnLocation,
                            EK2NewNodeFlags::None,
                            [](UK2Node_CallFunction* NewInstance)
                            {
                                NewInstance->SetFromFunction(
                                    UKismetSystemLibrary::StaticClass()->FindFunctionByName(
                                        GET_MEMBER_NAME_CHECKED(UKismetSystemLibrary,
                                                                MakeLiteralText)));
                            });

                    const auto LiteralValuePin = MakeLiteralText->FindPinChecked(TEXT("Value"));
                    LiteralValuePin->DefaultTextValue = CurrentPin->DefaultTextValue;
                    CurrentPin->DefaultTextValue = FText::GetEmpty();

                    auto LiteralReturnValuePin = MakeLiteralText->FindPinChecked(
                        UEdGraphSchema_K2::PN_ReturnValue);
                    GetSchema()->TryCreateConnection(LiteralReturnValuePin, CurrentPin);

                    ++NumPinsFixedUp;
                }

                // Potentially update an argument pin type
                SynchronizeArgumentPinType(CurrentPin);
            }

            if (NumPinsFixedUp > 0)
            {
                GetGraph()->NotifyNodeChanged(this);
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
            }
        }
    }
}

auto UCk_K2Node_Ensure::ExpandNode(FKismetCompilerContext& CompilerContext,
                                           UEdGraph* SourceGraph) -> void
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    // Create a "Make Array" node for format arguments
    auto MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this,
                                                                                  SourceGraph);
    MakeArrayNode->AllocateDefaultPins();
    CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeArrayNode, this);

    auto ArrayOut = MakeArrayNode->GetOutputPin();

    // Create Format function call
    const auto CallFormatFunction =
        CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CallFormatFunction->SetFromFunction(UKismetTextLibrary::StaticClass()->FindFunctionByName(
        GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Format)));
    CallFormatFunction->AllocateDefaultPins();
    CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFormatFunction, this);

    // Connect array to format function
    ArrayOut->MakeLinkTo(CallFormatFunction->FindPinChecked(TEXT("InArgs")));
    MakeArrayNode->PinConnectionListChanged(ArrayOut);

    // Create ensure function call
    const auto CallEnsureFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CallEnsureFunction->SetFromFunction(UCk_Utils_Ensure_UE::StaticClass()->FindFunctionByName(
        GET_MEMBER_NAME_CHECKED(UCk_Utils_Ensure_UE, EnsureMsgf)));
    CallEnsureFunction->AllocateDefaultPins();
    CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallEnsureFunction, this);

    // Process format arguments
    for (int32 ArgIdx = 0; ArgIdx < _PinNames.Num(); ++ArgIdx)
    {
        auto ArgumentPin = FindArgumentPin(_PinNames[ArgIdx]);

        static auto FormatArgumentDataStruct =
            FindObjectChecked<UScriptStruct>(FindObjectChecked<UPackage>(nullptr,
                                                                         TEXT("/Script/Engine")),
                                             TEXT("FormatArgumentData"));

        // Create Make Struct node
        auto MakeFormatArgumentDataStruct =
            CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
        MakeFormatArgumentDataStruct->StructType = FormatArgumentDataStruct;
        MakeFormatArgumentDataStruct->AllocateDefaultPins();
        MakeFormatArgumentDataStruct->bMadeAfterOverridePinRemoval = true;
        CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeFormatArgumentDataStruct,
                                                                    this);

        // Set argument name
        MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(
            *MakeFormatArgumentDataStruct->FindPinChecked(
                GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentName)),
            ArgumentPin->PinName.ToString());

        auto ArgumentTypePin = MakeFormatArgumentDataStruct->FindPinChecked(
            GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueType));

        // Handle argument type conversions (similar to FormatText)
        if (ArgumentPin->LinkedTo.Num() > 0)
        {
            const FName& ArgumentPinCategory = ArgumentPin->PinType.PinCategory;

            // Lambda for adding conversion nodes
            auto AddConversionNode = [&](const FName FuncName, const TCHAR* PinName)
            {
                MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                              TEXT("Text"));

                auto ToTextFunction =
                    CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
                ToTextFunction->SetFromFunction(
                    UKismetTextLibrary::StaticClass()->FindFunctionByName(FuncName));
                ToTextFunction->AllocateDefaultPins();
                CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ToTextFunction, this);

                CompilerContext.MovePinLinksToIntermediate(*ArgumentPin,
                                                           *ToTextFunction->FindPinChecked(
                                                               PinName));
                ToTextFunction->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)
                    ->MakeLinkTo(MakeFormatArgumentDataStruct->FindPinChecked(
                        GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValue)));
            };

            if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Int)
            {
                MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                              TEXT("Int"));

                auto CallIntToInt64Function =
                    CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
                CallIntToInt64Function->SetFromFunction(
                    UKismetMathLibrary::StaticClass()->FindFunctionByName(
                        GET_MEMBER_NAME_CHECKED(UKismetMathLibrary, Conv_IntToInt64)));
                CallIntToInt64Function->AllocateDefaultPins();
                CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallIntToInt64Function,
                                                                            this);

                CompilerContext.MovePinLinksToIntermediate(*ArgumentPin,
                                                           *CallIntToInt64Function->FindPinChecked(
                                                               TEXT("InInt")));
                CallIntToInt64Function->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)
                    ->MakeLinkTo(MakeFormatArgumentDataStruct->FindPinChecked(
                        GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueInt)));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Real)
            {
                if (ArgumentPin->PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
                {
                    MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                                  TEXT("Float"));
                    CompilerContext.MovePinLinksToIntermediate(
                        *ArgumentPin,
                        *MakeFormatArgumentDataStruct->FindPinChecked(
                            GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData,
                                                           ArgumentValueFloat)));
                }
                else if (ArgumentPin->PinType.PinSubCategory == UEdGraphSchema_K2::PC_Double)
                {
                    MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                                  TEXT("Double"));
                    CompilerContext.MovePinLinksToIntermediate(
                        *ArgumentPin,
                        *MakeFormatArgumentDataStruct->FindPinChecked(
                            GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData,
                                                           ArgumentValueDouble)));
                }
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Int64)
            {
                MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                              TEXT("Int64"));
                CompilerContext.MovePinLinksToIntermediate(
                    *ArgumentPin,
                    *MakeFormatArgumentDataStruct->FindPinChecked(
                        GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueInt)));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Text)
            {
                MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                              TEXT("Text"));
                CompilerContext.MovePinLinksToIntermediate(
                    *ArgumentPin,
                    *MakeFormatArgumentDataStruct->FindPinChecked(
                        GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValue)));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Byte &&
                     NOT ArgumentPin->PinType.PinSubCategoryObject.IsValid())
            {
                MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                              TEXT("Int"));

                auto CallByteToIntFunction =
                    CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
                CallByteToIntFunction->SetFromFunction(
                    UKismetMathLibrary::StaticClass()->FindFunctionByName(
                        GET_MEMBER_NAME_CHECKED(UKismetMathLibrary, Conv_ByteToInt64)));
                CallByteToIntFunction->AllocateDefaultPins();
                CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallByteToIntFunction,
                                                                            this);

                CompilerContext.MovePinLinksToIntermediate(*ArgumentPin,
                                                           *CallByteToIntFunction->FindPinChecked(
                                                               TEXT("InByte")));
                CallByteToIntFunction->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)
                    ->MakeLinkTo(MakeFormatArgumentDataStruct->FindPinChecked(
                        GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueInt)));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Byte ||
                     ArgumentPinCategory == UEdGraphSchema_K2::PC_Enum)
            {
                static auto TextGenderEnum =
                    FindObjectChecked<UEnum>(nullptr,
                                             TEXT("/Script/Engine.ETextGender"),
                                             /*ExactClass*/
                                             true);
                if (ArgumentPin->PinType.PinSubCategoryObject == TextGenderEnum)
                {
                    MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                                  TEXT("Gender"));
                    CompilerContext.MovePinLinksToIntermediate(
                        *ArgumentPin,
                        *MakeFormatArgumentDataStruct->FindPinChecked(
                            GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData,
                                                           ArgumentValueGender)));
                }
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Boolean)
            {
                AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_BoolToText),
                                  TEXT("InBool"));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Name)
            {
                AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_NameToText),
                                  TEXT("InName"));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_String)
            {
                AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText),
                                  TEXT("InString"));
            }
            else if (ArgumentPinCategory == UEdGraphSchema_K2::PC_Object)
            {
                AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_ObjectToText),
                                  TEXT("InObj"));
            }
            else
            {
                // Unexpected pin type
                CompilerContext.MessageLog.Error(
                    *FText::Format(LOCTEXT("Error_UnexpectedPinType",
                                           "Pin '{0}' has an unexpected type: {1}"),
                                   FText::FromName(_PinNames[ArgIdx]),
                                   FText::FromName(ArgumentPinCategory))
                         .ToString());
            }
        }
        else
        {
            // No connected pin - default to empty text
            MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultValue(*ArgumentTypePin,
                                                                          TEXT("Text"));
            MakeFormatArgumentDataStruct->GetSchema()->TrySetDefaultText(
                *MakeFormatArgumentDataStruct->FindPinChecked(
                    GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValue)),
                FText::GetEmpty());
        }

        // Add pins to array
        if (ArgIdx > 0)
            MakeArrayNode->AddInputPin();

        const FString PinName = FString::Printf(TEXT("[%d]"), ArgIdx);
        const auto InputPin = MakeArrayNode->FindPinChecked(PinName);

        // Connect struct output to array
        auto FindOutputStructPinChecked = [](UEdGraphNode* Node) -> UEdGraphPin*
        {
            check(Node != nullptr);
            UEdGraphPin* OutputPin = nullptr;
            for (auto Pin : Node->Pins)
            {
                if (ck::IsValid(Pin, ck::IsValid_Policy_NullptrOnly{}) &&
                    Pin->Direction == EGPD_Output)
                {
                    OutputPin = Pin;
                    break;
                }
            }
            check(OutputPin != nullptr);
            return OutputPin;
        };

        FindOutputStructPinChecked(MakeFormatArgumentDataStruct)->MakeLinkTo(InputPin);
    }

    // Connect everything
    CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallEnsureFunction->GetExecPin());
    CompilerContext.MovePinLinksToIntermediate(*GetExpressionPin(),
                                               *CallEnsureFunction->FindPinChecked(
                                                   TEXT("InExpression")));
    CompilerContext.MovePinLinksToIntermediate(*GetFormatPin(),
                                               *CallFormatFunction->FindPinChecked(
                                                   TEXT("InPattern")));

    // Connect formatted text to ensure function
    CallFormatFunction->GetReturnValuePin()->MakeLinkTo(
        CallEnsureFunction->FindPinChecked(TEXT("InMsg")));

    // Handle the two output exec pins based on the OutHitStatus
    auto ValidExecPin = CallEnsureFunction->FindPinChecked(TEXT("Valid"));
    auto InvalidExecPin = CallEnsureFunction->FindPinChecked(TEXT("Invalid"));

    CompilerContext.MovePinLinksToIntermediate(*GetPassedPin(), *ValidExecPin);
    CompilerContext.MovePinLinksToIntermediate(*GetFailedPin(), *InvalidExecPin);

    BreakAllNodeLinks();
}

auto UCk_K2Node_Ensure::DoPinsMatchForReconstruction(
    const UEdGraphPin* NewPin,
    int32 NewPinIndex,
    const UEdGraphPin* OldPin,
    int32 OldPinIndex) const
    -> UK2Node::ERedirectType
{
    auto RedirectType = ERedirectType_None;

    if (NewPin->PinName.ToString().Equals(OldPin->PinName.ToString(), ESearchCase::CaseSensitive))
    {
        if (auto OuterGraph = GetGraph();
            ck::IsValid(OuterGraph) && ck::IsValid(OuterGraph->Schema))
        {
            const auto K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
            if (ck::Is_NOT_Valid(K2Schema) || K2Schema->IsSelfPin(*NewPin) ||
                K2Schema->ArePinTypesCompatible(OldPin->PinType, NewPin->PinType))
                RedirectType = ERedirectType_Name;
            else
                RedirectType = ERedirectType_None;
        }
    }
    else
    {
        // Check for redirects
        if (const auto Node = Cast<UK2Node>(NewPin->GetOwningNode()))
        {
            TArray<FString> OldPinNames;
            GetRedirectPinNames(*OldPin, OldPinNames);

            FName NewPinName;
            RedirectType = ShouldRedirectParam(OldPinNames, /*out*/ NewPinName, Node);

            if ((RedirectType != ERedirectType_None) &&
                (NOT NewPin->PinName.ToString().Equals(NewPinName.ToString(),
                                                       ESearchCase::CaseSensitive)))
                RedirectType = ERedirectType_None;
        }
    }

    return RedirectType;
}

auto UCk_K2Node_Ensure::IsConnectionDisallowed(
    const UEdGraphPin* MyPin,
    const UEdGraphPin* OtherPin,
    FString& OutReason) const -> bool
{
    // Argument input pins may only be connected to supported types
    const auto FormatPin = GetFormatPin();
    const auto ExpressionPin = GetExpressionPin();

    if (MyPin != FormatPin && MyPin != ExpressionPin && MyPin != GetExecPin() &&
        MyPin != GetPassedPin() && MyPin != GetFailedPin() && MyPin->Direction == EGPD_Input)
    {
        const auto& OtherPinCategory = OtherPin->PinType.PinCategory;

        auto IsValidType = false;
        if (OtherPinCategory == UEdGraphSchema_K2::PC_Int ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Real ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Text ||
            (OtherPinCategory == UEdGraphSchema_K2::PC_Byte &&
             NOT OtherPin->PinType.PinSubCategoryObject.IsValid()) ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Boolean ||
            OtherPinCategory == UEdGraphSchema_K2::PC_String ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Name ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Object ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Wildcard ||
            OtherPinCategory == UEdGraphSchema_K2::PC_Int64)
        {
            IsValidType = true;
        }
        else if (OtherPinCategory == UEdGraphSchema_K2::PC_Byte ||
                 OtherPinCategory == UEdGraphSchema_K2::PC_Enum)
        {
            static auto TextGenderEnum =
                FindObjectChecked<UEnum>(nullptr,
                                         TEXT("/Script/Engine.ETextGender"),
                                         /*ExactClass*/
                                         true);
            if (OtherPin->PinType.PinSubCategoryObject == TextGenderEnum)
                IsValidType = true;
        }

        if (NOT IsValidType)
        {
            OutReason = LOCTEXT("Error_InvalidArgumentType",
                                "Format arguments may only be Byte, Integer, Int64, Float, Double, "
                                "Text, String, Name, Boolean, Object, Wildcard or ETextGender.")
                            .ToString();
            return true;
        }
    }

    return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

auto UCk_K2Node_Ensure::GetMenuActions(
    FBlueprintActionDatabaseRegistrar& ActionRegistrar) const -> void
{
    if (const auto ActionKey = GetClass();
        ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        const auto NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

auto UCk_K2Node_Ensure::GetMenuCategory() const -> FText
{
    return LOCTEXT("MenuCategory", "Ck|Utils|Ensure");
}

auto UCk_K2Node_Ensure::GetNodeRefreshPriority() const -> int32
{
    return Low_UsesDependentWildcard;
}

auto UCk_K2Node_Ensure::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
    -> FNodeHandlingFunctor*
{
    // No special handling needed
    return nullptr;
}

auto UCk_K2Node_Ensure::GetExpressionPin() const -> UEdGraphPin*
{
    if (ck::Is_NOT_Valid(_CachedExpressionPin, ck::IsValid_Policy_NullptrOnly{}))
        const_cast<UCk_K2Node_Ensure*>(this)->_CachedExpressionPin = FindPinChecked(
            ck_k2node_ensure::ExpressionPinName);

    return _CachedExpressionPin;
}

auto UCk_K2Node_Ensure::GetFormatPin() const -> UEdGraphPin*
{
    if (ck::Is_NOT_Valid(_CachedFormatPin, ck::IsValid_Policy_NullptrOnly{}))
        const_cast<UCk_K2Node_Ensure*>(this)->_CachedFormatPin = FindPinChecked(
            ck_k2node_ensure::FormatPinName);

    return _CachedFormatPin;
}

auto UCk_K2Node_Ensure::GetPassedPin() const -> UEdGraphPin*
{
    return FindPinChecked(ck_k2node_ensure::PassedPinName);
}

auto UCk_K2Node_Ensure::GetFailedPin() const -> UEdGraphPin*
{
    return FindPinChecked(ck_k2node_ensure::FailedPinName);
}

auto UCk_K2Node_Ensure::GetArgumentCount() const -> int32
{
    return _PinNames.Num();
}

auto UCk_K2Node_Ensure::GetArgumentName(int32 InIndex) const -> FText
{
    if (InIndex < _PinNames.Num())
        return FText::FromName(_PinNames[InIndex]);

    return FText::GetEmpty();
}

auto UCk_K2Node_Ensure::CanEditArguments() const -> bool
{
    return GetFormatPin()->LinkedTo.Num() > 0;
}

auto UCk_K2Node_Ensure::SynchronizeArgumentPinType(UEdGraphPin* Pin) const -> void
{
    const auto FormatPin = GetFormatPin();

    if (const auto ExpressionPin = GetExpressionPin();
        Pin != FormatPin && Pin != ExpressionPin && Pin != GetExecPin() && Pin != GetPassedPin() &&
        Pin != GetFailedPin() && Pin->Direction == EGPD_Input)
    {
        auto PinTypeChanged = false;
        if (Pin->LinkedTo.Num() == 0)
        {
            static const FEdGraphPinType WildcardPinType =
                FEdGraphPinType(UEdGraphSchema_K2::PC_Wildcard,
                                NAME_None,
                                nullptr,
                                EPinContainerType::None,
                                false,
                                FEdGraphTerminalType());

            // Ensure wildcard
            if (Pin->PinType != WildcardPinType)
            {
                Pin->PinType = WildcardPinType;
                PinTypeChanged = true;
            }
        }
        else
        {
            // Take the type of the connected pin
            if (const auto ArgumentSourcePin = Pin->LinkedTo[0];
                Pin->PinType != ArgumentSourcePin->PinType)
            {
                Pin->PinType = ArgumentSourcePin->PinType;
                PinTypeChanged = true;
            }
        }

        if (PinTypeChanged)
        {
            // Let the graph know to refresh
            GetGraph()->NotifyNodeChanged(this);

            if (const auto Blueprint = GetBlueprint();
                ck::IsValid(Blueprint) && NOT Blueprint->bBeingCompiled)
                FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        }
    }
}

auto UCk_K2Node_Ensure::GetUniquePinName() const -> FName
{
    FName NewPinName;
    auto I = 0;
    while (true)
    {
        NewPinName = *FString::FromInt(I++);
        if (ck::Is_NOT_Valid(FindPin(NewPinName), ck::IsValid_Policy_NullptrOnly{}))
            break;
    }
    return NewPinName;
}

auto UCk_K2Node_Ensure::FindArgumentPin(const FName InPinName) const -> UEdGraphPin*
{
    const auto FormatPin = GetFormatPin();
    const auto ExpressionPin = GetExpressionPin();

    for (const auto Pin : Pins)
    {
        if (Pin != FormatPin && Pin != ExpressionPin && Pin != GetExecPin() &&
            Pin != GetPassedPin() && Pin != GetFailedPin() && Pin->Direction == EGPD_Input &&
            Pin->PinName.ToString().Equals(InPinName.ToString(), ESearchCase::CaseSensitive))
            return Pin;
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE

// ----------------------------------------------------------------------------------------------------------------
