#pragma once

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UClass;
class UObject;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_AssetExportDispatchEntryResult
{
    FString AssetPath;
    FString AssetClass;
    bool    Succeeded = false;
    bool    Skipped = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_AssetExportDispatchSummary
{
    bool    Succeeded = false;
    FString ErrorMessage;
    int32   Exported = 0;
    int32   Skipped = 0;
    int32   Failed = 0;
    TArray<FCk_AssetExportDispatchEntryResult> Entries;
};

// --------------------------------------------------------------------------------------------------------------------
// Headless routing layer over the per-type exporters. Resolves input tokens to object paths, classifies each asset to
// its most-specific exporter (mirroring CkDataAssetExporter_AssetAction's dedicated-exporter exclusion order), and
// records per-asset success/skip/failure rows (a load failure is a ROW, never an abort — VfxCorpus precedent). Only
// the seven versioned exporters participate in the skip-fresh gate; Niagara/Cascade always re-export.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_Dispatch
{
public:
    // Accepts ONE input token — an object path (/Game/X/Y.Y), a package path (/Game/X/Y), or a Windows disk path
    // under any Content dir (backslashes normalized, converted via FPackageName). A package path becomes an object
    // path by appending ".<AssetName>". Empty array (+ module-log line) on any unrecognized/unconvertible input.
    static auto
    Resolve_InputToObjectPaths(
        const FString& InRaw) -> TArray<FString>;

    // Resolves a batch of input tokens; an unresolvable token becomes a FAILURE ROW on InOutSummary instead of
    // silently vanishing from the export set (a request that names garbage must exit nonzero, not green).
    static auto
    Resolve_TokensToObjectPaths(
        const TArray<FString>& InTokens,
        FCk_AssetExportDispatchSummary& InOutSummary) -> TArray<FString>;

    // Loads each object path, routes to its exporter, and returns per-asset rows. GC every 100 loads. Load failures
    // are failure rows, not aborts.
    static auto
    ExportAssets(
        const TArray<FString>& InObjectPaths,
        bool InSkipFresh) -> FCk_AssetExportDispatchSummary;

    // Asset-registry sweep of a package dir (recursive). Empty InClassFilters = every supported type. Unknown friendly
    // filter name = summary-level failure listing the valid names. Routes each hit through the same per-asset path.
    static auto
    SweepDirectory(
        const FString& InPackageDir,
        const TArray<FString>& InClassFilters,
        bool InSkipFresh) -> FCk_AssetExportDispatchSummary;

    // Same filter as SweepDirectory but no loading — rows of { "assetPath", "class", "diskPath" }. On a bad filter
    // name the returned object carries "ok": false + "error".
    static auto
    ListAssets(
        const FString& InPackageDir,
        const TArray<FString>& InClassFilters) -> TSharedPtr<FJsonObject>;

    // Writes <ProjectSavedDir>/<InProjectSavedSubdir>/LastRun.json (timestamp-free). Returns the path written.
    static auto
    WriteManifest(
        const FCk_AssetExportDispatchSummary& InSummary,
        const FString& InProjectSavedSubdir) -> FString;

public:
    // ---- Exposed for tests / reuse ----

    // Friendly filter name (case-insensitive) -> UClass. nullptr + OutError (listing valid names) on an unknown name.
    static auto
    TryResolve_FriendlyClassName(
        const FString& InName,
        FString& OutError) -> UClass*;

    // The .json path an exporter would write next to InAsset (the TryConvertLongPackageNameToFilename + ".json" idiom
    // the exporters use). Empty when the package can't be resolved to a filename.
    static auto
    Get_SiblingJsonPathForAsset(
        const UObject* InAsset) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
