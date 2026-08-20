#pragma once

#include "CoreMinimal.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkPixelArtRender_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, DisplayName = "CkUtils_PixelArtRender")
class CKPIXELARTRENDER_API UCk_Utils_PixelArtRender_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * The screen-percentage fraction that makes the renderer produce EXACTLY InTargetWidth texels.
     *
     * The scene renderer derives the internal view size as CeilToInt(UnscaledViewSize * Fraction), so the obvious
     * TargetWidth/ViewportWidth is one float rounding error away from yielding TargetWidth + 1 — and a single
     * stray texel column breaks the texel grid the whole technique rests on. Subtracting half a pixel before
     * dividing puts the product just below the target and forces CeilToInt onto it for any viewport width.
     *
     * Returns 1.0 (render at native resolution) for input that cannot describe a downscale: a non-positive
     * viewport or target, or a target wider than the viewport. That is a defined answer rather than a hidden
     * failure — this renderer never supersamples, and an infinity from dividing by zero would otherwise reach
     * the renderer's own resolution-fraction check.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|PixelArtRender",
              DisplayName = "[Ck][PixelArtRender] Get Exact Resolution Fraction")
    static float
    Get_ExactFraction(
        int32 InTargetWidth,
        int32 InViewportWidth);

    /**
     * Moves a view origin onto the texel lattice of its own right/up plane and reports the sub-texel remainder.
     *
     * The remainder is in texels, bounded by half a texel per axis, and expressed in the WORLD's sense of up.
     * Feeding it back as `Snapped + Remainder.X * Texel * Right + Remainder.Y * Texel * Up` reproduces the
     * original origin exactly, which is what lets the upscaler put the motion back that the snap took out.
     *
     * The reflected surface takes FVector2D where the underlying `ck::pixel_art` free function takes FVector2f:
     * FVector2f is marked BlueprintInternalUseOnly by the engine and cannot appear on a Blueprint node.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|PixelArtRender",
              DisplayName = "[Ck][PixelArtRender] Get Snapped View Origin")
    static FVector
    Get_SnappedViewOrigin(
        const FVector& InOrigin,
        const FMatrix& InViewRotationMatrix,
        float InTexelWorldSize,
        FVector2D& OutRemainderTexels);

    /**
     * The world-space width an orthographic projection spans across the full view rect (the `2 / M[0][0]`
     * extraction). Returns 0 for a projection with no horizontal scale.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|PixelArtRender",
              DisplayName = "[Ck][PixelArtRender] Get Ortho Width From Projection")
    static float
    Get_OrthoWidthFromProjection(
        const FMatrix& InProjection);

    /**
     * Horizontal render margin, in texels per side, that makes the rendered image exceed the displayed window by
     * at least InMarginTexels on every side at the given viewport aspect ratio.
     *
     * A margin asked for in texels cannot be spent evenly on both axes: the renderer scales both by one fraction,
     * so 2 texels of extra width buys only about 1.1 extra rows at 16:9 and fewer on an ultrawide. This returns
     * the wider horizontal margin whose vertical fallout still reaches the asked-for figure.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|PixelArtRender",
              DisplayName = "[Ck][PixelArtRender] Get Horizontal Margin Texels")
    static int32
    Get_HorizontalMarginTexels(
        int32 InInnerWidth,
        int32 InInnerHeight,
        int32 InMarginTexels,
        float InViewportAspect);
};
