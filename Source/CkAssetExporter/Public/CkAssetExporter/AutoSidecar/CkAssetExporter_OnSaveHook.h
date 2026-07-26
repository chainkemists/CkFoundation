#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_OnSaveHook
{
public:
    static auto
    Register() -> void;

    static auto
    Unregister() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
