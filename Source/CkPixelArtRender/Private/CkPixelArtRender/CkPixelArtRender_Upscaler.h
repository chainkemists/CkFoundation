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
    FIntPoint RenderSize = FIntPoint::ZeroValue;

    // The displayed window inside that render: the rendered image is larger on every side so shifting the
    // sampling window by the snap remainder never reads texels that were never rendered. The offset is not
    // necessarily symmetric — the vertical margin is what the engine's rounding leaves once the horizontal one is
    // chosen, and the surplus row goes to the bottom.
    FIntPoint InnerOffsetTexels = FIntPoint::ZeroValue;
    FIntPoint InnerSizeTexels = FIntPoint::ZeroValue;

    // Remainder of the camera's texel-grid snap, already converted to a source-texel shift for the shader: the
    // horizontal component as-is, the vertical one negated because V grows downward. Adding it to the sampled
    // position moves the displayed content the way the un-snapped camera would have. Zero means "not snapped".
    FVector2f SubTexelOffsetTexels = FVector2f::ZeroVector;

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
