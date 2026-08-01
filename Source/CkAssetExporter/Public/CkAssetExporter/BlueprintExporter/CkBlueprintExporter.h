#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UActorComponent;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UFunction;
class UClass;
class UWidgetBlueprint;
class UWidget;
class UWidgetAnimation;

struct FBPVariableDescription;
struct FEdGraphPinType;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_BlueprintExportResult
{
    bool Succeeded = false;
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
        UBlueprint* InBlueprint,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonAndText) -> FCk_BlueprintExportResult;

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

    static auto
    DoSerializeComponents_Json(
        const UBlueprint* InBlueprint) -> TArray<TSharedPtr<FJsonValue>>;

    static auto
    DoSerializeComponent_Json(
        const UActorComponent* InComponent,
        const FName& InVariableName,
        const FName& InAttachParent,
        const FName& InAttachSocket,
        const FString& InOrigin) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeImplementedInterfaces_Json(
        const UBlueprint* InBlueprint) -> TArray<TSharedPtr<FJsonValue>>;

    static auto
    DoSerializeInterfaceFunction_Json(
        const UFunction* InFunction) -> TSharedPtr<FJsonObject>;

    // ---- Widget Blueprint (UMG) serialization ----

    static auto
    DoSerializeWidgetHierarchy_Json(
        const UWidgetBlueprint* InWidgetBlueprint) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeWidget_Json(
        const UWidget* InWidget,
        const FString& InSlotName,
        int32 InDepth) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeAnimations_Json(
        const UWidgetBlueprint* InWidgetBlueprint) -> TArray<TSharedPtr<FJsonValue>>;

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

    static auto
    DoSerializeComponents_Text(
        const UBlueprint* InBlueprint,
        FString& OutText) -> void;

    static auto
    DoSerializeImplementedInterfaces_Text(
        const UBlueprint* InBlueprint,
        FString& OutText) -> void;

    static auto
    DoSerializeWidgetHierarchy_Text(
        const UWidgetBlueprint* InWidgetBlueprint,
        FString& OutText) -> void;

    static auto
    DoSerializeWidget_Text(
        const UWidget* InWidget,
        const FString& InSlotName,
        int32 InDepth,
        FString& OutText) -> void;

    static auto
    DoSerializeAnimations_Text(
        const UWidgetBlueprint* InWidgetBlueprint,
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
