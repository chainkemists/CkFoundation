#pragma once

#include "CoreMinimal.h"

class UCkUsf_LookDefinition;

namespace ck::usf_editor
{
    // Result of validating one look definition against its .ush source.
    // Errors block generation; Warnings surface drift that is legal today but confusing tomorrow.
    struct FLookValidationResult
    {
        TArray<FString> Errors;
        TArray<FString> Warnings;

        auto Get_IsValid() const -> bool { return Errors.IsEmpty(); }
    };

    // Validates a look BEFORE any material object is created. This is the single-source-of-truth
    // gate for the parameter contract: the generator wires Custom-node inputs to the look function
    // POSITIONALLY, so an asset-vs-.ush mismatch either fails with an HLSL error that names nothing
    // about the asset, or — when types coincide — compiles and silently renders wrong. Checks:
    //   1. Param names are valid HLSL identifiers, unique (FName-insensitive — material parameter
    //      names are), not HLSL keywords, and not reserved (the Custom-node inputs/outputs and the
    //      generated locals `In`/`O`; a collision mis-wires or fails with a "redefinition" error).
    //   2. _PerInstance only on Scalar/Vector params (textures cannot ride custom data).
    //   3. _UshIncludePath resolves through the registered shader source directory mappings.
    //   4. The pixel entry point exists in that file (defined DIRECTLY in it, not a nested include
    //      or macro expansion) with the exact expected signature:
    //          FCkUsf_SurfaceOutput <Fn>(FCkUsf_SurfaceInput In, <params in declaration order>)
    //      Count/type/order mismatch = ERROR. HLSL param-NAME drift = WARNING (wiring is positional,
    //      so it works — but the next reader will be lied to).
    //   5. Same for the WPO entry point when set: float3 <Fn>(FCkUsf_VertexInput In, <same params>).
    CKUSFEDITOR_API auto Validate_LookDefinition(const UCkUsf_LookDefinition* InDef) -> FLookValidationResult;
}
