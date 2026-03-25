#pragma once

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UBehaviorTree;
class UBTNode;
class UBTCompositeNode;
class UBTTaskNode;
class UBTDecorator;
class UBTService;
class UBlackboardData;

struct FBTCompositeChild;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_BehaviorTreeExportResult
{
    bool bSuccess = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_BehaviorTreeExporter
{
public:
    static auto
    ExportBehaviorTree(
        UBehaviorTree* InBehaviorTree) -> FCk_BehaviorTreeExportResult;

    static auto
    ExportBehaviorTrees(
        const TArray<UBehaviorTree*>& InBehaviorTrees) -> TArray<FCk_BehaviorTreeExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UBehaviorTree* InBehaviorTree) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeCompositeNode_Json(
        const UBTCompositeNode* InNode) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeTaskNode_Json(
        const UBTTaskNode* InNode) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeDecorators_Json(
        const TArray<UBTDecorator*>& InDecorators) -> TArray<TSharedPtr<FJsonValue>>;

    static auto
    DoSerializeServices_Json(
        const TArray<UBTService*>& InServices) -> TArray<TSharedPtr<FJsonValue>>;

    static auto
    DoSerializeNodeProperties_Json(
        const UBTNode* InNode) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeBlackboard_Json(
        const UBlackboardData* InBlackboardData) -> TSharedPtr<FJsonObject>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UBehaviorTree* InBehaviorTree) -> FString;

    static auto
    DoSerializeCompositeNode_Text(
        const UBTCompositeNode* InNode,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeTaskNode_Text(
        const UBTTaskNode* InNode,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeDecorators_Text(
        const TArray<UBTDecorator*>& InDecorators,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeServices_Text(
        const TArray<UBTService*>& InServices,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeNodeProperties_Text(
        const UBTNode* InNode,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeBlackboard_Text(
        const UBlackboardData* InBlackboardData,
        FString& OutText) -> void;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UBehaviorTree* InBehaviorTree,
        const FString& InExtension) -> FString;

    static auto
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer) -> FString;

    static auto
    DoGetPropertyDefaultAnnotation(
        const FProperty* InProperty,
        const UBTNode* InNode,
        const UObject* InCDO) -> FString;

    static auto
    DoShouldIncludeProperty(
        const FProperty* InProperty) -> bool;

    static auto
    DoGetIndent(
        int32 InDepth) -> FString;

    static auto
    DoGetNodeDisplayInfo(
        const UBTNode* InNode,
        FString& OutClassName,
        FString& OutNodeName,
        FString& OutDescription) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
