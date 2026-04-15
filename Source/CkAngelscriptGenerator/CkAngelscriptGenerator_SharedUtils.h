#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Shared AngelScript type / naming conversion helpers used by multiple generators in this module.
//
// These helpers are intentionally scoped to AngelScript code generation — they encode AS-specific
// conventions (snake_case namespaces, float32/float64 substitution, TEnumAsByte unwrapping, etc.)
// and therefore do not belong in CkCore/Reflection. Language-neutral reflection helpers live in
// UCk_Utils_Reflection_UE.
class CKANGELSCRIPTGENERATOR_API FCkAngelscriptGenerator_SharedUtils
{
public:
    // Maps a raw Unreal type string (e.g. "TArray<FFoo*>", "float", "TEnumAsByte<EBar>") to the
    // AngelScript-equivalent type. Recursively handles TArray / TMap / TSet / TOptional inner types,
    // unwraps TEnumAsByte, strips pointers and trailing "const".
    static auto
    Get_ConvertedToAngelscriptType(
        const FString& UnrealType) -> FString;

    // Resolves an FProperty to the AngelScript type string to emit. Handles TArray / TMap / TSet /
    // TOptional via recursion, returns enum names for FEnumProperty / FByteProperty-with-enum,
    // converts floats to float32 and doubles to float64. Falls back to GetCPPType + conversion.
    static auto
    Get_DetailedPropertyType(
        FProperty* InProperty) -> FString;

    // Converts a UClass name to the snake_case namespace used by the wrapper generator
    // (e.g. UCk_Utils_Actor_UE -> utils_actor). Strips leading "U" / "Ck_", trailing "_UE" /
    // "FunctionLibrary", and lowercases at word boundaries.
    static auto
    Get_ConvertedClassNameToNamespace(
        const FString& ClassName) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
