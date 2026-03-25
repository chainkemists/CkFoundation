#include "CkEQSExporter.h"

#include "CkAssetExporter_Log.h"

#include <EnvironmentQuery/EnvQuery.h>
#include <EnvironmentQuery/EnvQueryOption.h>
#include <EnvironmentQuery/EnvQueryGenerator.h>
#include <EnvironmentQuery/EnvQueryTest.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>

#include <UObject/UnrealType.h>
#include <UObject/FieldIterator.h>
#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Helpers (internal)
// --------------------------------------------------------------------------------------------------------------------

// UEnvQuery::Options is protected, so we access it via the UProperty reflection system.
static auto
    DoGetQueryOptions(
        const UEnvQuery* InQuery)
    -> TArray<UEnvQueryOption*>
{
    auto Result = TArray<UEnvQueryOption*>{};

    const auto* Prop = UEnvQuery::StaticClass()->FindPropertyByName(TEXT("Options"));
    if (Prop == nullptr)
    { return Result; }

    const auto* ArrayProp = CastField<FArrayProperty>(Prop);
    if (ArrayProp == nullptr)
    { return Result; }

    const auto* InnerProp = CastField<FObjectProperty>(ArrayProp->Inner);
    if (InnerProp == nullptr)
    { return Result; }

    const auto* ValuePtr = Prop->ContainerPtrToValuePtr<void>(InQuery);
    auto Helper = FScriptArrayHelper(ArrayProp, ValuePtr);

    for (int32 i = 0; i < Helper.Num(); ++i)
    {
        auto* Obj = InnerProp->GetObjectPropertyValue(Helper.GetRawPtr(i));
        if (auto* Option = Cast<UEnvQueryOption>(Obj))
        {
            Result.Add(Option);
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_EQSExporter::
    ExportEQS(
        UEnvQuery* InQuery)
    -> FCk_EQSExportResult
{
    auto Result = FCk_EQSExportResult{};

    if (!IsValid(InQuery))
    {
        Result.ErrorMessage = TEXT("Invalid EQS Query asset");
        return Result;
    }

    Result.AssetName = InQuery->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InQuery);
    if (!JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize EQS Query to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InQuery);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InQuery, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InQuery, TEXT(".txt"));

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
    FCk_EQSExporter::
    ExportEQSQueries(
        const TArray<UEnvQuery*>& InQueries)
    -> TArray<FCk_EQSExportResult>
{
    auto Results = TArray<FCk_EQSExportResult>{};
    Results.Reserve(InQueries.Num());

    for (auto* Query : InQueries)
    {
        Results.Add(ExportEQS(Query));
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_EQSExporter::
    DoSerializeToJson(
        const UEnvQuery* InQuery)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InQuery->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InQuery->GetPathName());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());

    // Options (accessed via reflection since UEnvQuery::Options is protected)
    auto OptionsArray = TArray<TSharedPtr<FJsonValue>>{};
    const auto Options = DoGetQueryOptions(InQuery);

    for (int32 OptionIdx = 0; OptionIdx < Options.Num(); ++OptionIdx)
    {
        const auto* Option = Options[OptionIdx];
        if (!IsValid(Option))
        { continue; }

        auto OptionObject = DoSerializeOption_Json(Option, OptionIdx);
        OptionsArray.Add(MakeShared<FJsonValueObject>(OptionObject));
    }

    RootObject->SetArrayField(TEXT("options"), OptionsArray);

    return RootObject;
}

auto
    FCk_EQSExporter::
    DoSerializeOption_Json(
        const UEnvQueryOption* InOption,
        int32 InOptionIndex)
    -> TSharedPtr<FJsonObject>
{
    auto OptionObject = MakeShared<FJsonObject>();

    if (!IsValid(InOption))
    { return OptionObject; }

    OptionObject->SetNumberField(TEXT("optionIndex"), InOptionIndex);

    // Generator
    UEnvQueryGenerator* Generator = InOption->Generator;
    if (IsValid(Generator))
    {
        OptionObject->SetObjectField(TEXT("generator"), DoSerializeGenerator_Json(Generator));
    }

    // Tests
    auto TestsArray = TArray<TSharedPtr<FJsonValue>>{};

    for (int32 TestIdx = 0; TestIdx < InOption->Tests.Num(); ++TestIdx)
    {
        UEnvQueryTest* Test = InOption->Tests[TestIdx];
        if (!IsValid(Test))
        { continue; }

        TestsArray.Add(MakeShared<FJsonValueObject>(DoSerializeTest_Json(Test)));
    }

    OptionObject->SetArrayField(TEXT("tests"), TestsArray);

    return OptionObject;
}

auto
    FCk_EQSExporter::
    DoSerializeGenerator_Json(
        const UEnvQueryGenerator* InGenerator)
    -> TSharedPtr<FJsonObject>
{
    auto GeneratorObject = MakeShared<FJsonObject>();

    if (!IsValid(InGenerator))
    { return GeneratorObject; }

    auto ClassName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InGenerator, ClassName, Description);

    GeneratorObject->SetStringField(TEXT("className"), ClassName);

    if (!Description.IsEmpty())
    {
        GeneratorObject->SetStringField(TEXT("description"), Description);
    }

    GeneratorObject->SetObjectField(TEXT("properties"), DoSerializeNodeProperties_Json(InGenerator));

    return GeneratorObject;
}

