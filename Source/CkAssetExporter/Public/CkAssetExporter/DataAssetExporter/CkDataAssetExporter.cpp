#include "CkDataAssetExporter.h"

#include "CkAssetExporter_Log.h"

#include <Engine/DataAsset.h>
#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>
#include <UObject/UnrealType.h>
#include <UObject/TextProperty.h>

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    ExportDataAsset(
        UDataAsset* InDataAsset)
    -> FCk_DataAssetExportResult
{
    auto Result = FCk_DataAssetExportResult{};

    if (!IsValid(InDataAsset))
    {
        Result.ErrorMessage = TEXT("Invalid DataAsset");
        return Result;
    }

    Result.AssetName = InDataAsset->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InDataAsset);
    if (!JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize DataAsset to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InDataAsset);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InDataAsset, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InDataAsset, TEXT(".txt"));

    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    // Write files
    const auto bJsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto bTextWritten = FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (!bJsonWritten || !bTextWritten)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to write files (JSON: %s, Text: %s)"),
            bJsonWritten ? TEXT("OK") : TEXT("FAILED"),
            bTextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.bSuccess = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_DataAssetExporter::
    ExportDataAssets(
        const TArray<UDataAsset*>& InDataAssets)
    -> TArray<FCk_DataAssetExportResult>
{
    auto Results = TArray<FCk_DataAssetExportResult>{};
    Results.Reserve(InDataAssets.Num());

    for (auto* DA : InDataAssets)
    {
        Results.Add(ExportDataAsset(DA));
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoSerializeToJson(
        const UDataAsset* InDataAsset)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InDataAsset->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InDataAsset->GetPathName());
    RootObject->SetStringField(TEXT("assetClass"), InDataAsset->GetClass()->GetName());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());

    // Parent class chain
    auto ParentChain = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Class = InDataAsset->GetClass()->GetSuperClass();
         Class != nullptr && Class != UObject::StaticClass();
         Class = Class->GetSuperClass())
    {
        ParentChain.Add(MakeShared<FJsonValueString>(Class->GetName()));
    }
    RootObject->SetArrayField(TEXT("parentClasses"), ParentChain);

    // Properties — stop at UDataAsset base (don't serialize UObject internals)
    RootObject->SetArrayField(TEXT("properties"),
        DoSerializeProperties_Json(InDataAsset, UDataAsset::StaticClass()));

    return RootObject;
}

auto
    FCk_DataAssetExporter::
    DoSerializeProperties_Json(
        const UObject* InObject,
        const UClass* InStopAtClass)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Properties = TArray<TSharedPtr<FJsonValue>>{};

    if (InObject == nullptr)
    { return Properties; }

    for (TFieldIterator<FProperty> It(InObject->GetClass()); It; ++It)
    {
        const auto* Property = *It;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        // Skip properties owned by the stop class or its parents
        const auto* OwnerClass = Property->GetOwnerClass();
        if (OwnerClass != nullptr && OwnerClass->IsChildOf(UObject::StaticClass())
            && !OwnerClass->IsChildOf(InStopAtClass))
        {
            // Property is from a class that is NOT a child of InStopAtClass
            // This means it's from UObject itself or something else — skip
        }
        else if (OwnerClass == InStopAtClass)
        {
            // Property is from the stop class itself — skip
            continue;
        }

        auto PropObject = MakeShared<FJsonObject>();
        PropObject->SetStringField(TEXT("name"), Property->GetName());
        PropObject->SetStringField(TEXT("type"), Property->GetCPPType());

        // Category
        const auto Category = Property->GetMetaData(TEXT("Category"));
        if (!Category.IsEmpty())
        {
            PropObject->SetStringField(TEXT("category"), Category);
        }

        // Value
        const auto Value = DoGetPropertyValueAsString(Property, InObject);
        PropObject->SetStringField(TEXT("value"), Value);

        // Tooltip/description
        const auto Tooltip = Property->GetMetaData(TEXT("ToolTip"));
        if (!Tooltip.IsEmpty())
        {
            PropObject->SetStringField(TEXT("tooltip"), Tooltip);
        }

        Properties.Add(MakeShared<FJsonValueObject>(PropObject));
    }

    return Properties;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-Text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoSerializeToText(
        const UDataAsset* InDataAsset)
    -> FString
{
    auto Text = FString{};

    Text += FString::Printf(TEXT("=== DataAsset: %s ===\n"), *InDataAsset->GetName());
    Text += FString::Printf(TEXT("Path: %s\n"), *InDataAsset->GetPathName());
    Text += FString::Printf(TEXT("Class: %s\n"), *InDataAsset->GetClass()->GetName());

    // Parent class chain
    Text += TEXT("Parent Classes: ");
    auto bFirst = true;
    for (const auto* Class = InDataAsset->GetClass()->GetSuperClass();
         Class != nullptr && Class != UObject::StaticClass();
         Class = Class->GetSuperClass())
    {
        if (!bFirst) { Text += TEXT(" -> "); }
        Text += Class->GetName();
        bFirst = false;
    }
    Text += TEXT("\n");

    Text += FString::Printf(TEXT("Exported: %s\n"), *FDateTime::UtcNow().ToString());
    Text += TEXT("\n");

    // Properties
    DoSerializeProperties_Text(InDataAsset, UDataAsset::StaticClass(), Text, 0);

    return Text;
}

