#pragma once

#include "CoreMinimal.h"

#include "SceneViewExtension.h"

#include "CkPixelArtRender/CkPixelArtRender_State.h"

// --------------------------------------------------------------------------------------------------------------------

// Drives the whole pixel-art render path for one frame:
//
//   IsActiveThisFrame        — resolve the context world's configuration (registry + console overlay)
//   SetupViewFamily          — drive the screen percentage so the scene rasterizes at the internal resolution
//   SetupViewProjectionMatrix— fold the render margin into the projection and snap the view origin to the texel
//                              lattice, keeping the sub-texel remainder
//   BeginRenderViewFamily    — install the box-filter upscaler for THIS family (the only legal registration
//                              point; the renderer checks all three upscaler slots are null first)
//
// Those three run in that order inside UGameViewportClient::Draw (GameViewportClient.cpp:1486, then :1687 via
// LocalPlayer::CalcSceneView, then :1971), all on the game thread, which is what lets one member carry the
// frame's state from hook to hook without a lock or a registry round trip. The frame number stamped on that
// member is the guard: a hook that does not run leaves the previous frame's answer behind, and consuming it would
// snap to a lattice the scene was never rendered against.
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

    auto SetupViewProjectionMatrix(
        FSceneViewProjectionData& InOutProjectionData) -> void override;

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

    // What the renderer is doing to the frame currently being assembled, built up across the three hooks and
    // published to the state registry once the snap is known. Reset at end of frame.
    TOptional<FCk_PixelArt_FrameReport> _FrameReport;

    // Resolved in SetupViewFamily, which is the only hook that is handed the scene. SetupViewProjectionMatrix
    // receives projection data and nothing else, so without this it could not name the world it is publishing for.
    TWeakObjectPtr<const UWorld> _FrameWorld;

    // Origin held by `ck.PixelArt.Debug.FreezeSnap`. Kept across frames on purpose — freezing is only meaningful
    // if the held value outlives the frame that captured it.
    TOptional<FVector> _FrozenViewOrigin;

    FDelegateHandle _EndFrameHandle;

    bool _LeaseRenewedThisFrame = false;

    FIntPoint _LastViewportSize = FIntPoint::ZeroValue;
    float _LastFraction = 0.0f;
    float _LastSecondaryViewFraction = 1.0f;
    bool _LastUpscaleSlotWasLive = true;
};
