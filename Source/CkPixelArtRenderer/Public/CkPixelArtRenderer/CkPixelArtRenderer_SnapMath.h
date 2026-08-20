#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// The camera-side arithmetic of the pixel-art technique, as pure functions. Separate from the scene view extension
// because this is the part that can be proven: the extension needs a rendered frame, this needs only its algebra.
namespace ck::pixel_art
{
    struct FCk_PixelArt_ViewBasis
    {
        FVector Right = FVector::ZeroVector;
        FVector Up = FVector::ZeroVector;
        FVector Forward = FVector::ZeroVector;
    };

    // ViewRotationMatrix maps world TO view, so its COLUMNS are the view axes in world space; its rows are the
    // transpose and are the single step most likely to be silently wrong. The engine also swizzles world
    // forward/right/up into view Z/X/Y, so the columns come out in the order above rather than X/Y/Z.
    CKPIXELARTRENDERER_API auto Get_ViewBasis(
        const FMatrix& InViewRotationMatrix) -> FCk_PixelArt_ViewBasis;

    /**
     * Moves the view origin onto the texel lattice of the view's own right/up plane, leaving view-forward alone.
     *
     * The lattice is view-aligned, not world-aligned: the rasterizer's sample grid is fixed to the render target,
     * so what must land on whole texels is the camera's position AS THE IMAGE SEES IT. Snapping world XY would
     * leave a rotated camera creeping exactly as before.
     *
     * OutRemainderTexels is (Origin - Snapped) resolved onto (Right, Up) over the texel size, so it is bounded by
     * half a texel per axis and reconstructs the original exactly:
     *     Snapped + (Remainder.X * Texel) * Right + (Remainder.Y * Texel) * Up == InOrigin
     * It is in the WORLD's sense of up; the upscaler flips the vertical sign because V grows downward.
     *
     * A non-positive texel size describes no lattice, so the origin comes back unchanged with a zero remainder.
     */
    CKPIXELARTRENDERER_API auto Get_SnappedViewOrigin(
        const FVector& InOrigin,
        const FMatrix& InViewRotationMatrix,
        double InTexelWorldSize,
        FVector2f& OutRemainderTexels) -> FVector;

    /**
     * The world-space width the projection spans across the full view rect (2 / M[0][0] for an orthographic one).
     *
     * Deliberately not "the camera's OrthoWidth": the engine divides by an axis multiplier that flips meaning
     * between landscape and portrait, and the span is what the texel size derives from either way. Returns 0 for
     * a projection with no horizontal scale.
     */
    CKPIXELARTRENDERER_API auto Get_OrthoWidthFromProjection(
        const FMatrix& InProjection) -> double;

    /**
     * Widens a projection so the RENDERED rect covers proportionally more world at an unchanged texel size, which
     * is what makes the margin extra image rather than a crop of the framing.
     *
     * Horizontal first, because width is the axis the screen percentage lands exactly. The vertical term is then
     * DERIVED from the rendered aspect rather than scaled by the same factor: the engine takes CeilToInt of the
     * rendered height, so one shared factor leaves texels a fraction of a percent off square.
     */
    CKPIXELARTRENDERER_API auto Apply_MarginFold(
        FMatrix& InOutProjection,
        const FIntPoint& InInnerSize,
        const FIntPoint& InRenderSize) -> void;

    /**
     * Horizontal margin, in texels per side, that makes the rendered image exceed the displayed window by at least
     * InMarginTexels on EVERY side.
     *
     * The margin cannot simply be added to both axes: one resolution fraction scales both, so the vertical margin
     * is whatever the horizontal one becomes after dividing by the aspect — 2 texels of width buys ~1.1 rows at
     * 16:9 and fewer on an ultrawide. Widening horizontally until the vertical fallout reaches the asked-for
     * margin is what keeps the guarantee aspect-independent.
     */
    CKPIXELARTRENDERER_API auto Get_HorizontalMarginTexels(
        int32 InInnerWidth,
        int32 InInnerHeight,
        int32 InMarginTexels,
        double InViewportAspect) -> int32;
}
