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

    // Texels rendered beyond the displayed window on each side, so re-applying the camera's sub-texel snap
    // remainder never samples texels that were not rendered.
    int32 MarginTexels = 2;

    bool SnapEnabled = true;

    ECk_PixelArt_UpscaleFilter FilterMode = ECk_PixelArt_UpscaleFilter::BoxFilter;
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
};
