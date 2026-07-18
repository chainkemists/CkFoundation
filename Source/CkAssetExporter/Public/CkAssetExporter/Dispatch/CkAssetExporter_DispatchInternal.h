#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Same-module internal helpers shared between the dispatch layer and the graph dumper. NOT part of the public
// FCk_AssetExporter_Dispatch API surface — external linkage only so a second TU in this module can reuse the one
// definition instead of duplicating it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_dispatch
{
    // Best-effort on-disk file path for a package name (list/graph display only — never loads). Prefers the .uasset,
    // then .umap; falls back to the .uasset guess when neither exists. Empty when the package can't be resolved.
    auto
    Get_PackageDiskPath(
        const FString& InPackageName) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
