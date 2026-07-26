#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// External linkage, NOT public API: the graph dumper's TU reuses the dispatch TU's one definition.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_dispatch
{
    // Best-effort, never loads. Prefers .uasset, then .umap; empty when the package can't be resolved.
    auto
    Get_PackageDiskPath(
        const FString& InPackageName) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
