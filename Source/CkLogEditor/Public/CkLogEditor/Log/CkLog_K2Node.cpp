#include "CkLog_K2Node.h"

#include "CkLog/CkLog_Utils.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "CkLog/CkLog_Category.h"
#include "CkLog/CkLog_Utils.h"
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
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

// ----------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "UCk_K2Node_Log"

namespace ck_k2node_log
{
    constexpr auto FormatPinName = TEXT("Format");
    constexpr auto LogCategoryPinName = TEXT("LogCategory");
    constexpr auto VerbosityPinName = TEXT("Verbosity");
    constexpr auto DefaultLogCategory = TEXT("CkCore");
}

UCk_K2Node_Log::UCk_K2Node_Log(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    _NodeTooltip = LOCTEXT("NodeTooltip",
                          "Logs a formatted message using available format argument values.\n"
                          "• Use {} to denote format arguments.\n"
                          "• Argument types may be Byte, Integer, Float, Text, String, Name, "
                          "Boolean, Object or ETextGender.\n"
                          "• Select the log category and verbosity level.");
}

auto UCk_K2Node_Log::AllocateDefaultPins() -> void
{
    Super::AllocateDefaultPins();

    // Create exec pins
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    // Create log category pin with default value
    _CachedLogCategoryPin = CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_LogCategory::StaticStruct(),
        ck_k2node_log::LogCategoryPinName);

    // Set default log category to "CkCore"
    const auto TheDefaultLogCategory = FCk_LogCategory(ck_k2node_log::DefaultLogCategory);
    FString DefaultValue;
    FCk_LogCategory::StaticStruct()->ExportText(
        DefaultValue,
        &TheDefaultLogCategory,
        &TheDefaultLogCategory,
        nullptr,
        PPF_SerializedAsImportText,
        nullptr);

    _CachedLogCategoryPin->DefaultValue = DefaultValue;

    // Create verbosity pin
    _CachedVerbosityPin = CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Byte,
        StaticEnum<ECk_LogVerbosity>(),
        ck_k2node_log::VerbosityPinName);

    _CachedVerbosityPin->DefaultValue = TEXT("Log");

    // Create format pin
    _CachedFormatPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Text, ck_k2node_log::FormatPinName);

    // Create argument pins
    for (const FName& PinName : _PinNames)
    {
        CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinName);
    }
}

auto UCk_K2Node_Log::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const FName PropertyName =
        (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Log, _PinNames))
    {
        ReconstructNode();
    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
    GetGraph()->NotifyNodeChanged(this);
}

auto UCk_K2Node_Log::GetNodeTitle(ENodeTitleType::Type TitleType) const -> FText
{
    auto VerbosityPin = GetVerbosityPin();
    auto VerbosityValue = FString{TEXT("")};

    if (ck::IsValid(VerbosityPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        if (NOT VerbosityPin->LinkedTo.IsEmpty())
        {
            // If connected, we can't determine the verbosity at compile time
            return LOCTEXT("NodeTitleConnected", "[Ck] Log Dynamic");
        }
        if (NOT VerbosityPin->DefaultValue.IsEmpty())
        {
            VerbosityValue = VerbosityPin->DefaultValue;
        }
    }

    // Add UTF-8 icons for each verbosity level
    FString Icon;
    if (VerbosityValue == TEXT("Fatal"))
    {
        Icon = TEXT("💀");
    }
    else if (VerbosityValue == TEXT("Error"))
    {
        Icon = TEXT("❌");
    }
    else if (VerbosityValue == TEXT("Warning"))
    {
        Icon = TEXT("⚠️");
    }
    else if (VerbosityValue == TEXT("Display"))
    {
        Icon = TEXT("📋");
    }
    else if (VerbosityValue == TEXT("Verbose"))
    {
        Icon = TEXT("💬");
    }
    else if (VerbosityValue == TEXT("VeryVerbose"))
    {
        Icon = TEXT("🔍");
    }
    else // Log
    {
        return FText::FromString(ck::Format_UE(TEXT("[Ck] Log 📝")));
    }

    return FText::FromString(ck::Format_UE(TEXT("[Ck] Log {} {}"), VerbosityValue, Icon));
}

auto UCk_K2Node_Log::GetNodeTitleColor() const -> FLinearColor
{
    auto VerbosityPin = GetVerbosityPin();
    FString VerbosityValue = TEXT("Log");

    if (ck::IsValid(VerbosityPin, ck::IsValid_Policy_NullptrOnly{}) &&
        VerbosityPin->LinkedTo.Num() == 0 && NOT VerbosityPin->DefaultValue.IsEmpty())
    {
        VerbosityValue = VerbosityPin->DefaultValue;
    }

    // Return colors based on verbosity
    if (VerbosityValue == TEXT("Fatal"))
    {
        return FLinearColor(0.4f, 0.0f, 0.0f); // Dark Red
    }
    if (VerbosityValue == TEXT("Error"))
    {
        return FLinearColor(0.8f, 0.0f, 0.0f); // Red
    }
    if (VerbosityValue == TEXT("Warning"))
    {
        return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
    }
    if (VerbosityValue == TEXT("Display"))
    {
        return FLinearColor(0.0f, 0.6f, 0.0f); // Green
    }
    if (VerbosityValue == TEXT("Verbose"))
    {
        return FLinearColor(0.5f, 0.5f, 0.5f); // Gray
    }
    if (VerbosityValue == TEXT("VeryVerbose"))
    {
        return FLinearColor(0.3f, 0.3f, 0.3f); // Dark Gray
    }
    // Log
    {
        return FLinearColor(0.1f, 0.3f, 0.6f); // Blue
    }
}

auto UCk_K2Node_Log::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCk_K2Node_Log::PinConnectionListChanged(UEdGraphPin* Pin) -> void
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
            if (auto CheckPin = *It; CheckPin != FormatPin && CheckPin != GetLogCategoryPin() &&
                                     CheckPin != GetVerbosityPin() && CheckPin != GetExecPin() &&
                                     CheckPin != GetThenPin() && CheckPin->Direction == EGPD_Input)
            {
                CheckPin->Modify();
                CheckPin->MarkAsGarbage();
                Pins.Remove(CheckPin);
                Removed = true;
                --It;
            }
        }

        if (Removed)
        {
            GetGraph()->NotifyNodeChanged(this);
        }

        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
    }

    // Potentially update an argument pin type
    SynchronizeArgumentPinType(Pin);
}

