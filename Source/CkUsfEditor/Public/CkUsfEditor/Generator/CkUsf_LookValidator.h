#pragma once

#include "CoreMinimal.h"

class UCkUsf_LookDefinition;

namespace ck::usf_editor
{
    struct FLookValidationResult
    {
        TArray<FString> Errors;
        TArray<FString> Warnings;

        auto Get_IsValid() const -> bool { return Errors.IsEmpty(); }
    };

    // Runs BEFORE any material object is created: the generator wires Custom-node inputs to the look
    // function POSITIONALLY, so an asset-vs-.ush mismatch renders silently wrong when the types happen
    // to coincide. Checks and their error/warning split: CkUsf/CLAUDE.md § The parameter contract.
    CKUSFEDITOR_API auto Validate_LookDefinition(const UCkUsf_LookDefinition* InDef) -> FLookValidationResult;
}
