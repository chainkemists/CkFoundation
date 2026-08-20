#include "CkPixelArtRender/CkPixelArtRender_UpscaleShader.h"

#include "DataDrivenShaderPlatformInfo.h"

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_GLOBAL_SHADER(FCk_PixelArt_UpscalePS, "/CkPixelArt/PixelArtUpscale.usf", "MainPS", SF_Pixel);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PixelArt_UpscalePS::
    ShouldCompilePermutation(
        const FGlobalShaderPermutationParameters& InParameters)
    -> bool
{
    return IsFeatureLevelSupported(InParameters.Platform, ERHIFeatureLevel::SM5);
}