auto UCk_K2Node_Log::PinDefaultValueChanged(UEdGraphPin* Pin) -> void
{
    if (const auto FormatPin = GetFormatPin(); Pin == FormatPin && FormatPin->LinkedTo.Num() == 0)
    {
        TArray<FString> ArgumentParams;
        FText::GetFormatPatternParameters(FormatPin->DefaultTextValue, ArgumentParams);

        _PinNames.Reset();

        for (const auto& Param : ArgumentParams)
        {
            const FName ParamName(*Param);
            if (NOT FindArgumentPin(ParamName))
            {
                CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ParamName);
            }
            _PinNames.Add(ParamName);
        }

        for (auto It = Pins.CreateIterator(); It; ++It)
        {
            auto CheckPin = *It;
            if (CheckPin != FormatPin && CheckPin != GetLogCategoryPin() &&
                CheckPin != GetVerbosityPin() && CheckPin != GetExecPin() &&
                CheckPin != GetThenPin() && CheckPin->Direction == EGPD_Input)
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

    if (Pin == GetVerbosityPin())
    {
        // Force a visual refresh to update title and color
        ReconstructNode();
    }
}

void UCk_K2Node_Log::PinTypeChanged(UEdGraphPin* Pin)
{
    // Potentially update an argument pin type
    SynchronizeArgumentPinType(Pin);
    Super::PinTypeChanged(Pin);
}

FText UCk_K2Node_Log::GetTooltipText() const
{
    return _NodeTooltip;
}

FText UCk_K2Node_Log::GetPinDisplayName(const UEdGraphPin* Pin) const
{
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        return FText::GetEmpty();
    }
    if (Pin->PinName == ck_k2node_log::LogCategoryPinName)
    {
        return LOCTEXT("LogCategoryPinDisplayName", "Log Category");
    }
    if (Pin->PinName == ck_k2node_log::VerbosityPinName)
    {
        return LOCTEXT("VerbosityPinDisplayName", "Verbosity");
    }
    if (Pin->PinName == ck_k2node_log::FormatPinName)
    {
        return LOCTEXT("FormatPinDisplayName", "Format");
    }

    return FText::FromName(Pin->PinName);
}

FSlateIcon UCk_K2Node_Log::GetIconAndTint(FLinearColor& OutColor) const
{
    OutColor = FLinearColor::White;
    static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
    return Icon;
}

auto UCk_K2Node_Log::IsNodePure() const -> bool
{
    return false;
}
bool UCk_K2Node_Log::NodeCausesStructuralBlueprintChange() const
{
    return true;
}

