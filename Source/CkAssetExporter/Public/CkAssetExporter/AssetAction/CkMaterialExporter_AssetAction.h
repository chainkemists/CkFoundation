#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter
{
    // Adds an "Export Material to JSON" entry to the Content Browser asset context menu (materials + instances).
    auto RegisterMaterialContextMenu() -> void;
}

// --------------------------------------------------------------------------------------------------------------------
