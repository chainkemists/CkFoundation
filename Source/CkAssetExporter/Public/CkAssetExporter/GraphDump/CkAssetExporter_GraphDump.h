#pragma once

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Asset-registry dependency-graph dumper. Enumerates EVERY asset under a package dir (no class filter — migration
// closure planning must see art, redirectors, and everything else), records each asset's hard/soft PACKAGE
// dependencies (including deps outside the root — exactly what closure planning needs), and flags cascade
// (UParticleSystem-derived) and redirector assets. Deterministic: no timestamps, assets sorted by object path, dep
// arrays sorted lexically. Loads NOTHING — class-hierarchy checks resolve the registry-reported class, not the asset.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_GraphDump
{
public:
    // Enumerates InPackageDir recursively and returns { "root", "count", "assets": [ { assetPath, class, diskPath,
    // hardDeps[], softDeps[], flags[]? } ] }. Invalid TSharedPtr on a registry enumeration failure (the ONLY error
    // path — an empty dir is a valid count-0 graph, not a failure).
    static auto
    DumpGraph(
        const FString& InPackageDir) -> TSharedPtr<FJsonObject>;

    // Serializes InGraph to <ProjectSavedDir>/<InProjectSavedSubdir>/graph.json (absolute) and returns that path.
    // Empty on an invalid graph.
    static auto
    WriteGraph(
        const TSharedPtr<FJsonObject>& InGraph,
        const FString& InProjectSavedSubdir) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
