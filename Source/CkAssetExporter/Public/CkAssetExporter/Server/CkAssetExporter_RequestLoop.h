#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_RequestLoop
{
public:
    // Serves until a 10-minute idle window, a 2-hour wall-clock cap, or a quit request. Always returns 0.
    static auto
    Run() -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
