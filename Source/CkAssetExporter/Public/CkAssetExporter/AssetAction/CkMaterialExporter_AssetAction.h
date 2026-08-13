#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{
    // Adds an "Export Material to JSON" entry to the Content Browser asset context menu (materials + instances).
    auto RegisterMaterialContextMenu() -> void;

    // The same, for UMaterialFunction. It needs its own registration because the material entry is a DYNAMIC
    // entry gated on the selection containing a UMaterialInterface — and a material function is not one, so a
    // selected function would otherwise show no Ck export entry at all.
    auto RegisterMaterialFunctionContextMenu() -> void;
}

// --------------------------------------------------------------------------------------------------------------------
