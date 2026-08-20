#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// The camera-side arithmetic of the pixel-art technique, as pure functions of their arguments.
//
// It lives apart from the scene view extension because it is the part that can be proven: the extension can only
// be exercised by rendering a frame, while everything here is testable against its own algebra (idempotence, the
// half-texel bound, exact reconstruction, forward invariance).
namespace ck::pixel_art
{
    // The view's axes expressed in world space. Not a general basis utility — it exists because the direction of
    // a snap has to be the camera's own right/up, and reading those out of a world-to-view matrix is the single
    // step most likely to be silently transposed.
    struct FCk_PixelArt_ViewBasis
    {
        FVector Right = FVector::ZeroVector;
        FVector Up = FVector::ZeroVector;
        FVector Forward = FVector::ZeroVector;
    };

    // FSceneViewProjectionData::ViewRotationMatrix maps world space TO view space, so its COLUMNS are the view
    // axes in world space (its rows are where the world axes end up, which is the transpose of what a snap
    // needs). The engine also swizzles on the way in — world forward/right/up become view Z/X/Y — so the columns
    // come out in the order below rather than X/Y/Z.
    CKPIXELARTRENDER_API auto Get_ViewBasis(
        const FMatrix& InViewRotationMatrix) -> FCk_PixelArt_ViewBasis;

    // Moves the view origin onto the texel lattice of the view's own right/up plane, leaving the view-forward
    // component untouched, and reports what was taken off.
    //
    // The lattice is view-aligned rather than world-aligned because the rasterizer's sample grid is fixed to the
    // render target: what has to land on whole texels is the camera's position AS THE IMAGE SEES IT. Snapping
    // world XY would leave a rotated camera creeping exactly as before.
    //
    // OutRemainderTexels is (Origin - Snapped) resolved onto (Right, Up) and divided by the texel size, so it is
    // bounded by half a texel per axis and reconstructs the original origin exactly:
    //     Snapped + (Remainder.X * Texel) * Right + (Remainder.Y * Texel) * Up == InOrigin
    // It is expressed in the WORLD's sense of up, not the image's: the upscaler flips the vertical sign when it
    // turns this into a UV shift, because V grows downward.
    //
    // A non-positive texel size describes no lattice at all, so the origin is returned unchanged with a zero
    // remainder — the caller renders unsnapped rather than dividing by zero.
    CKPIXELARTRENDER_API auto Get_SnappedViewOrigin(
        const FVector& InOrigin,
        const FMatrix& InViewRotationMatrix,
        double InTexelWorldSize,
        FVector2f& OutRemainderTexels) -> FVector;

    // The world-space width the projection spans across the full view rect.
    //
    // For an orthographic projection M[0][0] is 1 / half-width, so the width is 2 / M[0][0]. Deliberately phrased
    // as "what the projection spans" and not "the camera's OrthoWidth": the engine divides by an axis multiplier
    // that flips meaning between landscape and portrait viewports, and it is the span the texel size is derived
    // from either way. Returns 0 for a projection with no horizontal scale.
    CKPIXELARTRENDER_API auto Get_OrthoWidthFromProjection(
        const FMatrix& InProjection) -> double;

    // Horizontal margin, in texels per side, that makes the rendered image exceed the displayed window by at
    // least InMarginTexels on EVERY side.
    //
    // The margin cannot simply be added to both axes: the renderer applies one resolution fraction to both, so
    // the vertical margin is whatever the horizontal one becomes after division by the aspect ratio — 2 texels of
    // width buys only ~1.1 rows at 16:9 and less than one on an ultrawide. Widening horizontally until the
    // vertical fallout reaches the asked-for margin is what keeps the guarantee aspect-independent.
    CKPIXELARTRENDER_API auto Get_HorizontalMarginTexels(
        int32 InInnerWidth,
        int32 InInnerHeight,
        int32 InMarginTexels,
        double InViewportAspect) -> int32;
}
