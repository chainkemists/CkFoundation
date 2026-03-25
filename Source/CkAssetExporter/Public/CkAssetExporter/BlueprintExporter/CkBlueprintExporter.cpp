#include "CkBlueprintExporter.h"

#include "CkAssetExporter_Log.h"

#include <Engine/Blueprint.h>
#include <EdGraph/EdGraph.h>
#include <EdGraph/EdGraphNode.h>
#include <EdGraph/EdGraphPin.h>
#include <EdGraphSchema_K2.h>
#include <K2Node.h>
#include <K2Node_Event.h>
#include <K2Node_CallFunction.h>
#include <K2Node_FunctionEntry.h>
#include <K2Node_FunctionResult.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    ExportBlueprint(
        UBlueprint* InBlueprint)
    -> FCk_BlueprintExportResult
{
    auto Result = FCk_BlueprintExportResult{};

    if (!IsValid(InBlueprint))
    {
        Result.ErrorMessage = TEXT("Invalid Blueprint asset");
        return Result;
    }

    Result.AssetName = InBlueprint->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InBlueprint);
    if (!JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize Blueprint to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InBlueprint);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InBlueprint, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InBlueprint, TEXT(".txt"));

    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    // Write files
    const auto bJsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto bTextWritten = FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (!bJsonWritten || !bTextWritten)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to write files (JSON: %s, Text: %s)"),
            bJsonWritten ? TEXT("OK") : TEXT("FAILED"),
            bTextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.bSuccess = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_BlueprintExporter::
    ExportBlueprints(
        const TArray<UBlueprint*>& InBlueprints)
    -> TArray<FCk_BlueprintExportResult>
{
    auto Results = TArray<FCk_BlueprintExportResult>{};
    Results.Reserve(InBlueprints.Num());

    for (auto* BP : InBlueprints)
    {
        Results.Add(ExportBlueprint(BP));
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    DoSerializeToJson(
        const UBlueprint* InBlueprint)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InBlueprint->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InBlueprint->GetPathName());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());
    RootObject->SetStringField(TEXT("blueprintType"), InBlueprint->GetClass()->GetName());

    // Parent class
    if (InBlueprint->ParentClass != nullptr)
    {
        RootObject->SetStringField(TEXT("parentClass"), InBlueprint->ParentClass->GetName());
        RootObject->SetStringField(TEXT("parentClassPath"), InBlueprint->ParentClass->GetPathName());
    }
    else
    {
        RootObject->SetStringField(TEXT("parentClass"), TEXT("None"));
    }

    // Variables
    RootObject->SetArrayField(TEXT("variables"),
        DoSerializeVariables_Json(InBlueprint->NewVariables));

    // Event graphs (UbergraphPages)
    auto EventGraphs = TArray<TSharedPtr<FJsonValue>>{};
    for (const UEdGraph* Graph : InBlueprint->UbergraphPages)
    {
        if (IsValid(Graph))
        {
            EventGraphs.Add(MakeShared<FJsonValueObject>(
                DoSerializeGraph_Json(Graph, TEXT("EventGraph"))));
        }
    }
    RootObject->SetArrayField(TEXT("eventGraphs"), EventGraphs);

    // Function graphs
    auto FunctionGraphs = TArray<TSharedPtr<FJsonValue>>{};
    for (const UEdGraph* Graph : InBlueprint->FunctionGraphs)
    {
        if (IsValid(Graph))
        {
            FunctionGraphs.Add(MakeShared<FJsonValueObject>(
                DoSerializeGraph_Json(Graph, TEXT("Function"))));
        }
    }
    RootObject->SetArrayField(TEXT("functionGraphs"), FunctionGraphs);

    // Macro graphs
    auto MacroGraphs = TArray<TSharedPtr<FJsonValue>>{};
    for (const UEdGraph* Graph : InBlueprint->MacroGraphs)
    {
        if (IsValid(Graph))
        {
            MacroGraphs.Add(MakeShared<FJsonValueObject>(
                DoSerializeGraph_Json(Graph, TEXT("Macro"))));
        }
    }
    RootObject->SetArrayField(TEXT("macroGraphs"), MacroGraphs);

    return RootObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializeVariables_Json(
        const TArray<FBPVariableDescription>& InVariables)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto& Var : InVariables)
    {
        auto VarObject = MakeShared<FJsonObject>();

        VarObject->SetStringField(TEXT("varName"), Var.VarName.ToString());
        VarObject->SetStringField(TEXT("friendlyName"), Var.FriendlyName);
        VarObject->SetStringField(TEXT("varType"), DoGetPinTypeAsString(Var.VarType));
        VarObject->SetStringField(TEXT("category"), Var.Category.ToString());
        VarObject->SetStringField(TEXT("defaultValue"), Var.DefaultValue);
        VarObject->SetBoolField(TEXT("isReplicated"), Var.PropertyFlags & CPF_Net);

        if (!Var.RepNotifyFunc.IsNone())
        {
            VarObject->SetStringField(TEXT("repNotifyFunc"), Var.RepNotifyFunc.ToString());
        }

        Result.Add(MakeShared<FJsonValueObject>(VarObject));
    }

    return Result;
}

