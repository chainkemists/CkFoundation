#pragma once

#include "CoreMinimal.h"

#include <HAL/IConsoleManager.h>
#include <SceneViewExtension.h>

#include "CkPixelArtRenderer/CkPixelArtRenderer_State.h"

// --------------------------------------------------------------------------------------------------------------------

// Drives the pixel-art render path for one frame across four game-thread hooks, which run in a fixed order inside
// UGameViewportClient::Draw. That single-threaded order is what lets one member carry the frame's state from hook
// to hook without a lock; the frame number stamped on it is the guard against consuming a hook that did not run.
//
// Disabling is a LEASE, not an event — see this module's Claude.md for why there is no "on disabled" callback.
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

    // Saved beside the value, because a console variable remembers who set it last and refuses weaker
    // writers. Restoring the number without the priority would leave r.ScreenPercentage pinned at this
    // renderer's own priority and silently kill the scalability slider that writes it.
    EConsoleVariableFlags _SavedScreenPercentagePriority = ECVF_SetByConstructor;

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
