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

    // Reports a look's real HLSL compile errors (the resource's own error list, so a failure names its file,
    // line and message). Generation runs this itself; it is exported so a test can hold a one-off master to the
    // SAME bar instead of re-implementing a weaker check.
    // Returns true (clean) when the process cannot render — a -nullrhi run never builds shader maps.
    //
    // InForceSynchronousCompile is what gives the check TEETH, and it is DESTRUCTIVE: without it the shader jobs
    // are still pending and a broken .ush passes (mutation-tested 2026-08-06). With it, the material is left
    // without an applied game-thread shader map — measured, that makes the master render BLACK, which is why
    // generation must NOT force it across the roster. Pass true only for a throwaway master you are about to
    // discard; pass false to gate a master that will actually be used.
    CKUSFEDITOR_API auto Validate_LookShaderCompile(
        UMaterial* InMaterial, FName InLookName, TArray<FString>& OutErrors,
        bool InForceSynchronousCompile) -> bool;
}
