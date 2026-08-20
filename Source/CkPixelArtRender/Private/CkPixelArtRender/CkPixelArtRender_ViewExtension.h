#pragma once

#include "CoreMinimal.h"

#include "SceneViewExtension.h"

// --------------------------------------------------------------------------------------------------------------------

// Drives the whole pixel-art render path for one frame:
//
//   SetupViewFamily        — drive the screen percentage so the scene rasterizes at the internal resolution
//   BeginRenderViewFamily  — install the box-filter upscaler for THIS family (the only legal registration point;
//                            the renderer checks all three upscaler slots are null before the extension callbacks)
//
// The internal size is currently a fixed constant and the camera is not snapped, so the sub-texel remainder handed
// to the upscaler is always zero.
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
    // Puts r.ScreenPercentage back exactly as it was found before the spike first touched it, and forgets the saved
    // value. Idempotent, and a no-op when the spike never ran — so both the runtime toggle-off and teardown can call
    // it unconditionally and leave zero residue.
    auto Request_RestoreScreenPercentage() -> void;

private:
    auto DoApply_ScreenPercentage(
        const FSceneViewFamily& InViewFamily) -> void;

private:
    TOptional<float> _SavedScreenPercentage;
    FIntPoint _LastViewportSize = FIntPoint::ZeroValue;
    FIntPoint _LastInternalSize = FIntPoint::ZeroValue;
    float _LastFraction = 0.0f;
    float _LastSecondaryViewFraction = 1.0f;
};
