#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UUserDefinedStruct;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_UserDefinedStructExportResult
{
    bool Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

// Exports Blueprint structs (UUserDefinedStruct). A UDS is a struct *type*, not an
// instance, so we export its schema — each field's friendly name, internal name,
// CPP type, category and tooltip — plus the default value of each field read from
// the struct's default instance (reusing FCk_DataAssetExporter's shared serializer).
class CKASSETEXPORTER_API FCk_UserDefinedStructExporter
{
public:
    static auto
    ExportUserDefinedStruct(
        UUserDefinedStruct* InStruct,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonOnly) -> FCk_UserDefinedStructExportResult;

    static auto
    ExportUserDefinedStructs(
        const TArray<UUserDefinedStruct*>& InStructs) -> TArray<FCk_UserDefinedStructExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UUserDefinedStruct* InStruct) -> TSharedPtr<FJsonObject>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UUserDefinedStruct* InStruct) -> FString;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UUserDefinedStruct* InStruct,
        const FString& InExtension) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
