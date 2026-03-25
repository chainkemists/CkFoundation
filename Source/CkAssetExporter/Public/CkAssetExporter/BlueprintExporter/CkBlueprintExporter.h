#pragma once

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

struct FBPVariableDescription;
struct FEdGraphPinType;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_BlueprintExportResult
{
    bool bSuccess = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_BlueprintExporter
{
public:
    static auto
    ExportBlueprint(
        UBlueprint* InBlueprint) -> FCk_BlueprintExportResult;

    static auto
    ExportBlueprints(
        const TArray<UBlueprint*>& InBlueprints) -> TArray<FCk_BlueprintExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UBlueprint* InBlueprint) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeVariables_Json(
        const TArray<FBPVariableDescription>& InVariables) -> TArray<TSharedPtr<FJsonValue>>;

    static auto
    DoSerializeGraph_Json(
        const UEdGraph* InGraph,
        const FString& InGraphCategory) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeNode_Json(
        const UEdGraphNode* InNode) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializePin_Json(
        const UEdGraphPin* InPin) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializePinType_Json(
        const FEdGraphPinType& InPinType) -> TSharedPtr<FJsonObject>;

    static auto
    DoExtractExecutionFlow_Json(
        const UEdGraph* InGraph) -> TArray<TSharedPtr<FJsonValue>>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UBlueprint* InBlueprint) -> FString;

    static auto
    DoSerializeVariables_Text(
        const TArray<FBPVariableDescription>& InVariables,
        FString& OutText) -> void;

    static auto
    DoSerializeGraph_Text(
        const UEdGraph* InGraph,
        const FString& InGraphCategory,
        FString& OutText) -> void;

    static auto
    DoSerializeNode_Text(
        const UEdGraphNode* InNode,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoExtractExecutionFlow_Text(
        const UEdGraph* InGraph,
        FString& OutText) -> void;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UBlueprint* InBlueprint,
        const FString& InExtension) -> FString;

    static auto
    DoGetNodeId(
        const UEdGraphNode* InNode) -> FString;

    static auto
    DoGetPinTypeAsString(
        const FEdGraphPinType& InPinType) -> FString;

    static auto
    DoGetNodeDisplayName(
        const UEdGraphNode* InNode) -> FString;

    static auto
    DoGetIndent(
        int32 InDepth) -> FString;

    static auto
    DoIsExecPin(
        const UEdGraphPin* InPin) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
