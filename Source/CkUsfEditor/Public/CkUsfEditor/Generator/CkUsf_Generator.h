#pragma once

#include "CoreMinimal.h"

class UCkUsf_LookDefinition;
class UMaterial;

namespace ck::usf_editor
{
    struct FGenerateResult
    {
        int32 NumGenerated = 0;
        int32 NumSkipped = 0;
        TArray<FString> Errors;     // validation + shader-compile failures (each names its look)
        TArray<FString> Warnings;   // legal-but-drifting looks (e.g. HLSL param-name mismatch)
    };

    // Discovers all UCkUsf_LookDefinition assets and generates/refreshes a master per look.
    CKUSFEDITOR_API auto Generate_AllLookMaterials() -> FGenerateResult;

    // Generates/refreshes one master. Returns nullptr on validation failure (logged).
    CKUSFEDITOR_API auto Generate_LookMaterial(UCkUsf_LookDefinition* InDef) -> UMaterial*;
}
