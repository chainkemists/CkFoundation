#include "CkDataTableExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/DataTable.h>
#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <UObject/Class.h>
#include <UObject/Object.h>
#include <UObject/Package.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_data_table_exporter
{
    static auto
    DoResolveOutputPath(
        const UDataTable* InDataTable,
        const FString& InExtension)
        -> FString
    {
        const auto PackageName = InDataTable->GetOutermost()->GetName();

        auto DiskPath = FString{};
        if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
        { return {}; }

        return DiskPath + InExtension;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataTableExporter::
    ExportDataTable(
        UDataTable* InDataTable)
    -> FCk_DataTableExportResult
{
    auto Result = FCk_DataTableExportResult{};

    if (ck::Is_NOT_Valid(InDataTable))
    {
        Result.ErrorMessage = TEXT("Invalid DataTable");
        return Result;
    }

    Result.AssetName = InDataTable->GetName();

    const auto JsonPath = ck_data_table_exporter::DoResolveOutputPath(InDataTable, ck::asset_exporter::extension::Sidecar);
    const auto CsvPath  = ck_data_table_exporter::DoResolveOutputPath(InDataTable, ck::asset_exporter::extension::Csv);
    if (JsonPath.IsEmpty() || CsvPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    // Rows come from the engine's own serializer; re-parse them so they nest inside our root object as structured
    // JSON rather than an escaped string. A parse failure is loud (never a silent empty "rows").
    const auto RowsJsonString = InDataTable->GetTableAsJSON();
    auto RowsArray = TArray<TSharedPtr<FJsonValue>>{};
    {
        const auto Reader = TJsonReaderFactory<>::Create(RowsJsonString);
        if (NOT FJsonSerializer::Deserialize(Reader, RowsArray))
        {
            Result.ErrorMessage = ck::Format_UE(
                TEXT("Failed to parse GetTableAsJSON output for DataTable [{}]"), Result.AssetName);
            return Result;
        }
    }

    auto RootObject = MakeShared<FJsonObject>();
    RootObject->SetStringField(TEXT("assetName"), InDataTable->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InDataTable->GetPathName());
    RootObject->SetStringField(TEXT("assetClass"), InDataTable->GetClass()->GetName());
    RootObject->SetObjectField(TEXT("_meta"),
        FCk_AssetExportMeta::MakeMetaObject(InDataTable, ck::asset_exporter::version::DataTable));

    const auto* RowStruct = InDataTable->GetRowStruct();
    RootObject->SetStringField(TEXT("rowStruct"), RowStruct != nullptr ? RowStruct->GetPathName() : FString{TEXT("None")});
    RootObject->SetArrayField(TEXT("rows"), RowsArray);

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject, JsonWriter);

    const auto CsvString = InDataTable->GetTableAsCSV();

    const auto CsvWritten = FFileHelper::SaveStringToFile(
        CsvString, *CsvPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto JsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (NOT JsonWritten || NOT CsvWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, CSV: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"),
            CsvWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    Result.CsvFilePath = CsvPath;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
