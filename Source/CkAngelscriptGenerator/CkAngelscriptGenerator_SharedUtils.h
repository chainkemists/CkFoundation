#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// These deliberately do NOT live in CkCore/Reflection: they encode AngelScript-specific emission
// conventions (snake_case namespaces, float32/float64 substitution, TEnumAsByte unwrapping).
// Language-neutral reflection helpers belong in UCk_Utils_Reflection_UE.
class CKANGELSCRIPTGENERATOR_API FCkAngelscriptGenerator_SharedUtils
{
public:
    // e.g. "TArray<FFoo*>" / "TEnumAsByte<EBar>" -> the AngelScript spelling.
    static auto
    Get_ConvertedToAngelscriptType(
        const FString& UnrealType) -> FString;

    // Preserves reflected TWeakObjectPtr / TSoftObjectPtr / TSoftClassPtr wrappers rather than
    // collapsing them to the pointee — the emitted mirror must keep the retention policy.
    static auto
    Get_DetailedPropertyType(
        FProperty* InProperty) -> FString;

    // e.g. UCk_Utils_Actor_UE -> utils_actor.
    static auto
    Get_ConvertedClassNameToNamespace(
        const FString& ClassName) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
