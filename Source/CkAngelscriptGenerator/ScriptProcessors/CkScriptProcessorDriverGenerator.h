#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Generates one AngelScript "<Dev>_Driver" per typed script processor — every dev class declaring
// a plain (non-UFUNCTION) `void ForEachEntity(FCk_Time, [FCk_Handle,] <fragment params>...)`.
//
// The driver SUBCLASSES the dev class, so ForEachEntity is inherited and called directly from the
// generated ForEachBatch, and Configure infers slots from the signature (const&/value -> ReadOnly,
// & -> ReadWrite). A hand-authored <Dev>_Driver suppresses emission for that dev class.
// Emits into <plugin|project>/Script/Generated/<Name>_ScriptProcessorDrivers.as.
// --------------------------------------------------------------------------------------------------------------------

class FCkScriptProcessorDriverGenerator
{
public:
    static auto GenerateAll() -> void;
};