auto
    FCk_DataAssetExporter::
    DoSerializeProperties_Text(
        const UObject* InObject,
        const UClass* InStopAtClass,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (InObject == nullptr)
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    // Group properties by category
    auto CategoryProperties = TMap<FString, TArray<const FProperty*>>{};
    auto PropertyOrder = TArray<const FProperty*>{};

    for (TFieldIterator<FProperty> It(InObject->GetClass()); It; ++It)
    {
        const auto* Property = *It;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        const auto* OwnerClass = Property->GetOwnerClass();
        if (OwnerClass == InStopAtClass)
        { continue; }

        const auto Category = Property->GetMetaData(TEXT("Category"));
        const auto CategoryKey = Category.IsEmpty() ? FString{TEXT("Uncategorized")} : Category;
        CategoryProperties.FindOrAdd(CategoryKey).Add(Property);
        PropertyOrder.Add(Property);
    }

    if (PropertyOrder.Num() == 0)
    {
        OutText += FString::Printf(TEXT("%s(No exported properties)\n"), *Indent);
        return;
    }

    OutText += FString::Printf(TEXT("%s--- Properties (%d) ---\n"), *Indent, PropertyOrder.Num());

    // Output grouped by category
    for (const auto& [Category, Props] : CategoryProperties)
    {
        OutText += FString::Printf(TEXT("%s  [%s]\n"), *Indent, *Category);

        for (const auto* Property : Props)
        {
            const auto TypeStr = Property->GetCPPType();
            const auto Value = DoGetPropertyValueAsString(Property, InObject);

            OutText += FString::Printf(TEXT("%s    (%s) %s = %s\n"),
                *Indent, *TypeStr, *Property->GetName(), *Value);
        }
    }

    OutText += TEXT("\n");
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoResolveOutputPath(
        const UDataAsset* InDataAsset,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InDataAsset->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;
    return DiskPath;
}

auto
    FCk_DataAssetExporter::
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer)
    -> FString
{
    auto Value = FString{};
    const auto* ValuePtr = InProperty->ContainerPtrToValuePtr<void>(InContainer);
    InProperty->ExportTextItem_Direct(Value, ValuePtr, nullptr, nullptr, PPF_None);
    return Value;
}

auto
    FCk_DataAssetExporter::
    DoShouldIncludeProperty(
        const FProperty* InProperty)
    -> bool
{
    if (InProperty == nullptr)
    { return false; }

    // Skip transient and deprecated properties
    if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
    { return false; }

    // Skip properties owned by UObject itself
    const auto* OwnerClass = InProperty->GetOwnerClass();
    if (OwnerClass == nullptr)
    { return false; }

    if (OwnerClass == UObject::StaticClass())
    { return false; }

    // Include if the property is EditAnywhere, VisibleAnywhere, or BlueprintVisible
    if (InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
    { return true; }

    return false;
}

auto
    FCk_DataAssetExporter::
    DoGetIndent(
        int32 InDepth)
    -> FString
{
    auto Result = FString{};
    for (auto i = int32{0}; i < InDepth; ++i)
    {
        Result += TEXT("  ");
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
