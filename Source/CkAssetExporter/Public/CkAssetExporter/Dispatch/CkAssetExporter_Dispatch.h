#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

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

class CKASSETEXPORTER_API FCk_AssetExporter_Dispatch
{
public:
    // ONE token: object path (/Game/X/Y.Y), package path (/Game/X/Y), or a Content disk path. Empty array + a
    // module-log line on any unrecognized/unconvertible input.
    static auto
    Resolve_InputToObjectPaths(
        const FString& InRaw) -> TArray<FString>;

    // An unresolvable token becomes a FAILURE ROW on InOutSummary — a request naming garbage must exit nonzero.
    static auto
    Resolve_TokensToObjectPaths(
        const TArray<FString>& InTokens,
        FCk_AssetExportDispatchSummary& InOutSummary) -> TArray<FString>;

    // GC every 100 loads. A load failure is a failure row, never an abort.
    static auto
    ExportAssets(
        const TArray<FString>& InObjectPaths,
        bool InSkipFresh,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonAndText) -> FCk_AssetExportDispatchSummary;

    // Recursive. Empty InClassFilters = every supported type; an unknown friendly name = summary-level failure.
    static auto
    SweepDirectory(
        const FString& InPackageDir,
        const TArray<FString>& InClassFilters,
        bool InSkipFresh,
        ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonAndText) -> FCk_AssetExportDispatchSummary;

    // Same filter as SweepDirectory but no loading. A bad filter name returns "ok": false + "error".
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

    // Case-insensitive. nullptr + OutError (listing the valid names) on an unknown name.
    static auto
    TryResolve_FriendlyClassName(
        const FString& InName,
        FString& OutError) -> UClass*;

    // The structured sidecar (<Asset>.ckexport) next to the source .uasset. Empty when the package can't be
    // resolved to a filename.
    static auto
    Get_SiblingSidecarPathForAsset(
        const UObject* InAsset) -> FString;

    // The on-save sidecar hook's cheap pre-filter — art saves must not spam export attempts.
    static auto
    Get_IsExportableClass(
        const UClass* InClass) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
