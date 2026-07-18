#pragma once

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UObject;

// --------------------------------------------------------------------------------------------------------------------
// Per-exporter output-format versions. Bump the relevant constant whenever an exporter's serialized shape changes so
// the freshness gate (Get_IsExportFresh) re-exports assets whose sibling was written by an older exporter. The value
// is embedded in each export's "_meta" object; a version mismatch forces a re-export even when the source is unchanged.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::version
{
    inline constexpr int32 DataAsset = 1;
    inline constexpr int32 DataTable = 1;
    inline constexpr int32 Blueprint = 2;   // v2: WBP paste artifacts (hierarchy.copy.txt + animation t3d dumps)
    inline constexpr int32 BehaviorTree = 1;
    inline constexpr int32 EQS = 1;
    inline constexpr int32 StateTree = 1;
    inline constexpr int32 UserDefinedEnum = 1;
    inline constexpr int32 UserDefinedStruct = 1;
}

// --------------------------------------------------------------------------------------------------------------------
// Deterministic export metadata: a content hash of the source asset file plus the exporter version. Emitted as the
// "_meta" JSON object on every sibling-writing exporter. Deliberately carries NO timestamp and NO machine path so two
// exports of identical input are byte-identical — that determinism is what lets the freshness gate and any external
// diff tooling trust the output.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExportMeta
{
public:
    // Absolute on-disk path of the asset's source package (.uasset, else .umap). Empty when neither exists on disk
    // (unsaved / transient / cooked-only object).
    static auto
    Get_SourcePackageFilename(
        const UObject* InAsset) -> FString;

    // MD5 of the source package file (LexToString hex). Empty when there is no source file.
    static auto
    Get_SourceHash(
        const UObject* InAsset) -> FString;

    // { "sourceHash": <string>, "exporterVersion": <number> } — the "_meta" object embedded in each export.
    static auto
    MakeMetaObject(
        const UObject* InAsset,
        int32 InExporterVersion) -> TSharedPtr<FJsonObject>;

    // Reads an existing export json and extracts its "_meta" fields. False on any absence/malformation (missing file,
    // unparseable json, missing "_meta", missing fields).
    static auto
    TryRead_SiblingMeta(
        const FString& InJsonFilePath,
        FString& OutSourceHash,
        int32& OutExporterVersion) -> bool;

    // True iff the sibling json exists, its "_meta" parses, its stored hash equals the current source hash, and its
    // stored version equals the current exporter version. Any other state (including no source file) is "not fresh".
    static auto
    Get_IsExportFresh(
        const UObject* InAsset,
        const FString& InSiblingJsonPath,
        int32 InCurrentExporterVersion) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
