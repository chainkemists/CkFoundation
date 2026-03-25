#pragma once

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UEnvQuery;
class UEnvQueryOption;
class UEnvQueryGenerator;
class UEnvQueryTest;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_EQSExportResult
{
    bool bSuccess = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_EQSExporter
{
public:
    static auto
    ExportEQS(
        UEnvQuery* InQuery) -> FCk_EQSExportResult;

    static auto
    ExportEQSQueries(
        const TArray<UEnvQuery*>& InQueries) -> TArray<FCk_EQSExportResult>;

private:
    // ---- JSON serialization ----

    static auto
    DoSerializeToJson(
        const UEnvQuery* InQuery) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeOption_Json(
        const UEnvQueryOption* InOption,
        int32 InOptionIndex) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeGenerator_Json(
        const UEnvQueryGenerator* InGenerator) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeTest_Json(
        const UEnvQueryTest* InTest) -> TSharedPtr<FJsonObject>;

    static auto
    DoSerializeNodeProperties_Json(
        const UObject* InNode) -> TSharedPtr<FJsonObject>;

    // ---- Plain-text serialization ----

    static auto
    DoSerializeToText(
        const UEnvQuery* InQuery) -> FString;

    static auto
    DoSerializeOption_Text(
        const UEnvQueryOption* InOption,
        FString& OutText,
        int32 InOptionIndex) -> void;

    static auto
    DoSerializeGenerator_Text(
        const UEnvQueryGenerator* InGenerator,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeTest_Text(
        const UEnvQueryTest* InTest,
        FString& OutText,
        int32 InDepth) -> void;

    static auto
    DoSerializeNodeProperties_Text(
        const UObject* InNode,
        FString& OutText,
        int32 InDepth) -> void;

    // ---- Helpers ----

    static auto
    DoResolveOutputPath(
        const UEnvQuery* InQuery,
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
    DoGetNodeDisplayInfo(
        const UObject* InNode,
        FString& OutClassName,
        FString& OutDescription) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
