#pragma once

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UDataAsset;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_DataAssetExportResult
{
    bool bSuccess = false;
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
        UDataAsset* InDataAsset) -> FCk_DataAssetExportResult;

    static auto
    ExportDataAssets(
        const TArray<UDataAsset*>& InDataAssets) -> TArray<FCk_DataAssetExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UDataAsset* InDataAsset) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeProperties_Json(
        const UObject* InObject,
        const UClass* InStopAtClass) -> TArray<TSharedPtr<FJsonValue>>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UDataAsset* InDataAsset) -> FString;

    static auto
    DoSerializeProperties_Text(
        const UObject* InObject,
        const UClass* InStopAtClass,
        FString& OutText,
        int32 InDepth) -> void;

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
    DoShouldIncludeProperty(
        const FProperty* InProperty) -> bool;

    static auto
    DoGetIndent(
        int32 InDepth) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