auto
    FCk_BlueprintExporter::
    DoSerializeGraph_Json(
        const UEdGraph* InGraph,
        const FString& InGraphCategory)
    -> TSharedPtr<FJsonObject>
{
    auto GraphObject = MakeShared<FJsonObject>();

    GraphObject->SetStringField(TEXT("graphName"), InGraph->GetName());
    GraphObject->SetStringField(TEXT("graphCategory"), InGraphCategory);

    // Execution flows
    GraphObject->SetArrayField(TEXT("executionFlows"), DoExtractExecutionFlow_Json(InGraph));

    // All nodes
    auto NodesArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const UEdGraphNode* Node : InGraph->Nodes)
    {
        if (IsValid(Node))
        {
            NodesArray.Add(MakeShared<FJsonValueObject>(DoSerializeNode_Json(Node)));
        }
    }
    GraphObject->SetArrayField(TEXT("nodes"), NodesArray);

    return GraphObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializeNode_Json(
        const UEdGraphNode* InNode)
    -> TSharedPtr<FJsonObject>
{
    auto NodeObject = MakeShared<FJsonObject>();

    NodeObject->SetStringField(TEXT("nodeId"), DoGetNodeId(InNode));
    NodeObject->SetStringField(TEXT("nodeClass"), InNode->GetClass()->GetName());
    NodeObject->SetStringField(TEXT("nodeTitle"), DoGetNodeDisplayName(InNode));

    if (!InNode->NodeComment.IsEmpty())
    {
        NodeObject->SetStringField(TEXT("nodeComment"), InNode->NodeComment);
    }

    // Separate input and output pins
    auto InputPins = TArray<TSharedPtr<FJsonValue>>{};
    auto OutputPins = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto* Pin : InNode->Pins)
    {
        if (Pin == nullptr || Pin->bHidden || Pin->bOrphanedPin)
        { continue; }

        auto PinJson = DoSerializePin_Json(Pin);

        if (Pin->Direction == EGPD_Input)
        {
            InputPins.Add(MakeShared<FJsonValueObject>(PinJson));
        }
        else
        {
            OutputPins.Add(MakeShared<FJsonValueObject>(PinJson));
        }
    }

    NodeObject->SetArrayField(TEXT("inputPins"), InputPins);
    NodeObject->SetArrayField(TEXT("outputPins"), OutputPins);

    return NodeObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializePin_Json(
        const UEdGraphPin* InPin)
    -> TSharedPtr<FJsonObject>
{
    auto PinObject = MakeShared<FJsonObject>();

    PinObject->SetStringField(TEXT("pinName"), InPin->PinName.ToString());
    PinObject->SetStringField(TEXT("pinType"), DoGetPinTypeAsString(InPin->PinType));
    PinObject->SetStringField(TEXT("direction"),
        InPin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));

    if (!InPin->DefaultValue.IsEmpty())
    {
        PinObject->SetStringField(TEXT("defaultValue"), InPin->DefaultValue);
    }
    else if (!InPin->DefaultTextValue.IsEmpty())
    {
        PinObject->SetStringField(TEXT("defaultValue"), InPin->DefaultTextValue.ToString());
    }
    else if (InPin->DefaultObject != nullptr)
    {
        PinObject->SetStringField(TEXT("defaultValue"), InPin->DefaultObject->GetPathName());
    }

    // Connections
    auto Connections = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* LinkedPin : InPin->LinkedTo)
    {
        if (LinkedPin == nullptr || LinkedPin->GetOwningNode() == nullptr)
        { continue; }

        const auto ConnectionStr = FString::Printf(TEXT("%s.%s"),
            *DoGetNodeId(LinkedPin->GetOwningNode()),
            *LinkedPin->PinName.ToString());

        Connections.Add(MakeShared<FJsonValueString>(ConnectionStr));
    }

    if (Connections.Num() > 0)
    {
        PinObject->SetArrayField(TEXT("connections"), Connections);
    }

    return PinObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializePinType_Json(
        const FEdGraphPinType& InPinType)
    -> TSharedPtr<FJsonObject>
{
    auto TypeObject = MakeShared<FJsonObject>();

    TypeObject->SetStringField(TEXT("category"), InPinType.PinCategory.ToString());

    if (!InPinType.PinSubCategory.IsNone())
    {
        TypeObject->SetStringField(TEXT("subCategory"), InPinType.PinSubCategory.ToString());
    }

    if (InPinType.PinSubCategoryObject.IsValid())
    {
        TypeObject->SetStringField(TEXT("subCategoryObject"),
            InPinType.PinSubCategoryObject->GetName());
    }

    if (InPinType.ContainerType == EPinContainerType::Array)
    {
        TypeObject->SetBoolField(TEXT("isArray"), true);
    }
    else if (InPinType.ContainerType == EPinContainerType::Set)
    {
        TypeObject->SetBoolField(TEXT("isSet"), true);
    }
    else if (InPinType.ContainerType == EPinContainerType::Map)
    {
        TypeObject->SetBoolField(TEXT("isMap"), true);
    }

    return TypeObject;
}

