#include "CkUserDefinedStructExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/DataAssetExporter/CkDataAssetExporter.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <StructUtils/UserDefinedStruct.h>
#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>
#include <UObject/UnrealType.h>
#include <UObject/Class.h>

// --------------------------------------------------------------------------------------------------------------------
// Internal helpers
// --------------------------------------------------------------------------------------------------------------------

namespace ck_user_defined_struct_exporter_internal
{
    // UDS user fields are never transient/deprecated, but apply the same lighter
    // filter the shared struct-field walker uses for consistency.
    static auto
        DoShouldIncludeField(
            const FProperty* InProperty)
        -> bool
    {
        if (InProperty == nullptr)
        { return false; }

        return NOT InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedStructExporter::
    ExportUserDefinedStruct(
        UUserDefinedStruct* InStruct)
    -> FCk_UserDefinedStructExportResult
{
    auto Result = FCk_UserDefinedStructExportResult{};

    if (ck::Is_NOT_Valid(InStruct))
    {
        Result.ErrorMessage = TEXT("Invalid UserDefinedStruct");
        return Result;
    }

    Result.AssetName = InStruct->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InStruct);
    if (NOT JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize UserDefinedStruct to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InStruct);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InStruct, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InStruct, TEXT(".txt"));

    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    // Write files
    const auto JsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (NOT JsonWritten || NOT TextWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, Text: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"),
            TextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_UserDefinedStructExporter::
    ExportUserDefinedStructs(
        const TArray<UUserDefinedStruct*>& InStructs)
    -> TArray<FCk_UserDefinedStructExportResult>
{
    auto Results = TArray<FCk_UserDefinedStructExportResult>{};
    Results.Reserve(InStructs.Num());

    ck::algo::ForEach(InStructs, [&](UUserDefinedStruct* Struct)
    {
        Results.Add(ExportUserDefinedStruct(Struct));
    });

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedStructExporter::
    DoSerializeToJson(
        const UUserDefinedStruct* InStruct)
    -> TSharedPtr<FJsonObject>
{
    using namespace ck_user_defined_struct_exporter_internal;

    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InStruct->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InStruct->GetPathName());
    RootObject->SetStringField(TEXT("assetClass"), InStruct->GetClass()->GetName());
    RootObject->SetStringField(TEXT("structGuid"), InStruct->GetCustomGuid().ToString());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());

    // The shared serializer keeps thread-local dedup state across calls — reset it
    // so stale entries from a previous (DataAsset/Blueprint) export don't leak in.
    FCk_DataAssetExporter::ResetSharedRecursionState();

    // Default values are read from the struct's default instance (the authored
    // per-field defaults). May be null if the struct isn't up to date.
    const auto* DefaultInstance = InStruct->GetDefaultInstance();

    auto FieldsArray = TArray<TSharedPtr<FJsonValue>>{};

    for (TFieldIterator<FProperty> It(InStruct); It; ++It)
    {
        const auto* Property = *It;

        if (NOT DoShouldIncludeField(Property))
        { continue; }

        auto FieldObject = MakeShared<FJsonObject>();

        // GetAuthoredName() strips the GUID suffix UDS appends to field names,
        // yielding the friendly name the designer typed (GetName() keeps the suffix).
        FieldObject->SetStringField(TEXT("name"), Property->GetAuthoredName());
        FieldObject->SetStringField(TEXT("internalName"), Property->GetName());
        FieldObject->SetStringField(TEXT("type"), Property->GetCPPType());

        const auto Category = Property->GetMetaData(TEXT("Category"));
        if (NOT Category.IsEmpty())
        {
            FieldObject->SetStringField(TEXT("category"), Category);
        }

        const auto Tooltip = Property->GetMetaData(TEXT("ToolTip"));
        if (NOT Tooltip.IsEmpty())
        {
            FieldObject->SetStringField(TEXT("tooltip"), Tooltip);
        }

        if (ck::IsValid(DefaultInstance, ck::IsValid_Policy_NullptrOnly{}))
        {
            const auto* ValuePtr = Property->ContainerPtrToValuePtr<void>(DefaultInstance);
            const auto JsonValue = FCk_DataAssetExporter::DoSerializePropertyValue_Json(Property, ValuePtr);
            FieldObject->SetField(TEXT("defaultValue"),
                JsonValue.IsValid() ? JsonValue : MakeShared<FJsonValueNull>());
        }
        else
        {
            FieldObject->SetField(TEXT("defaultValue"), MakeShared<FJsonValueNull>());
        }

        FieldsArray.Add(MakeShared<FJsonValueObject>(FieldObject));
    }

    RootObject->SetArrayField(TEXT("fields"), FieldsArray);

    return RootObject;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-Text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedStructExporter::
    DoSerializeToText(
        const UUserDefinedStruct* InStruct)
    -> FString
{
    using namespace ck_user_defined_struct_exporter_internal;

    auto Text = FString{};

    Text += ck::Format_UE(TEXT("=== UserDefinedStruct: {} ===\n"), InStruct->GetName());
    Text += ck::Format_UE(TEXT("Path: {}\n"), InStruct->GetPathName());
    Text += ck::Format_UE(TEXT("GUID: {}\n"), InStruct->GetCustomGuid().ToString());
    Text += ck::Format_UE(TEXT("Exported: {}\n\n"), FDateTime::UtcNow().ToString());

    const auto* DefaultInstance = InStruct->GetDefaultInstance();
    const auto HasDefaults = ck::IsValid(DefaultInstance, ck::IsValid_Policy_NullptrOnly{});

    auto Fields = TArray<const FProperty*>{};
    for (TFieldIterator<FProperty> It(InStruct); It; ++It)
    {
        if (DoShouldIncludeField(*It))
        {
            Fields.Add(*It);
        }
    }

    if (Fields.Num() == 0)
    {
        Text += TEXT("(No fields)\n");
        return Text;
    }

    Text += ck::Format_UE(TEXT("--- Fields ({}) ---\n"), Fields.Num());

    ck::algo::ForEach(Fields, [&](const FProperty* Property)
    {
        auto DefaultValue = FString{};
        if (HasDefaults)
        {
            const auto* ValuePtr = Property->ContainerPtrToValuePtr<void>(DefaultInstance);
            Property->ExportTextItem_Direct(DefaultValue, ValuePtr, nullptr, nullptr, PPF_None);
        }

        Text += ck::Format_UE(TEXT("  ({}) {} = {}\n"),
            Property->GetCPPType(),
            Property->GetAuthoredName(),
            DefaultValue);
    });

    return Text;
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedStructExporter::
    DoResolveOutputPath(
        const UUserDefinedStruct* InStruct,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InStruct->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;
    return DiskPath;
}

// --------------------------------------------------------------------------------------------------------------------
