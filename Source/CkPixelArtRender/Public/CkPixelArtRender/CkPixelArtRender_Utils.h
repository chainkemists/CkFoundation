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
};
