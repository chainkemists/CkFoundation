#pragma once

#include "CoreMinimal.h"

#include "SceneViewExtension.h"

#include "CkPixelArtRender/CkPixelArtRender_State.h"

// --------------------------------------------------------------------------------------------------------------------

// Drives the whole pixel-art render path for one frame:
//
//   IsActiveThisFrame      — resolve the context world's configuration (registry + console overlay)
//   SetupViewFamily        — drive the screen percentage so the scene rasterizes at the internal resolution
//   BeginRenderViewFamily  — install the box-filter upscaler for THIS family (the only legal registration point;
//                            the renderer checks all three upscaler slots are null before the extension callbacks)
//
// Disabling is a LEASE, not an event. Every frame the renderer is meant to be running, SetupViewFamily renews the
// lease; a frame that ends without a renewal releases it and puts r.ScreenPercentage back. That converges from
// every path that can stop us — the console override, the world's configuration being cleared, the world being
// torn down, or the activation functor going false — instead of working only for the one path we remembered to
// hook. It is also why there is no "on disabled" callback anywhere in this class.
class FCk_PixelArt_ViewExtension : public FSceneViewExtensionBase
{
public:
    explicit FCk_PixelArt_ViewExtension(
        const FAutoRegister& InAutoRegister);

    virtual ~FCk_PixelArt_ViewExtension() override;

public:
    auto SetupViewFamily(
        FSceneViewFamily& InViewFamily) -> void override;

    auto BeginRenderViewFamily(
        FSceneViewFamily& InViewFamily) -> void override;

    auto IsActiveThisFrame_Internal(
        const FSceneViewExtensionContext& InContext) const -> bool override;

public:
    // Puts r.ScreenPercentage back exactly as it was found before this extension first drove it, and forgets the
    // saved value. Idempotent, and a no-op when the renderer never ran — so teardown may call it unconditionally.
    auto Request_ReleaseScreenPercentage() -> void;

private:
    auto DoApply_ScreenPercentage(
        const FSceneViewFamily& InViewFamily,
        const FCk_PixelArt_RenderConfig& InConfig) -> void;

    auto DoOn_EndFrame() -> void;

private:
    TOptional<FCk_PixelArt_RenderConfig> _FrameConfig;
    TOptional<float> _SavedScreenPercentage;

    FDelegateHandle _EndFrameHandle;

    bool _LeaseRenewedThisFrame = false;

    FIntPoint _LastViewportSize = FIntPoint::ZeroValue;
    FIntPoint _LastInternalSize = FIntPoint::ZeroValue;
    int32 _LastMarginTexels = 0;
    float _LastFraction = 0.0f;
    float _LastSecondaryViewFraction = 1.0f;
    bool _LastUpscaleSlotWasLive = true;
};
