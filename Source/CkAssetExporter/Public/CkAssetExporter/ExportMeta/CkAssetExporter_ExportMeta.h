#pragma once

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UObject;

// --------------------------------------------------------------------------------------------------------------------
// Bump a constant whenever that exporter's serialized shape changes: Get_IsExportFresh re-exports on mismatch.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::version
{
    inline constexpr int32 DataAsset = 1;
    inline constexpr int32 DataTable = 1;
    inline constexpr int32 Blueprint = 2;
    inline constexpr int32 BehaviorTree = 1;
    inline constexpr int32 EQS = 1;
    inline constexpr int32 StateTree = 1;
    inline constexpr int32 UserDefinedEnum = 1;
    inline constexpr int32 UserDefinedStruct = 1;
    inline constexpr int32 Material = 1;
}

// --------------------------------------------------------------------------------------------------------------------
// The "_meta" object every sibling-writing exporter embeds. Deliberately NO timestamp and NO machine path: two
// exports of identical input must be byte-identical, or the freshness gate and external diff tooling cannot trust it.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExportMeta
{
public:
    // Absolute .uasset, else .umap. Empty for an unsaved / transient / cooked-only object.
    static auto
    Get_SourcePackageFilename(
        const UObject* InAsset) -> FString;

    // MD5 of the source package file (LexToString hex). Empty when there is no source file.
    static auto
    Get_SourceHash(
        const UObject* InAsset) -> FString;

    // { "sourceHash": <string>, "exporterVersion": <number> }
    static auto
    MakeMetaObject(
        const UObject* InAsset,
        int32 InExporterVersion) -> TSharedPtr<FJsonObject>;

    // False on any absence/malformation: missing file, unparseable json, missing "_meta", missing fields.
    static auto
    TryRead_SiblingMeta(
        const FString& InJsonFilePath,
        FString& OutSourceHash,
        int32& OutExporterVersion) -> bool;

    // Fresh iff the sibling json parses AND both its hash and its version match. Anything else is not fresh.
    static auto
    Get_IsExportFresh(
        const UObject* InAsset,
        const FString& InSiblingJsonPath,
        int32 InCurrentExporterVersion) -> bool;

    // The txt siblings carry no "_meta" of their own; this banner points readers at the json's, written in the
    // same export call.
    static auto
    Get_SummaryTextBanner(
        const FString& InAssetName) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