auto
    FCk_EQSExporter::
    DoSerializeTest_Json(
        const UEnvQueryTest* InTest)
    -> TSharedPtr<FJsonObject>
{
    auto TestObject = MakeShared<FJsonObject>();

    if (!IsValid(InTest))
    { return TestObject; }

    auto ClassName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InTest, ClassName, Description);

    TestObject->SetStringField(TEXT("className"), ClassName);

    if (!Description.IsEmpty())
    {
        TestObject->SetStringField(TEXT("description"), Description);
    }

    TestObject->SetObjectField(TEXT("properties"), DoSerializeNodeProperties_Json(InTest));

    return TestObject;
}

auto
    FCk_EQSExporter::
    DoSerializeNodeProperties_Json(
        const UObject* InNode)
    -> TSharedPtr<FJsonObject>
{
    auto PropertiesObject = MakeShared<FJsonObject>();

    if (!IsValid(InNode))
    { return PropertiesObject; }

    for (TFieldIterator<FProperty> Itr(InNode->GetClass()); Itr; ++Itr)
    {
        const auto* Property = *Itr;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        const auto& PropertyName = Property->GetName();
        const auto& PropertyValue = DoGetPropertyValueAsString(Property, InNode);

        PropertiesObject->SetStringField(PropertyName, PropertyValue);
    }

    return PropertiesObject;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-Text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_EQSExporter::
    DoSerializeToText(
        const UEnvQuery* InQuery)
    -> FString
{
    auto Text = FString{};

    Text += FString::Printf(TEXT("=== EQS Query: %s ===\n"), *InQuery->GetName());
    Text += FString::Printf(TEXT("Path: %s\n"), *InQuery->GetPathName());
    Text += FString::Printf(TEXT("Exported: %s\n"), *FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")));
    Text += TEXT("\n");

    const auto Options = DoGetQueryOptions(InQuery);
    for (int32 OptionIdx = 0; OptionIdx < Options.Num(); ++OptionIdx)
    {
        const auto* Option = Options[OptionIdx];
        if (!IsValid(Option))
        { continue; }

        DoSerializeOption_Text(Option, Text, OptionIdx);
    }

    return Text;
}

auto
    FCk_EQSExporter::
    DoSerializeOption_Text(
        const UEnvQueryOption* InOption,
        FString& OutText,
        int32 InOptionIndex)
    -> void
{
    if (!IsValid(InOption))
    { return; }

    OutText += FString::Printf(TEXT("--- Option %d ---\n"), InOptionIndex);

    // Generator
    UEnvQueryGenerator* Generator = InOption->Generator;
    if (IsValid(Generator))
    {
        DoSerializeGenerator_Text(Generator, OutText, 1);
    }

    // Tests
    for (int32 TestIdx = 0; TestIdx < InOption->Tests.Num(); ++TestIdx)
    {
        UEnvQueryTest* Test = InOption->Tests[TestIdx];
        if (!IsValid(Test))
        { continue; }

        DoSerializeTest_Text(Test, OutText, 1);
    }

    OutText += TEXT("\n");
}

auto
    FCk_EQSExporter::
    DoSerializeGenerator_Text(
        const UEnvQueryGenerator* InGenerator,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InGenerator))
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    auto ClassName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InGenerator, ClassName, Description);

    if (!Description.IsEmpty())
    {
        OutText += FString::Printf(TEXT("%s[Generator] %s: \"%s\"\n"), *Indent, *ClassName, *Description);
    }
    else
    {
        OutText += FString::Printf(TEXT("%s[Generator] %s\n"), *Indent, *ClassName);
    }

    DoSerializeNodeProperties_Text(InGenerator, OutText, InDepth + 1);
}

auto
    FCk_EQSExporter::
    DoSerializeTest_Text(
        const UEnvQueryTest* InTest,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InTest))
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    auto ClassName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InTest, ClassName, Description);

    if (!Description.IsEmpty())
    {
        OutText += FString::Printf(TEXT("%s[Test] %s: \"%s\"\n"), *Indent, *ClassName, *Description);
    }
    else
    {
        OutText += FString::Printf(TEXT("%s[Test] %s\n"), *Indent, *ClassName);
    }

    DoSerializeNodeProperties_Text(InTest, OutText, InDepth + 1);
}

