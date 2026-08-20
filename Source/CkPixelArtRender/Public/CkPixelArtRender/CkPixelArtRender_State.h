#pragma once

#include "CoreMinimal.h"

#include "CkPixelArtRender_State.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PixelArt_UpscaleFilter : uint8
{
    // Area-average of the source over each output pixel's footprint, from a single bilinear tap at a
    // derivative-shifted UV. Sharp at texel boundaries without the staircase of point sampling.
    BoxFilter,

    // Point sampling. A debug filter: it makes the texel grid unambiguous when diagnosing snap or margin
    // problems, at the cost of aliasing the box filter exists to avoid.
    Nearest
};

// --------------------------------------------------------------------------------------------------------------------

// What decides the internal resolution.
//
// FixedHeight pins the texel count and lets the texel/pixel ratio fall where the window puts it — the authored
// "360p" that looks the same on every monitor. TexelsPerPixel pins the ratio instead, so the grid is always a
// whole number of screen pixels and never resamples unevenly, at the cost of a bigger window showing more world.
UENUM(BlueprintType)
enum class ECk_PixelArt_ResolutionMode : uint8
{
    FixedHeight,
    TexelsPerPixel
};

// --------------------------------------------------------------------------------------------------------------------

// What one world wants from the pixel-art renderer.
//
// Plain struct with public members rather than the house private-`_Member` + CK_PROPERTY shape: this module loads
// at PostConfigInit and so cannot link CkCore, where those macros live (see CkPixelArtRender.Build.cs). Same
// reason and same shape as FCk_Iskm_BoneMatrix3x4 in the sibling PostConfigInit module. The reflected,
// accessor-bearing configuration surface belongs in the Default-phase module that consumes this one.
struct FCk_PixelArt_RenderConfig
{
    bool Enabled = false;

    // Authored vertical texel count — the "360p" knob. The renderer derives the target WIDTH from this and the
    // viewport aspect, then drives the screen percentage on width, because the engine applies one resolution
    // fraction to both axes and only one of them can be made exact.
    int32 InternalHeight = 360;

    ECk_PixelArt_ResolutionMode ResolutionMode = ECk_PixelArt_ResolutionMode::FixedHeight;

    // Output pixels per texel when the mode is TexelsPerPixel. The internal height is then the viewport height
    // divided by this, so the texel grid lands on whole screen pixels at any window size.
    int32 TexelsPerPixel = 4;

    // Texels rendered beyond the displayed window on each side, so re-applying the camera's sub-texel snap
    // remainder never samples texels that were not rendered.
    int32 MarginTexels = 2;

    bool SnapEnabled = true;

    ECk_PixelArt_UpscaleFilter FilterMode = ECk_PixelArt_UpscaleFilter::BoxFilter;
};

// --------------------------------------------------------------------------------------------------------------------

// What the renderer actually did to one frame, published for the frame it describes.
//
// Two readers need it. The upscaler's hooks run in a fixed order within a frame (SetupViewFamily, then
// SetupViewProjectionMatrix, then BeginRenderViewFamily) and the snap remainder computed in the middle one has to
// reach the last one. Everything else outside the renderer that cares where the camera REALLY ended up — a
// world-space widget projecting to the screen, a debug pan that wants to move by exact texels — needs the same
// numbers and has no other way to get them, because the snap deliberately never touches the gameplay camera.
//
// FrameNumber is GFrameCounter at publication. A reader whose frame number does not match is holding last
// frame's answer and must treat it as absent rather than as slightly stale.
struct FCk_PixelArt_FrameReport
{
    uint64 FrameNumber = 0;

    // Texels the scene actually rasterizes into, including the margin on every side.
    FIntPoint RenderSize = FIntPoint::ZeroValue;

    // The displayed window inside that render: where it starts, and how big it is. The offset is NOT always
    // symmetric — the vertical margin is whatever the engine's rounding leaves after the horizontal one is
    // chosen, so the surplus row goes to the bottom.
    FIntPoint InnerOffsetTexels = FIntPoint::ZeroValue;
    FIntPoint InnerSizeTexels = FIntPoint::ZeroValue;

    // World units one texel spans. Zero when the view was not orthographic, which is also how a reader tells that
    // no snap was possible this frame.
    double TexelWorldSize = 0.0;

    FVector SnappedViewOrigin = FVector::ZeroVector;

    // Sub-texel part of the camera position the snap removed, in texels, in the view's right/up basis and the
    // world's sense of up. The upscaler flips the vertical sign to turn it into a UV shift.
    FVector2f RemainderTexels = FVector2f::ZeroVector;

    bool SnapApplied = false;
};

// --------------------------------------------------------------------------------------------------------------------

// The only channel between game-side configuration and the scene view extension's hooks.
//
// Keyed per world and weakly, because PIE runs several worlds at once (configuring one must never pixelate the
// others) and because a strong key would keep a dead world's UObject alive off the registry — the same trap the
// house rules describe for UObject references held outside the reflection system.
//
// Game thread only, asserted in every accessor: every hook that reads it (SetupViewFamily,
// SetupViewProjectionMatrix, BeginRenderViewFamily and the activation functor) is a game-thread callback, and the
// render thread must only ever see the copy baked into the per-frame upscaler instance.
class CKPIXELARTRENDER_API FCk_PixelArtRender_StateRegistry
{
public:
    // Replaces the world's configuration wholesale — this is not a merge. Also sweeps entries whose world has
    // been destroyed, so the map cannot grow across level transitions.
    static auto Set(
        const UWorld* InWorld,
        const FCk_PixelArt_RenderConfig& InConfig) -> void;

    // Forgetting a world that was never configured is a no-op, which is what lets teardown paths call this
    // unconditionally.
    static auto Clear(
        const UWorld* InWorld) -> void;

    // Unset means "nobody asked this world to render pixelated" — deliberately NOT a default-constructed config,
    // so a world nobody configured can never render pixelated by accident.
    static auto TryGet(
        const UWorld* InWorld) -> TOptional<FCk_PixelArt_RenderConfig>;

public:
    // Publishes what the renderer did to the frame currently being assembled. Overwrites unconditionally: there
    // is exactly one report per world per frame and a reader that wanted the previous one was already too late.
    static auto Set_FrameReport(
        const UWorld* InWorld,
        const FCk_PixelArt_FrameReport& InReport) -> void;

    // Unset when the world has no report, or when the one it has belongs to an earlier frame — a caller must
    // never act on a stale snap.
    static auto TryGet_FrameReport(
        const UWorld* InWorld) -> TOptional<FCk_PixelArt_FrameReport>;
};
