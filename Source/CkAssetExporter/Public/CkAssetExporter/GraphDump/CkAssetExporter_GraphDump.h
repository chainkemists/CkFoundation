#pragma once

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Loads NOTHING — class-hierarchy checks resolve the registry-reported class, not the asset.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_GraphDump
{
public:
    // Recursive. Returns { "root", "count", "assets": [ { assetPath, class, diskPath, hardDeps[], softDeps[],
    // flags[]? } ] }. Invalid TSharedPtr ONLY on registry enumeration failure — an empty dir is a valid count-0 graph.
    static auto
    DumpGraph(
        const FString& InPackageDir) -> TSharedPtr<FJsonObject>;

    // Returns the absolute graph.json path written; empty on an invalid graph.
    static auto
    WriteGraph(
        const TSharedPtr<FJsonObject>& InGraph,
        const FString& InProjectSavedSubdir) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
