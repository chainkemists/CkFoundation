#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UUserDefinedEnum;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_UserDefinedEnumExportResult
{
    bool Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

// Exports Blueprint enums (UUserDefinedEnum). A UDE is an enum *type*, so we export
// its schema — each enumerator's authored display name, internal name, fully-qualified
// name, numeric value and tooltip — plus the enum's own asset-level tooltip.
class CKASSETEXPORTER_API FCk_UserDefinedEnumExporter
{
public:
    static auto
    ExportUserDefinedEnum(
        UUserDefinedEnum* InEnum,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonOnly) -> FCk_UserDefinedEnumExportResult;

    static auto
    ExportUserDefinedEnums(
        const TArray<UUserDefinedEnum*>& InEnums) -> TArray<FCk_UserDefinedEnumExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UUserDefinedEnum* InEnum) -> TSharedPtr<FJsonObject>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UUserDefinedEnum* InEnum) -> FString;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UUserDefinedEnum* InEnum,
        const FString& InExtension) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