auto
    FCk_EQSExporter::
    DoSerializeNodeProperties_Text(
        const UObject* InNode,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InNode))
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    auto bHasProperties = false;

    for (TFieldIterator<FProperty> Itr(InNode->GetClass()); Itr; ++Itr)
    {
        const auto* Property = *Itr;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        if (!bHasProperties)
        {
            OutText += FString::Printf(TEXT("%sProperties:\n"), *Indent);
            bHasProperties = true;
        }

        const auto& PropertyName = Property->GetName();
        const auto& PropertyValue = DoGetPropertyValueAsString(Property, InNode);

        OutText += FString::Printf(TEXT("%s  %s = %s\n"), *Indent, *PropertyName, *PropertyValue);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_EQSExporter::
    DoResolveOutputPath(
        const UEnvQuery* InQuery,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InQuery->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    // The conversion gives us the path without extension, append our extension
    DiskPath += InExtension;

    return DiskPath;
}

auto
    FCk_EQSExporter::
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer)
    -> FString
{
    auto Value = FString{};
    const auto* ValuePtr = InProperty->ContainerPtrToValuePtr<void>(InContainer);
    InProperty->ExportTextItem_Direct(Value, ValuePtr, nullptr, nullptr, PPF_None);

    // If a struct property exported as "()", expand its sub-fields individually
    if (Value == TEXT("()"))
    {
        const auto* StructProperty = CastField<FStructProperty>(InProperty);
        if (StructProperty != nullptr)
        {
            auto ExpandedParts = TArray<FString>{};

            for (TFieldIterator<FProperty> SubItr(StructProperty->Struct); SubItr; ++SubItr)
            {
                const auto* SubProperty = *SubItr;
                auto SubValue = FString{};
                const auto* SubValuePtr = SubProperty->ContainerPtrToValuePtr<void>(ValuePtr);
                SubProperty->ExportTextItem_Direct(SubValue, SubValuePtr, nullptr, nullptr, PPF_None);
                ExpandedParts.Add(FString::Printf(TEXT("%s=%s"), *SubProperty->GetName(), *SubValue));
            }

            Value = FString::Printf(TEXT("(%s)"), *FString::Join(ExpandedParts, TEXT(", ")));
        }
    }

    return Value;
}

auto
    FCk_EQSExporter::
    DoShouldIncludeProperty(
        const FProperty* InProperty)
    -> bool
{
    if (InProperty == nullptr)
    { return false; }

    // Skip transient and deprecated properties
    if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
    { return false; }

    // Skip properties from base engine classes (UObject, UEnvQueryNode)
    const auto* OwnerClass = InProperty->GetOwnerClass();
    if (OwnerClass == nullptr)
    { return false; }

    static const TSet<FName> SkipClasses = {
        TEXT("Object"),
        TEXT("EnvQueryNode"),
    };

    if (SkipClasses.Contains(OwnerClass->GetFName()))
    { return false; }

    // Include if the property is EditAnywhere, VisibleAnywhere, or BlueprintVisible
    if (InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
    { return true; }

    return false;
}

auto
    FCk_EQSExporter::
    DoGetIndent(
        int32 InDepth)
    -> FString
{
    auto Indent = FString{};
    for (int32 i = 0; i < InDepth; ++i)
    {
        Indent += TEXT("    ");
    }
    return Indent;
}

auto
    FCk_EQSExporter::
    DoGetNodeDisplayInfo(
        const UObject* InNode,
        FString& OutClassName,
        FString& OutDescription)
    -> void
{
    if (!IsValid(InNode))
    {
        OutClassName = TEXT("Unknown");
        OutDescription = FString{};
        return;
    }

    OutClassName = InNode->GetClass()->GetName();

    // UEnvQueryGenerator and UEnvQueryTest both have GetDescriptionTitle()
    // We use the option description if available
    const auto* Generator = Cast<UEnvQueryGenerator>(InNode);
    const auto* Test = Cast<UEnvQueryTest>(InNode);

    if (Generator != nullptr)
    {
        OutDescription = Generator->GetDescriptionTitle().ToString();
    }
    else if (Test != nullptr)
    {
        OutDescription = Test->GetDescriptionTitle().ToString();
    }
    else
    {
        OutDescription = FString{};
    }

    // Clean up description - remove line terminators for single-line display
    OutDescription.ReplaceInline(LINE_TERMINATOR, TEXT(" | "));
}

// --------------------------------------------------------------------------------------------------------------------
