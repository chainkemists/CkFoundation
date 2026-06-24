#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Macros/CkMacros.h"

#include "CkWidgetRasterizer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UUserWidget;
class UTexture2D;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WidgetRasterizer_GammaCorrection : uint8
{
    // Renderer applies gamma correction (linear->sRGB). Correct when the render target is left
    // linear; against an sRGB render target it double-encodes (washes the result out). Default
    // to preserve existing callers.
    Enabled,

    // Renderer skips gamma correction — the source widget's sRGB art is drawn as-is so the sRGB
    // render target does the single encode. Use this when Enabled looks washed-out / too light.
    Disabled
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Off-screen UMG widget -> transient UTexture2D rasterizer (no viewport needed).
 *
 * The widget->texture sibling of UCk_Utils_RenderTarget (scene/primitive->texture).
 * Generic: no game-specific knowledge. Wraps FWidgetRenderer + SVirtualWindow so a
 * widget composed off-screen can be baked into a texture for use on a material
 * (3D mesh cover art, item icons, in-world screens, photo-mode thumbnails, ...).
 *
 * The texture is a pure local function of whatever the widget was initialized from,
 * so it never needs to be replicated or saved — replicate the value-data that
 * builds the widget and rasterize identically on each client.
 *
 * Game-thread + client-only (cosmetic). Returns nullptr on processes without a real
 * RHI/Slate renderer (dedicated server, -nullrhi headless) — callers must fall back.
 */
UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_WidgetRasterizer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_WidgetRasterizer_UE);

public:
    /**
     * Rasterize InWidget to a transient texture at InRenderSize. Two-pass render
     * (prepass for layout, then draw) so AutoWrap/justified text resolves.
     *
     * Returns nullptr when rasterization is unavailable on this process or the
     * inputs are invalid. The returned texture is GC-eligible on return — the
     * caller MUST root it (store in a UPROPERTY) or it will be collected.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              DisplayName = "[Ck][WidgetRasterizer] Render Widget To Texture",
              Category = "Ck|Utils|WidgetRasterizer")
    static UTexture2D*
    RenderWidgetToTexture(
        UUserWidget* InWidget,
        FIntPoint InRenderSize,
        ECk_WidgetRasterizer_GammaCorrection InGammaCorrection = ECk_WidgetRasterizer_GammaCorrection::Enabled);

    /** Whether widget rasterization can run on this process (real RHI + Slate renderer present). */
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][WidgetRasterizer] Get Can Rasterize On This Process",
              Category = "Ck|Utils|WidgetRasterizer")
    static bool
    Get_CanRasterizeOnThisProcess();
};
