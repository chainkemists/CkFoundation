#pragma once

#include "CoreMinimal.h"

#include "PostProcess/PostProcessUpscale.h"

#include "CkPixelArtRender/CkPixelArtRender_State.h"

// --------------------------------------------------------------------------------------------------------------------

// Everything the upscale pass needs to know about ONE frame. Baked on the game thread and handed to the upscaler
// instance that BeginRenderViewFamily news up for that frame; the view family owns and deletes that instance, so
// this is the whole game-thread-to-render-thread transport — no globals, no cross-frame cache, no race.
struct FCk_PixelArt_UpscaleFrame
{
    // Internal view rect size the game thread drove the screen percentage towards. The pass reads the rect it is
    // actually handed; this is here so a breadcrumb can name a disagreement between the two.
    FIntPoint InternalSize = FIntPoint::ZeroValue;

    // Remainder of the camera's texel-grid snap, in source texels, re-applied by the upscale shader so motion
    // stays sub-texel smooth while the raster stays grid-aligned. Zero means "camera not snapped".
    FVector2f SubTexelOffsetTexels = FVector2f::ZeroVector;

    // Texels rendered beyond the displayed window on each side, so shifting the sampling window by the remainder
    // above never reads outside the rendered image. Callers guarantee InternalSize > 2 * MarginTexels per axis.
    int32 MarginTexels = 0;

    ECk_PixelArt_UpscaleFilter FilterMode = ECk_PixelArt_UpscaleFilter::BoxFilter;
};

// --------------------------------------------------------------------------------------------------------------------

// Replaces the engine's primary spatial upscale pass entirely (PostProcessing.cpp: a non-null custom upscaler is
// called INSTEAD of AddDefaultUpscalePass). The output geometry rules below are therefore not ours to choose —
// they mirror ISpatialUpscaler::AddDefaultUpscalePass, because the renderer checks the result against them.
class FCk_PixelArt_SpatialUpscaler final : public ISpatialUpscaler
{
public:
    explicit FCk_PixelArt_SpatialUpscaler(
        const FCk_PixelArt_UpscaleFrame& InFrame);

public:
    auto GetDebugName() const -> const TCHAR* override;

    auto Fork_GameThread(
        const FSceneViewFamily& InViewFamily) const -> ISpatialUpscaler* override;

    auto AddPasses(
        FRDGBuilder& InGraphBuilder,
        const FViewInfo& InView,
        const FInputs& InPassInputs) const -> FScreenPassTexture override;

private:
    FCk_PixelArt_UpscaleFrame _Frame;
};