void UCk_K2Node_Log::PostReconstructNode()
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
            for (const auto CurrentPin : Pins)
            {
                if (CurrentPin != FormatPin && CurrentPin != GetLogCategoryPin() &&
                    CurrentPin != GetVerbosityPin() && CurrentPin != GetExecPin() &&
                    CurrentPin != GetThenPin() && CurrentPin->Direction == EGPD_Input &&
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

                    const auto LiteralReturnValuePin = MakeLiteralText->FindPinChecked(
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

void UCk_K2Node_Log::ExpandNode(FKismetCompilerContext& CompilerContext,
                                        UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    // Create a "Make Array" node for format arguments
    const auto MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this,
                                                                                  SourceGraph);
    MakeArrayNode->AllocateDefaultPins();
    CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeArrayNode, this);

    const auto ArrayOut = MakeArrayNode->GetOutputPin();

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

    // Create log function call based on verbosity
    const auto CreateLogFunctionCall =
        CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

    // Determine which log function to call based on verbosity
    const auto VerbosityPin = GetVerbosityPin();
    FString VerbosityValue = VerbosityPin->DefaultValue;
    if (VerbosityPin->LinkedTo.Num() > 0)
    {
        // If verbosity is connected, we need to use a switch statement or similar
        // For now, we'll default to Log verbosity
        VerbosityValue = TEXT("Log");
    }

    FName LogFunctionName;
    if (VerbosityValue == TEXT("Fatal"))
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log_Fatal);
    }
    else if (VerbosityValue == TEXT("Error"))
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log_Error);
    }
    else if (VerbosityValue == TEXT("Warning"))
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log_Warning);
    }
    else if (VerbosityValue == TEXT("Display"))
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log_Display);
    }
    else if (VerbosityValue == TEXT("Verbose"))
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log_Verbose);
    }
    else if (VerbosityValue == TEXT("VeryVerbose"))
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log_VeryVerbose);
    }
    else
    {
        LogFunctionName = GET_MEMBER_NAME_CHECKED(UCk_Utils_Log_UE, Log);
    }

    CreateLogFunctionCall->SetFromFunction(
        UCk_Utils_Log_UE::StaticClass()->FindFunctionByName(LogFunctionName));
    CreateLogFunctionCall->AllocateDefaultPins();
    CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CreateLogFunctionCall, this);

    // Process format arguments
    for (int32 ArgIdx = 0; ArgIdx < _PinNames.Num(); ++ArgIdx)
    {
        auto ArgumentPin = FindArgumentPin(_PinNames[ArgIdx]);

        static auto FormatArgumentDataStruct =
            FindObjectChecked<UScriptStruct>(FindObjectChecked<UPackage>(
                nullptr,
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

                const auto CallIntToInt64Function =
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

                const auto CallByteToIntFunction =
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
                static auto TextGenderEnum = FindObjectChecked<UEnum>(
                    nullptr, TEXT("/Script/Engine.ETextGender"), /*ExactClass*/ true);
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
        {
            MakeArrayNode->AddInputPin();
        }

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
    CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CreateLogFunctionCall->GetExecPin());
    CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *CreateLogFunctionCall->GetThenPin());
    CompilerContext.MovePinLinksToIntermediate(*GetFormatPin(),
                                               *CallFormatFunction->FindPinChecked(
                                                   TEXT("InPattern")));
    CompilerContext.MovePinLinksToIntermediate(*GetLogCategoryPin(),
                                               *CreateLogFunctionCall->FindPinChecked(
                                                   TEXT("InLogCategory")));

    // Connect formatted text to log function
    CallFormatFunction->GetReturnValuePin()->MakeLinkTo(
        CreateLogFunctionCall->FindPinChecked(TEXT("InMsg")));

    BreakAllNodeLinks();
}

UK2Node::ERedirectType UCk_K2Node_Log::DoPinsMatchForReconstruction(
    const UEdGraphPin* NewPin,
    int32 NewPinIndex,
    const UEdGraphPin* OldPin,
    int32 OldPinIndex) const
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
            {
                RedirectType = ERedirectType_Name;
            }
            else
            {
                RedirectType = ERedirectType_None;
            }
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
            {
                RedirectType = ERedirectType_None;
            }
        }
    }

    return RedirectType;
}

