#pragma once

#include "CoreMinimal.h"

#include "GlobalShader.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"

// --------------------------------------------------------------------------------------------------------------------

// Fullscreen box-filter upscale from the internal (pixel-art) resolution to the output resolution. The filter
// itself and why it is the right one are documented in Shaders/CkPixelArt/PixelArtUpscale.usf.
//
// The displayed window is NOT passed as parameters: it rides FScreenPassTextureViewport through
// AddDrawScreenPass, so the vertex shader's UV already spans exactly the rect this pass was told to read.
class FCk_PixelArt_UpscalePS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FCk_PixelArt_UpscalePS);
    SHADER_USE_PARAMETER_STRUCT(FCk_PixelArt_UpscalePS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FVector2f, InputExtent)
        SHADER_PARAMETER(FVector2f, InputExtentInv)
        SHADER_PARAMETER(FVector2f, SubTexelOffsetTexels)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    // Point sampling instead of the box filter. A permutation rather than a uniform branch because it is a debug
    // path taken for a whole frame, never per pixel.
    class FNearestDebugDim : SHADER_PERMUTATION_BOOL("NEAREST_DEBUG");

    using FPermutationDomain = TShaderPermutationDomain<FNearestDebugDim>;

public:
    static auto ShouldCompilePermutation(
        const FGlobalShaderPermutationParameters& InParameters) -> bool;
};
