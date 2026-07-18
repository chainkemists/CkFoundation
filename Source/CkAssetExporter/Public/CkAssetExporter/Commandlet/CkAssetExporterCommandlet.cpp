#include "CkAssetExporterCommandlet.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/Dispatch/CkAssetExporter_Dispatch.h"
#include "CkAssetExporter/GraphDump/CkAssetExporter_GraphDump.h"
#include "CkAssetExporter/Server/CkAssetExporter_RequestLoop.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Dom/JsonObject.h>
#include <Misc/FileHelper.h>
#include <Misc/Parse.h>
#include <Misc/Paths.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_commandlet
{
    constexpr auto SavedSubdir = TEXT("CkAssetExporter");

    static auto
    Serialize_Json(
        const TSharedPtr<FJsonObject>& InObject)
        -> FString
    {
        auto JsonString = FString{};
        const auto Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(InObject.ToSharedRef(), Writer);
        return JsonString;
    }

    static auto
    Parse_Classes(
        const FString& InParams,
        TArray<FString>& OutClasses)
        -> void
    {
        auto ClassesToken = FString{};
        if (FParse::Value(*InParams, TEXT("Classes="), ClassesToken) && NOT ClassesToken.IsEmpty())
        {
            constexpr auto CullEmpty = true;
            ClassesToken.ParseIntoArray(OutClasses, TEXT(","), CullEmpty);
            for (auto& Class : OutClasses)
            { Class.TrimStartAndEndInline(); }
        }
    }

    static auto
    Merge_Summary(
        FCk_AssetExportDispatchSummary& InOutSummary,
        const FCk_AssetExportDispatchSummary& InSummary)
        -> void
    {
        InOutSummary.Succeeded = InOutSummary.Succeeded && InSummary.Succeeded;
        if (NOT InSummary.ErrorMessage.IsEmpty() && InOutSummary.ErrorMessage.IsEmpty())
        { InOutSummary.ErrorMessage = InSummary.ErrorMessage; }
        InOutSummary.Exported += InSummary.Exported;
        InOutSummary.Skipped  += InSummary.Skipped;
        InOutSummary.Failed   += InSummary.Failed;
        InOutSummary.Entries.Append(InSummary.Entries);
    }

    static auto
    Log_Summary(
        const FCk_AssetExportDispatchSummary& InSummary,
        const FString& InManifestPath)
        -> void
    {
        for (const auto& Entry : InSummary.Entries)
        {
            if (Entry.Skipped)
            { ck::asset_exporter::Display(TEXT("[Commandlet]   SKIP  [{}] (fresh)"), Entry.AssetPath); }
            else if (Entry.Succeeded)
            { ck::asset_exporter::Display(TEXT("[Commandlet]   OK    [{}] -> {}"), Entry.AssetPath, Entry.JsonFilePath); }
            else
            { ck::asset_exporter::Warning(TEXT("[Commandlet]   FAIL  [{}]: {}"), Entry.AssetPath, Entry.ErrorMessage); }
        }

        ck::asset_exporter::Display(
            TEXT("[Commandlet] Done: {} exported, {} skipped, {} failed (manifest: {})"),
            InSummary.Exported, InSummary.Skipped, InSummary.Failed, InManifestPath);
    }

    static auto
    Print_Usage()
        -> void
    {
        ck::asset_exporter::Display(TEXT("[Commandlet] CkAssetExporter — headless asset export"));
        ck::asset_exporter::Display(TEXT("  -Assets=<a;b;c>   Export specific assets. Each token is an object path (/Game/X/Y.Y),"));
        ck::asset_exporter::Display(TEXT("                    a package path (/Game/X/Y), or a Content disk path. Semicolon-separated."));
        ck::asset_exporter::Display(TEXT("  -Dir=<packagePath> Sweep a package dir recursively and export matching assets."));
        ck::asset_exporter::Display(TEXT("  -Classes=<a,b>    (with -Dir) Restrict to friendly class names. Valid: DataAsset, DataTable,"));
        ck::asset_exporter::Display(TEXT("                    Blueprint, BehaviorTree, EQS, StateTree, Enum, Struct, Niagara, Cascade. Empty = all."));
        ck::asset_exporter::Display(TEXT("  -DumpGraph        Dump the dependency graph (every class) under -Dir (default /Game) to graph.json."));
        ck::asset_exporter::Display(TEXT("                    Composes with nothing else — -Assets/-List are ignored when present."));
        ck::asset_exporter::Display(TEXT("  -List             (with -Dir) List matching assets to LastList.json + console; does not export."));
        ck::asset_exporter::Display(TEXT("  -SkipFresh        Skip assets whose sibling json is already up to date (7 versioned types only)."));
        ck::asset_exporter::Display(TEXT("  -Force            Export even fresh assets (overrides -SkipFresh)."));
        ck::asset_exporter::Display(TEXT("  -Out=<dir>        NOT supported in v1 — exporters always write sibling paths. Logged and ignored."));
        ck::asset_exporter::Display(TEXT("  -ExportServer     Run the request-file server loop (Saved/CkAssetExporter/Requests|Results)."));
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCkAssetExporterCommandlet::UCkAssetExporterCommandlet()
{
    // Loading assets + walking reflection needs an editor-capable context so every contributing UClass is registered
    // (mirrors CkAngelscriptGenerator_DriftCommandlet). IsEditor=true makes UCommandletHelpers spin up GEditor.
    IsClient       = false;
    IsServer       = false;
    IsEditor       = true;
    LogToConsole   = true;
    ShowErrorCount = true;
    // Without this, LaunchEngineLoop promotes ANY logged Error to process exit 1 (LaunchEngineLoop.cpp:4160-4167) —
    // and this project's editor boot always logs pre-existing plugin errors (e.g. AdvancedCommenting CDO textures),
    // which would make the exit code permanently red regardless of export outcome. Main()'s manifest-backed return
    // IS the verdict.
    UseCommandletResultAsExitCode = true;
}

// --------------------------------------------------------------------------------------------------------------------

int32 UCkAssetExporterCommandlet::Main(const FString& InParams)
{
    using namespace ck_asset_exporter_commandlet;

    // NOT "-Server": that token is a reserved engine switch (dedicated-server mode) — passing it hijacks the launch
    // into a ticking headless session and the commandlet never runs.
    if (FParse::Param(*InParams, TEXT("ExportServer")))
    { return FCk_AssetExporter_RequestLoop::Run(); }

    auto AssetsToken = FString{};
    const auto HasAssets = FParse::Value(*InParams, TEXT("Assets="), AssetsToken) && NOT AssetsToken.IsEmpty();

    auto Dir = FString{};
    const auto HasDir = FParse::Value(*InParams, TEXT("Dir="), Dir) && NOT Dir.IsEmpty();

    const auto List = FParse::Param(*InParams, TEXT("List"));

    // ---- Graph-dump mode (composes with nothing else; defaults to /Game when no -Dir) ----
    if (FParse::Param(*InParams, TEXT("DumpGraph")))
    {
        if (HasAssets || List)
        { ck::asset_exporter::Display(TEXT("[Commandlet] -DumpGraph ignores -Assets/-List (graph mode composes with nothing else)")); }

        const auto GraphDir = HasDir ? Dir : FString{TEXT("/Game")};

        const auto Graph = FCk_AssetExporter_GraphDump::DumpGraph(GraphDir);
        if (NOT Graph.IsValid())
        {
            ck::asset_exporter::Error(TEXT("[Commandlet] Graph enumeration failed for [{}]"), GraphDir);
            return 1;
        }

        const auto GraphPath = FCk_AssetExporter_GraphDump::WriteGraph(Graph, SavedSubdir);

        auto Count = double{0.0};
        Graph->TryGetNumberField(TEXT("count"), Count);
        ck::asset_exporter::Display(
            TEXT("[Commandlet] Graph written to [{}] ({} assets)"), GraphPath, static_cast<int32>(Count));
        return 0;
    }

    auto OutDir = FString{};
    if (FParse::Value(*InParams, TEXT("Out="), OutDir) && NOT OutDir.IsEmpty())
    {
        ck::asset_exporter::Warning(
            TEXT("[Commandlet] -Out=[{}] is not yet supported in v1 dispatch — exporting to sibling paths instead"), OutDir);
    }

    const auto Force = FParse::Param(*InParams, TEXT("Force"));
    const auto SkipFresh = FParse::Param(*InParams, TEXT("SkipFresh")) && NOT Force;

    if (NOT HasAssets && NOT HasDir)
    {
        Print_Usage();
        return 1;
    }

    // ---- List mode (requires -Dir) ----
    if (List)
    {
        if (NOT HasDir)
        {
            ck::asset_exporter::Error(TEXT("[Commandlet] -List requires -Dir=<packagePath>"));
            Print_Usage();
            return 1;
        }

        auto ClassFilters = TArray<FString>{};
        Parse_Classes(InParams, ClassFilters);

        const auto ListJson = FCk_AssetExporter_Dispatch::ListAssets(Dir, ClassFilters);
        const auto JsonString = Serialize_Json(ListJson);

        const auto ListPath = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(FPaths::ProjectSavedDir(), SavedSubdir, TEXT("LastList.json")));
        FFileHelper::SaveStringToFile(JsonString, *ListPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

        ck::asset_exporter::Display(TEXT("[Commandlet] List written to [{}]:\n{}"), ListPath, JsonString);

        auto Ok = false;
        ListJson->TryGetBoolField(TEXT("ok"), Ok);
        return Ok ? 0 : 1;
    }

    // ---- Export mode ----
    auto Summary = FCk_AssetExportDispatchSummary{};
    Summary.Succeeded = true;

    if (HasAssets)
    {
        auto Tokens = TArray<FString>{};
        constexpr auto CullEmpty = true;
        AssetsToken.ParseIntoArray(Tokens, TEXT(";"), CullEmpty);

        const auto ObjectPaths = FCk_AssetExporter_Dispatch::Resolve_TokensToObjectPaths(Tokens, Summary);
        Merge_Summary(Summary, FCk_AssetExporter_Dispatch::ExportAssets(ObjectPaths, SkipFresh));
    }

    if (HasDir)
    {
        auto ClassFilters = TArray<FString>{};
        Parse_Classes(InParams, ClassFilters);
        Merge_Summary(Summary, FCk_AssetExporter_Dispatch::SweepDirectory(Dir, ClassFilters, SkipFresh));
    }

    const auto ManifestPath = FCk_AssetExporter_Dispatch::WriteManifest(Summary, SavedSubdir);
    Log_Summary(Summary, ManifestPath);

    return (Summary.Succeeded && Summary.Failed == 0) ? 0 : 1;
}

// --------------------------------------------------------------------------------------------------------------------