auto
    FCk_BlueprintExporter::
    DoExtractExecutionFlow_Json(
        const UEdGraph* InGraph)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Flows = TArray<TSharedPtr<FJsonValue>>{};

    // Find entry point nodes (events, function entries)
    for (const UEdGraphNode* Node : InGraph->Nodes)
    {
        if (!IsValid(Node))
        { continue; }

        const auto bIsEntryPoint =
            Node->IsA<UK2Node_Event>() ||
            Node->IsA<UK2Node_FunctionEntry>();

        if (!bIsEntryPoint)
        { continue; }

        auto FlowObject = MakeShared<FJsonObject>();
        FlowObject->SetStringField(TEXT("entryPoint"), DoGetNodeDisplayName(Node));
        FlowObject->SetStringField(TEXT("entryNodeId"), DoGetNodeId(Node));

        // Walk exec chain
        auto Steps = TArray<TSharedPtr<FJsonValue>>{};
        auto Visited = TSet<const UEdGraphNode*>{};

        struct FWalkItem
        {
            const UEdGraphNode* Node;
            FString BranchLabel;
        };

        auto Stack = TArray<FWalkItem>{};
        Stack.Push({Node, FString{}});

        constexpr auto MaxSteps = int32{200};
        auto StepCount = int32{0};

        while (Stack.Num() > 0 && StepCount < MaxSteps)
        {
            const auto Current = Stack.Pop(EAllowShrinking::No);

            if (Current.Node == nullptr || Visited.Contains(Current.Node))
            { continue; }

            Visited.Add(Current.Node);
            ++StepCount;

            auto StepObject = MakeShared<FJsonObject>();
            StepObject->SetStringField(TEXT("nodeId"), DoGetNodeId(Current.Node));
            StepObject->SetStringField(TEXT("title"), DoGetNodeDisplayName(Current.Node));

            if (!Current.BranchLabel.IsEmpty())
            {
                StepObject->SetStringField(TEXT("branchLabel"), Current.BranchLabel);
            }

            Steps.Add(MakeShared<FJsonValueObject>(StepObject));

            // Find output exec pins and follow them
            auto ExecOutputPins = TArray<const UEdGraphPin*>{};
            for (const auto* Pin : Current.Node->Pins)
            {
                if (Pin != nullptr &&
                    Pin->Direction == EGPD_Output &&
                    DoIsExecPin(Pin) &&
                    Pin->LinkedTo.Num() > 0)
                {
                    ExecOutputPins.Add(Pin);
                }
            }

            // If single exec output, continue chain. If multiple, label branches.
            if (ExecOutputPins.Num() == 1)
            {
                for (const auto* LinkedPin : ExecOutputPins[0]->LinkedTo)
                {
                    if (LinkedPin != nullptr && LinkedPin->GetOwningNode() != nullptr)
                    {
                        Stack.Push({LinkedPin->GetOwningNode(), FString{}});
                    }
                }
            }
            else if (ExecOutputPins.Num() > 1)
            {
                // Reverse iterate to maintain order when popping from stack
                for (auto i = ExecOutputPins.Num() - 1; i >= 0; --i)
                {
                    const auto* ExecPin = ExecOutputPins[i];
                    for (const auto* LinkedPin : ExecPin->LinkedTo)
                    {
                        if (LinkedPin != nullptr && LinkedPin->GetOwningNode() != nullptr)
                        {
                            Stack.Push({LinkedPin->GetOwningNode(),
                                FString::Printf(TEXT("[%s]"), *ExecPin->PinName.ToString())});
                        }
                    }
                }
            }
        }

        FlowObject->SetArrayField(TEXT("steps"), Steps);
        Flows.Add(MakeShared<FJsonValueObject>(FlowObject));
    }

    return Flows;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    DoSerializeToText(
        const UBlueprint* InBlueprint)
    -> FString
{
    auto Text = FString{};

    // Header
    Text += FString::Printf(TEXT("=== Blueprint: %s ===\n"), *InBlueprint->GetName());
    Text += FString::Printf(TEXT("Path: %s\n"), *InBlueprint->GetPathName());
    Text += FString::Printf(TEXT("Type: %s\n"), *InBlueprint->GetClass()->GetName());

    if (InBlueprint->ParentClass != nullptr)
    {
        Text += FString::Printf(TEXT("Parent Class: %s (%s)\n"),
            *InBlueprint->ParentClass->GetName(),
            *InBlueprint->ParentClass->GetPathName());
    }

    Text += FString::Printf(TEXT("Exported: %s\n"), *FDateTime::UtcNow().ToString());
    Text += TEXT("\n");

    // Variables
    DoSerializeVariables_Text(InBlueprint->NewVariables, Text);

    // Event graphs
    for (const UEdGraph* Graph : InBlueprint->UbergraphPages)
    {
        if (IsValid(Graph))
        {
            DoSerializeGraph_Text(Graph, TEXT("Event Graph"), Text);
        }
    }

    // Function graphs
    for (const UEdGraph* Graph : InBlueprint->FunctionGraphs)
    {
        if (IsValid(Graph))
        {
            DoSerializeGraph_Text(Graph, TEXT("Function"), Text);
        }
    }

    // Macro graphs
    for (const UEdGraph* Graph : InBlueprint->MacroGraphs)
    {
        if (IsValid(Graph))
        {
            DoSerializeGraph_Text(Graph, TEXT("Macro"), Text);
        }
    }

    return Text;
}

