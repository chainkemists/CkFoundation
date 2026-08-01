#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UStateTree;
class UStateTreeState;

struct FStateTreeEditorNode;
struct FStateTreeTransition;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_StateTreeExportResult
{
    bool Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_StateTreeExporter
{
public:
    static auto
    ExportStateTree(
        UStateTree* InStateTree,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonAndText) -> FCk_StateTreeExportResult;

    static auto
    ExportStateTrees(
        const TArray<UStateTree*>& InStateTrees) -> TArray<FCk_StateTreeExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UStateTree* InStateTree) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeState_Json(
        const UStateTreeState* InState) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeEditorNode_Json(
        const FStateTreeEditorNode& InNode,
        const FString& InNodeType) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeTransition_Json(
        const FStateTreeTransition& InTransition) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeProperties_Json(
        const UStruct* InStruct,
        const void* InContainer,
        const void* InDefaultContainer) -> TSharedPtr<FJsonObject>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UStateTree* InStateTree) -> FString;

    static auto
    DoSerializeState_Text(
        const UStateTreeState* InState,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeEditorNode_Text(
        const FStateTreeEditorNode& InNode,
        const FString& InNodeType,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeTransition_Text(
        const FStateTreeTransition& InTransition,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeProperties_Text(
        const UStruct* InStruct,
        const void* InContainer,
        const void* InDefaultContainer,
        FString& OutText,
        int32 InDepth) -> void;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UStateTree* InStateTree,
        const FString& InExtension) -> FString;

    static auto
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer) -> FString;

    static auto
    DoShouldIncludeProperty(
        const FProperty* InProperty) -> bool;

    static auto
    DoGetIndent(
        int32 InDepth) -> FString;

    static auto
    DoGetEditorNodeDisplayInfo(
        const FStateTreeEditorNode& InNode,
        FString& OutClassName,
        FString& OutNodeName) -> void;

    static auto
    DoGetEnumValueAsString(
        const UEnum* InEnum,
        int64 InValue) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
