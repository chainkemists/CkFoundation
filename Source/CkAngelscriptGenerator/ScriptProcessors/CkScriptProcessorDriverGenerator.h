#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Generates one AngelScript "<Dev>_Driver" class per typed script processor — a UCk_Processor_Script_Base_UE subclass
// that declares a plain (non-UFUNCTION) method:
//
//     void ForEachEntity(FCk_Time InDeltaT, [FCk_Handle InHandle,] <fragment params>...)
//
// The generated driver is a SIBLING of the dev class (never a base — the dev class always compiles standalone). Its
// ForEachBatch loops the natively-joined batch and forwards each element to the dev's ForEachEntity; its Configure
// declares the slots inferred from the signature (const&/value -> ReadOnly, & -> ReadWrite) and forwards to the dev's
// own Configure for Require/Exclude/NoEntities.
//
// Mirrors the AutoTest wrapper generator: runs at OnPostEngineInit and after every AngelScript recompile, enumerates
// via TObjectIterator + IsChildOf, reflects the method through the AS type system, emits into
// <plugin|project>/Script/Generated/<Name>_ScriptProcessorDrivers.as, and writes ONLY when content changes (so it
// never re-triggers its own post-compile pass). A hand-authored <Dev>_Driver suppresses emission for that dev class.
// --------------------------------------------------------------------------------------------------------------------

class FCkScriptProcessorDriverGenerator
{
public:
    static auto GenerateAll() -> void;
};