auto
    FCk_BlueprintExporter::
    DoSerializeVariables_Text(
        const TArray<FBPVariableDescription>& InVariables,
        FString& OutText)
    -> void
{
    if (InVariables.Num() == 0)
    { return; }

    OutText += FString::Printf(TEXT("--- Variables (%d) ---\n"), InVariables.Num());

    for (const auto& Var : InVariables)
    {
        const auto TypeStr = DoGetPinTypeAsString(Var.VarType);
        const auto bReplicated = (Var.PropertyFlags & CPF_Net) != 0;

        OutText += FString::Printf(TEXT("  [%s] %s"),
            *TypeStr, *Var.VarName.ToString());

        if (!Var.DefaultValue.IsEmpty())
        {
            OutText += FString::Printf(TEXT(" = %s"), *Var.DefaultValue);
        }

        // Metadata in parentheses
        auto Meta = TArray<FString>{};

        if (!Var.Category.IsEmpty())
        {
            Meta.Add(FString::Printf(TEXT("Category: %s"), *Var.Category.ToString()));
        }

        if (bReplicated)
        {
            Meta.Add(TEXT("Replicated: Yes"));
            if (!Var.RepNotifyFunc.IsNone())
            {
                Meta.Add(FString::Printf(TEXT("RepNotify: %s"), *Var.RepNotifyFunc.ToString()));
            }
        }

        if (Meta.Num() > 0)
        {
            OutText += FString::Printf(TEXT(" (%s)"), *FString::Join(Meta, TEXT(", ")));
        }

        OutText += TEXT("\n");
    }

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeGraph_Text(
        const UEdGraph* InGraph,
        const FString& InGraphCategory,
        FString& OutText)
    -> void
{
    OutText += FString::Printf(TEXT("--- %s: %s ---\n"), *InGraphCategory, *InGraph->GetName());

    // Execution flows first (most useful for understanding logic)
    DoExtractExecutionFlow_Text(InGraph, OutText);

    // All nodes
    auto ValidNodeCount = int32{0};
    for (const UEdGraphNode* Node : InGraph->Nodes)
    {
        if (IsValid(Node)) { ++ValidNodeCount; }
    }

    OutText += FString::Printf(TEXT("  Nodes (%d):\n"), ValidNodeCount);

    for (const UEdGraphNode* Node : InGraph->Nodes)
    {
        if (IsValid(Node))
        {
            DoSerializeNode_Text(Node, OutText, 2);
        }
    }

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeNode_Text(
        const UEdGraphNode* InNode,
        FString& OutText,
        int32 InDepth)
    -> void
{
    const auto Indent = DoGetIndent(InDepth);
    const auto NodeId = DoGetNodeId(InNode);
    const auto ClassName = InNode->GetClass()->GetName();
    const auto DisplayName = DoGetNodeDisplayName(InNode);

    OutText += FString::Printf(TEXT("%s[%s] \"%s\" (%s)\n"),
        *Indent, *ClassName, *DisplayName, *NodeId);

    if (!InNode->NodeComment.IsEmpty())
    {
        OutText += FString::Printf(TEXT("%s  Comment: %s\n"),
            *Indent, *InNode->NodeComment);
    }

    // Input pins
    auto bHasInputHeader = false;
    for (const auto* Pin : InNode->Pins)
    {
        if (Pin == nullptr || Pin->bHidden || Pin->bOrphanedPin || Pin->Direction != EGPD_Input)
        { continue; }

        if (!bHasInputHeader)
        {
            OutText += FString::Printf(TEXT("%s  In Pins:\n"), *Indent);
            bHasInputHeader = true;
        }

        const auto TypeStr = DoIsExecPin(Pin) ? FString{TEXT("exec")} : DoGetPinTypeAsString(Pin->PinType);

        OutText += FString::Printf(TEXT("%s    (%s) %s"),
            *Indent, *TypeStr, *Pin->PinName.ToString());

        // Show connections or default value
        if (Pin->LinkedTo.Num() > 0)
        {
            auto ConnectionStrs = TArray<FString>{};
            for (const auto* Linked : Pin->LinkedTo)
            {
                if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                {
                    ConnectionStrs.Add(FString::Printf(TEXT("%s.%s"),
                        *DoGetNodeId(Linked->GetOwningNode()),
                        *Linked->PinName.ToString()));
                }
            }
            OutText += FString::Printf(TEXT(" <- %s"), *FString::Join(ConnectionStrs, TEXT(", ")));
        }
        else if (!DoIsExecPin(Pin))
        {
            if (!Pin->DefaultValue.IsEmpty())
            {
                OutText += FString::Printf(TEXT(" = %s"), *Pin->DefaultValue);
            }
            else if (!Pin->DefaultTextValue.IsEmpty())
            {
                OutText += FString::Printf(TEXT(" = %s"), *Pin->DefaultTextValue.ToString());
            }
            else if (Pin->DefaultObject != nullptr)
            {
                OutText += FString::Printf(TEXT(" = %s"), *Pin->DefaultObject->GetName());
            }
        }

        OutText += TEXT("\n");
    }

    // Output pins
    auto bHasOutputHeader = false;
    for (const auto* Pin : InNode->Pins)
    {
        if (Pin == nullptr || Pin->bHidden || Pin->bOrphanedPin || Pin->Direction != EGPD_Output)
        { continue; }

        if (!bHasOutputHeader)
        {
            OutText += FString::Printf(TEXT("%s  Out Pins:\n"), *Indent);
            bHasOutputHeader = true;
        }

        const auto TypeStr = DoIsExecPin(Pin) ? FString{TEXT("exec")} : DoGetPinTypeAsString(Pin->PinType);

        OutText += FString::Printf(TEXT("%s    (%s) %s"),
            *Indent, *TypeStr, *Pin->PinName.ToString());

        if (Pin->LinkedTo.Num() > 0)
        {
            auto ConnectionStrs = TArray<FString>{};
            for (const auto* Linked : Pin->LinkedTo)
            {
                if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                {
                    ConnectionStrs.Add(FString::Printf(TEXT("%s.%s"),
                        *DoGetNodeId(Linked->GetOwningNode()),
                        *Linked->PinName.ToString()));
                }
            }
            OutText += FString::Printf(TEXT(" -> %s"), *FString::Join(ConnectionStrs, TEXT(", ")));
        }

        OutText += TEXT("\n");
    }
}

auto
    FCk_BlueprintExporter::
    DoExtractExecutionFlow_Text(
        const UEdGraph* InGraph,
        FString& OutText)
    -> void
{
    // Find entry points
    auto EntryNodes = TArray<const UEdGraphNode*>{};
    for (const UEdGraphNode* Node : InGraph->Nodes)
    {
        if (!IsValid(Node))
        { continue; }

        if (Node->IsA<UK2Node_Event>() || Node->IsA<UK2Node_FunctionEntry>())
        {
            EntryNodes.Add(Node);
        }
    }

    if (EntryNodes.Num() == 0)
    { return; }

    OutText += TEXT("  Execution Flows:\n");

    auto FlowIndex = int32{1};
    for (const auto* EntryNode : EntryNodes)
    {
        OutText += FString::Printf(TEXT("    [%d] %s"),
            FlowIndex++, *DoGetNodeDisplayName(EntryNode));

        // Walk the exec chain
        auto Visited = TSet<const UEdGraphNode*>{};
        Visited.Add(EntryNode);

        struct FWalkState
        {
            const UEdGraphNode* Node;
            int32 IndentLevel;
            FString Prefix;
        };

        auto Queue = TArray<FWalkState>{};

        // Seed with first exec output
        for (const auto* Pin : EntryNode->Pins)
        {
            if (Pin == nullptr || Pin->Direction != EGPD_Output || !DoIsExecPin(Pin))
            { continue; }

            for (const auto* Linked : Pin->LinkedTo)
            {
                if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                {
                    Queue.Add({Linked->GetOwningNode(), 0, FString{}});
                }
            }
        }

        constexpr auto MaxSteps = int32{200};
        auto StepCount = int32{0};

        while (Queue.Num() > 0 && StepCount < MaxSteps)
        {
            const auto Current = Queue[0];
            Queue.RemoveAt(0);
            ++StepCount;

            if (Current.Node == nullptr || Visited.Contains(Current.Node))
            {
                if (Current.IndentLevel > 0 && !Current.Prefix.IsEmpty())
                {
                    OutText += FString::Printf(TEXT("\n%s%s -> (cycle/end)"),
                        *DoGetIndent(4 + Current.IndentLevel), *Current.Prefix);
                }
                continue;
            }

            Visited.Add(Current.Node);

            if (Current.IndentLevel == 0 && Current.Prefix.IsEmpty())
            {
                OutText += FString::Printf(TEXT(" -> %s"), *DoGetNodeDisplayName(Current.Node));
            }
            else
            {
                OutText += FString::Printf(TEXT("\n%s%s -> %s"),
                    *DoGetIndent(4 + Current.IndentLevel),
                    *Current.Prefix,
                    *DoGetNodeDisplayName(Current.Node));
            }

            // Find exec outputs
            auto ExecOutputPins = TArray<const UEdGraphPin*>{};
            for (const auto* Pin : Current.Node->Pins)
            {
                if (Pin != nullptr &&
                    Pin->Direction == EGPD_Output &&
                    DoIsExecPin(Pin) &&
                    Pin->LinkedTo.Num() > 0)
                {
                    ExecOutputPins.Add(Pin);
                }
            }

            if (ExecOutputPins.Num() == 1)
            {
                for (const auto* Linked : ExecOutputPins[0]->LinkedTo)
                {
                    if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                    {
                        Queue.Insert({Linked->GetOwningNode(), Current.IndentLevel, FString{}}, 0);
                    }
                }
            }
            else if (ExecOutputPins.Num() > 1)
            {
                // Multiple branches - add each with a label
                for (auto i = ExecOutputPins.Num() - 1; i >= 0; --i)
                {
                    const auto* ExecPin = ExecOutputPins[i];
                    for (const auto* Linked : ExecPin->LinkedTo)
                    {
                        if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                        {
                            Queue.Insert({
                                Linked->GetOwningNode(),
                                Current.IndentLevel + 1,
                                FString::Printf(TEXT("[%s]"), *ExecPin->PinName.ToString())
                            }, i == 0 ? 0 : Queue.Num());
                        }
                    }
                }
            }
        }

        OutText += TEXT("\n");
    }

    OutText += TEXT("\n");
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    DoResolveOutputPath(
        const UBlueprint* InBlueprint,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InBlueprint->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;

    return DiskPath;
}

auto
    FCk_BlueprintExporter::
    DoGetNodeId(
        const UEdGraphNode* InNode)
    -> FString
{
    if (InNode == nullptr)
    { return TEXT("None"); }

    return FString::Printf(TEXT("Node_%s"),
        *InNode->NodeGuid.ToString(EGuidFormats::Short));
}

// Returns true if the string only contains printable ASCII characters
static auto
    DoIsCleanAscii(
        const FString& InStr)
    -> bool
{
    for (const auto Ch : InStr)
    {
        if (Ch < 32 || Ch > 126) { return false; }
    }
    return !InStr.IsEmpty();
}

// Attempts to get a clean name from a UObject, returns empty string if garbled
static auto
    DoGetCleanObjectName(
        const TWeakObjectPtr<UObject>& InObj)
    -> FString
{
    if (!InObj.IsValid())
    { return FString{}; }

    const auto Name = InObj->GetName();
    if (DoIsCleanAscii(Name))
    { return Name; }

    // Try the class name of the object instead
    if (InObj->GetClass() != nullptr)
    {
        const auto ClassName = InObj->GetClass()->GetName();
        if (DoIsCleanAscii(ClassName))
        { return ClassName; }
    }

    return FString{};
}

auto
    FCk_BlueprintExporter::
    DoGetPinTypeAsString(
        const FEdGraphPinType& InPinType)
    -> FString
{
    const auto Cat = InPinType.PinCategory;
    auto BaseName = FString{};

    // Map known K2 categories to readable strings
    if (Cat == UEdGraphSchema_K2::PC_Exec)
    {
        BaseName = TEXT("exec");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Boolean)
    {
        BaseName = TEXT("Boolean");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Byte)
    {
        BaseName = TEXT("Byte");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Int)
    {
        BaseName = TEXT("Integer");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Int64)
    {
        BaseName = TEXT("Integer64");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Real)
    {
        const auto Sub = InPinType.PinSubCategory.ToString();
        BaseName = (DoIsCleanAscii(Sub)) ? Sub : TEXT("Float");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Name)
    {
        BaseName = TEXT("Name");
    }
    else if (Cat == UEdGraphSchema_K2::PC_String)
    {
        BaseName = TEXT("String");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Text)
    {
        BaseName = TEXT("Text");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Delegate)
    {
        BaseName = TEXT("Delegate");
    }
    else if (Cat == UEdGraphSchema_K2::PC_MCDelegate)
    {
        BaseName = TEXT("MC Delegate");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Wildcard)
    {
        BaseName = TEXT("Wildcard");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Object || Cat == UEdGraphSchema_K2::PC_SoftObject)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Object") : ObjName;
    }
    else if (Cat == UEdGraphSchema_K2::PC_Class || Cat == UEdGraphSchema_K2::PC_SoftClass)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Class") : FString::Printf(TEXT("Class<%s>"), *ObjName);
    }
    else if (Cat == UEdGraphSchema_K2::PC_Struct)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Struct") : ObjName;
    }
    else if (Cat == UEdGraphSchema_K2::PC_Enum)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Enum") : ObjName;
    }
    else if (Cat == UEdGraphSchema_K2::PC_Interface)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Interface") : ObjName;
    }
    else
    {
        // Unknown category — try to extract something readable
        const auto CatStr = Cat.ToString();
        if (DoIsCleanAscii(CatStr))
        {
            BaseName = CatStr;
        }
        else
        {
            // Last resort: try SubCategoryObject name
            const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
            BaseName = ObjName.IsEmpty() ? TEXT("Pin") : ObjName;
        }
    }

    // Container types
    if (InPinType.ContainerType == EPinContainerType::Array)
    {
        return FString::Printf(TEXT("TArray<%s>"), *BaseName);
    }
    if (InPinType.ContainerType == EPinContainerType::Set)
    {
        return FString::Printf(TEXT("TSet<%s>"), *BaseName);
    }
    if (InPinType.ContainerType == EPinContainerType::Map)
    {
        const auto ValName = DoGetCleanObjectName(InPinType.PinValueType.TerminalSubCategoryObject);
        const auto ValCat = InPinType.PinValueType.TerminalCategory.ToString();
        const auto ValueType = !ValName.IsEmpty() ? ValName
            : (DoIsCleanAscii(ValCat) ? ValCat : FString{TEXT("?")});
        return FString::Printf(TEXT("TMap<%s, %s>"), *BaseName, *ValueType);
    }

    return BaseName;
}

auto
    FCk_BlueprintExporter::
    DoGetNodeDisplayName(
        const UEdGraphNode* InNode)
    -> FString
{
    if (InNode == nullptr)
    { return TEXT("None"); }

    const auto Title = InNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    if (!Title.IsEmpty())
    {
        return Title;
    }

    return InNode->GetClass()->GetName();
}

auto
    FCk_BlueprintExporter::
    DoGetIndent(
        int32 InDepth)
    -> FString
{
    auto Indent = FString{};
    for (auto i = int32{0}; i < InDepth; ++i)
    {
        Indent += TEXT("  ");
    }
    return Indent;
}

auto
    FCk_BlueprintExporter::
    DoIsExecPin(
        const UEdGraphPin* InPin)
    -> bool
{
    if (InPin == nullptr)
    { return false; }

    return InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
}

// --------------------------------------------------------------------------------------------------------------------
