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
    inline constexpr int32 DataAsset = 2;
    inline constexpr int32 DataTable = 2;
    inline constexpr int32 Blueprint = 3;
    inline constexpr int32 BehaviorTree = 2;
    inline constexpr int32 EQS = 2;
    inline constexpr int32 StateTree = 2;
    inline constexpr int32 UserDefinedEnum = 2;
    inline constexpr int32 UserDefinedStruct = 2;
    // 3 adds "graph": the expression nodes with their connectivity, pin masks and non-default properties, plus
    // which node feeds each material output. Through 2 the export carried only a CLASS HISTOGRAM of the graph,
    // which says a material has 29 Multiplies but never which 29 things it multiplies — enough to compare two
    // materials, never enough to reconstruct one.
    inline constexpr int32 Material = 3;

    // MaterialFunction exports the same "graph" shape as Material 3, minus the parameter/blend fields a
    // function does not have. First stamped at 1 — there is no older shape to be compatible with.
    inline constexpr int32 MaterialFunction = 1;

    // First stamped at 3, the shape that added systemState / eventHandlers / lifetimeResolved: earlier Niagara
    // sidecars carried no "_meta" at all, so a consumer cannot tell 1 from 2 and must not be told it can.
    inline constexpr int32 Niagara = 3;
}

// --------------------------------------------------------------------------------------------------------------------
// Sidecar file extensions, written next to the source .uasset under Content/.
//
// The structured sidecar deliberately does NOT end in ".json". UE's auto-reimport monitor applies
// SetApplicableExtensions(GetAllFactoryExtensions()) to every watched content directory, and json IS a registered
// import format (UReimportDataTableFactory). A watched sidecar corpus accumulates into the monitor's pending set,
// parks its state machine on the "N changes to source content files" prompt, and re-stats the entire outstanding set
// EVERY FRAME while parked -- measured at 18 fps on a 1,178-file corpus. FMatchRules::IsFileApplicable gates on the
// extension BEFORE any wildcard rule, so an extension no factory claims is invisible to the monitor in every project
// with zero configuration. The gate reads the FINAL extension only (MatchExtensionString -> Strrchr): "Foo.x.json"
// still parses as json, so this must remain a terminal extension.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::extension
{
    // The structured, lossless export. Plain UTF-8 JSON content despite the extension.
    inline const FString Sidecar = TEXT(".ckexport");

    // The human-readable companion summary. Lossy; manual-export path only.
    inline const FString SummaryText = TEXT(".txt");

    // DataTable row dump.
    inline const FString Csv = TEXT(".csv");
}

// --------------------------------------------------------------------------------------------------------------------
// Which sibling files an export call writes. The on-save path is JsonOnly: the summary text is lossy, gitignored in
// consuming projects, and therefore never shared -- refreshing it on every save costs I/O for a file nobody reads,
// while the committed structured sidecar is the artifact that must stay truthful.
// --------------------------------------------------------------------------------------------------------------------

enum class ECk_AssetExporter_SidecarFormats
{
    JsonOnly,
    JsonAndText
};

namespace ck::asset_exporter
{
    // The single gate every exporter consults before writing its lossy .txt summary. Centralized so the JsonOnly
    // auto path cannot regress in one exporter silently -- Ck.AssetExporter.Dispatch.SummaryTextFormatGate pins it.
    constexpr auto ShouldWrite_SummaryText(ECk_AssetExporter_SidecarFormats InFormats) -> bool
    {
        return InFormats == ECk_AssetExporter_SidecarFormats::JsonAndText;
    }
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