bool UCk_K2Node_Log::IsConnectionDisallowed(const UEdGraphPin* MyPin,
                                                    const UEdGraphPin* OtherPin,
                                                    FString& OutReason) const
{
    // Argument input pins may only be connected to supported types
    const auto FormatPin = GetFormatPin();
    const auto LogCategoryPin = GetLogCategoryPin();
    const auto VerbosityPin = GetVerbosityPin();

    if (MyPin != FormatPin && MyPin != LogCategoryPin && MyPin != VerbosityPin &&
        MyPin != GetExecPin() && MyPin != GetThenPin() && MyPin->Direction == EGPD_Input)
    {
        const FName& OtherPinCategory = OtherPin->PinType.PinCategory;

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
            static auto TextGenderEnum = FindObjectChecked<UEnum>(
                nullptr, TEXT("/Script/Engine.ETextGender"), /*ExactClass*/ true);
            if (OtherPin->PinType.PinSubCategoryObject == TextGenderEnum)
            {
                IsValidType = true;
            }
        }

        if (NOT IsValidType)
        {
            OutReason = LOCTEXT("Error_InvalidArgumentType",
                                "Format arguments may only be Byte, Integer, Int64, Float, Double, "
                                "Text, String, Name, Boolean, Object, Wildcard or ETextGender.").ToString();
            return true;
        }
    }

    return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UCk_K2Node_Log::GetMenuActions(
    FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    if (const auto ActionKey = GetClass();
        ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        const auto NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UCk_K2Node_Log::GetMenuCategory() const
{
    return LOCTEXT("MenuCategory", "Ck|Utils|Log");
}
int32 UCk_K2Node_Log::GetNodeRefreshPriority() const
{
    return Low_UsesDependentWildcard;
}

FNodeHandlingFunctor* UCk_K2Node_Log::CreateNodeHandler(
    FKismetCompilerContext& CompilerContext) const
{
    // No special handling needed
    return nullptr;
}

UEdGraphPin* UCk_K2Node_Log::GetLogCategoryPin() const
{
    if (ck::Is_NOT_Valid(_CachedLogCategoryPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        const_cast<UCk_K2Node_Log*>(this)->_CachedLogCategoryPin = FindPinChecked(
            ck_k2node_log::LogCategoryPinName);
    }
    return _CachedLogCategoryPin;
}

UEdGraphPin* UCk_K2Node_Log::GetVerbosityPin() const
{
    if (ck::Is_NOT_Valid(_CachedVerbosityPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        const_cast<UCk_K2Node_Log*>(this)->_CachedVerbosityPin = FindPin(ck_k2node_log::VerbosityPinName);
    }
    return _CachedVerbosityPin;
}

UEdGraphPin* UCk_K2Node_Log::GetFormatPin() const
{
    if (ck::Is_NOT_Valid(_CachedFormatPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        const_cast<UCk_K2Node_Log*>(this)->_CachedFormatPin = FindPinChecked(ck_k2node_log::FormatPinName);
    }
    return _CachedFormatPin;
}

auto UCk_K2Node_Log::GetArgumentCount() const -> int32
{
    return _PinNames.Num();
}

FText UCk_K2Node_Log::GetArgumentName(int32 InIndex) const
{
    if (InIndex < _PinNames.Num())
    {
        return FText::FromName(_PinNames[InIndex]);
    }
    return FText::GetEmpty();
}
bool UCk_K2Node_Log::CanEditArguments() const
{
    return GetFormatPin()->LinkedTo.Num() > 0;
}

void UCk_K2Node_Log::SynchronizeArgumentPinType(UEdGraphPin* Pin) const
{
    const auto FormatPin = GetFormatPin();
    const auto LogCategoryPin = GetLogCategoryPin();
    const auto VerbosityPin = GetVerbosityPin();

    if (Pin != FormatPin && Pin != LogCategoryPin && Pin != VerbosityPin && Pin != GetExecPin() &&
        Pin != GetThenPin() && Pin->Direction == EGPD_Input)
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

            auto Blueprint = GetBlueprint();
            if (NOT Blueprint->bBeingCompiled)
            {
                FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            }
        }
    }
}

FName UCk_K2Node_Log::GetUniquePinName() const
{
    FName NewPinName;
    auto I = 0;
    while (true)
    {
        NewPinName = *FString::FromInt(I++);
        if (ck::Is_NOT_Valid(FindPin(NewPinName), ck::IsValid_Policy_NullptrOnly{}))
        {
            break;
        }
    }
    return NewPinName;
}

UEdGraphPin* UCk_K2Node_Log::FindArgumentPin(const FName InPinName) const
{
    const auto FormatPin = GetFormatPin();
    const auto LogCategoryPin = GetLogCategoryPin();
    const auto VerbosityPin = GetVerbosityPin();

    for (const auto Pin : Pins)
    {
        if (Pin != FormatPin && Pin != LogCategoryPin && Pin != VerbosityPin &&
            Pin != GetExecPin() && Pin != GetThenPin() && Pin->Direction == EGPD_Input &&
            Pin->PinName.ToString().Equals(InPinName.ToString(), ESearchCase::CaseSensitive))
        {
            return Pin;
        }
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE

// ----------------------------------------------------------------------------------------------------------------
