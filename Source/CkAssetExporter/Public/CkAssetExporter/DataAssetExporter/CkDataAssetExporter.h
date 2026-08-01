#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UDataAsset;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_DataAssetExportResult
{
    bool Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_DataAssetExporter
{
public:
    static auto
    ExportDataAsset(
        UDataAsset* InDataAsset,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonAndText) -> FCk_DataAssetExportResult;

    static auto
    ExportDataAssets(
        const TArray<UDataAsset*>& InDataAssets) -> TArray<FCk_DataAssetExportResult>;

public:
    // ---- Shared JSON property serialization (reused by BlueprintExporter) ----

    static auto
    DoSerializeProperties_Json(
        const UObject* InObject,
        const UClass* InStopAtClass) -> TArray<TSharedPtr<FJsonValue>>;

    static auto
    DoSerializePropertyValue_Json(
        const FProperty* InProperty,
        const void* InValuePtr) -> TSharedPtr<FJsonValue>;

    static auto
    DoSerializeObjectProperties_Json(
        const UObject* InObject,
        const UClass* InStopAtClass) -> TSharedPtr<FJsonObject>;

    // ---- Shared plain-text property serialization (reused by BlueprintExporter) ----

    static auto
    DoSerializeProperties_Text(
        const UObject* InObject,
        const UClass* InStopAtClass,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoShouldIncludeProperty(
        const FProperty* InProperty) -> bool;

    // Resets the thread-local recursion/dedup bookkeeping used by the shared
    // serializers. Call this before driving DoSerializeProperties_Json /
    // DoSerializePropertyValue_Json from a non-DataAsset exporter so stale
    // "alreadyExported" entries from a previous export don't leak in.
    static auto
    ResetSharedRecursionState() -> void;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UDataAsset* InDataAsset) -> TSharedPtr<FJsonObject>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UDataAsset* InDataAsset) -> FString;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UDataAsset* InDataAsset,
        const FString& InExtension) -> FString;

    static auto
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer) -> FString;

    static auto
    DoGetIndent(
        int32 InDepth) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
