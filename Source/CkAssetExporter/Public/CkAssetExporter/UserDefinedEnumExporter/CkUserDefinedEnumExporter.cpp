#include "CkUserDefinedEnumExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/UserDefinedEnum.h>
#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>

// --------------------------------------------------------------------------------------------------------------------
// Internal helpers
// --------------------------------------------------------------------------------------------------------------------

namespace ck_user_defined_enum_exporter_internal
{
    // UUserDefinedEnum always appends an implicit, hidden <EnumName>_MAX entry as the
    // final enumerator. NumEnums() counts it; the canonical engine idiom (mirrored by
    // FEnumEditorUtils) is to iterate [0, NumEnums()-1) so the authored entries are
    // exported and the auto-generated MAX is skipped.
    static auto
        DoGetAuthoredEnumeratorCount(
            const UUserDefinedEnum* InEnum)
        -> int32
    {
        const auto NumEnums = InEnum->NumEnums();
        return NumEnums > 0 ? NumEnums - 1 : 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedEnumExporter::
    ExportUserDefinedEnum(
        UUserDefinedEnum* InEnum)
    -> FCk_UserDefinedEnumExportResult
{
    auto Result = FCk_UserDefinedEnumExportResult{};

    if (ck::Is_NOT_Valid(InEnum))
    {
        Result.ErrorMessage = TEXT("Invalid UserDefinedEnum");
        return Result;
    }

    Result.AssetName = InEnum->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InEnum);
    if (NOT JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize UserDefinedEnum to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InEnum);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InEnum, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InEnum, TEXT(".txt"));

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
    FCk_UserDefinedEnumExporter::
    ExportUserDefinedEnums(
        const TArray<UUserDefinedEnum*>& InEnums)
    -> TArray<FCk_UserDefinedEnumExportResult>
{
    auto Results = TArray<FCk_UserDefinedEnumExportResult>{};
    Results.Reserve(InEnums.Num());

    ck::algo::ForEach(InEnums, [&](UUserDefinedEnum* Enum)
    {
        Results.Add(ExportUserDefinedEnum(Enum));
    });

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedEnumExporter::
    DoSerializeToJson(
        const UUserDefinedEnum* InEnum)
    -> TSharedPtr<FJsonObject>
{
    using namespace ck_user_defined_enum_exporter_internal;

    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InEnum->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InEnum->GetPathName());
    RootObject->SetStringField(TEXT("assetClass"), InEnum->GetClass()->GetName());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());

    const auto EnumTooltip = InEnum->GetMetaData(TEXT("ToolTip"));
    if (NOT EnumTooltip.IsEmpty())
    {
        RootObject->SetStringField(TEXT("tooltip"), EnumTooltip);
    }

    const auto Count = DoGetAuthoredEnumeratorCount(InEnum);

    auto EnumeratorsArray = TArray<TSharedPtr<FJsonValue>>{};

    for (auto Index = int32{0}; Index < Count; ++Index)
    {
        auto EnumeratorObject = MakeShared<FJsonObject>();

        // DisplayName is the friendly authored label (UDE's DisplayNameMap); falls back
        // to the internal name when the designer never set one.
        EnumeratorObject->SetStringField(TEXT("displayName"), InEnum->GetDisplayNameTextByIndex(Index).ToString());
        EnumeratorObject->SetStringField(TEXT("internalName"), InEnum->GetNameStringByIndex(Index));
        EnumeratorObject->SetStringField(TEXT("fullName"), InEnum->GetNameByIndex(Index).ToString());
        EnumeratorObject->SetNumberField(TEXT("value"), static_cast<double>(InEnum->GetValueByIndex(Index)));

        const auto Tooltip = InEnum->GetMetaData(TEXT("ToolTip"), Index);
        if (NOT Tooltip.IsEmpty())
        {
            EnumeratorObject->SetStringField(TEXT("tooltip"), Tooltip);
        }

        EnumeratorsArray.Add(MakeShared<FJsonValueObject>(EnumeratorObject));
    }

    RootObject->SetArrayField(TEXT("enumerators"), EnumeratorsArray);

    return RootObject;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-Text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedEnumExporter::
    DoSerializeToText(
        const UUserDefinedEnum* InEnum)
    -> FString
{
    using namespace ck_user_defined_enum_exporter_internal;

    auto Text = FString{};

    Text += ck::Format_UE(TEXT("=== UserDefinedEnum: {} ===\n"), InEnum->GetName());
    Text += ck::Format_UE(TEXT("Path: {}\n"), InEnum->GetPathName());

    const auto EnumTooltip = InEnum->GetMetaData(TEXT("ToolTip"));
    if (NOT EnumTooltip.IsEmpty())
    {
        Text += ck::Format_UE(TEXT("Tooltip: {}\n"), EnumTooltip);
    }

    Text += ck::Format_UE(TEXT("Exported: {}\n\n"), FDateTime::UtcNow().ToString());

    const auto Count = DoGetAuthoredEnumeratorCount(InEnum);

    if (Count == 0)
    {
        Text += TEXT("(No enumerators)\n");
        return Text;
    }

    Text += ck::Format_UE(TEXT("--- Enumerators ({}) ---\n"), Count);

    for (auto Index = int32{0}; Index < Count; ++Index)
    {
        const auto Tooltip = InEnum->GetMetaData(TEXT("ToolTip"), Index);

        Text += ck::Format_UE(TEXT("  [{}] {} ({}){}\n"),
            InEnum->GetValueByIndex(Index),
            InEnum->GetDisplayNameTextByIndex(Index).ToString(),
            InEnum->GetNameStringByIndex(Index),
            Tooltip.IsEmpty() ? FString{} : ck::Format_UE(TEXT(" - {}"), Tooltip));
    }

    return Text;
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UserDefinedEnumExporter::
    DoResolveOutputPath(
        const UUserDefinedEnum* InEnum,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InEnum->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;
    return DiskPath;
}

// --------------------------------------------------------------------------------------------------------------------
