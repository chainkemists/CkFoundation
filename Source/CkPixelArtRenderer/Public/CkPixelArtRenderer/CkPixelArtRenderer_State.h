#pragma once

#include "CoreMinimal.h"

#include "CkPixelArtRenderer_State.generated.h"

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

// What decides the internal resolution. FixedHeight pins the texel count (the authored "360p", identical on
// every monitor); TexelsPerPixel pins the texel/pixel ratio instead, so the grid never resamples unevenly but
// a bigger window shows more world.
UENUM(BlueprintType)
enum class ECk_PixelArt_ResolutionMode : uint8
{
    FixedHeight,
    TexelsPerPixel
};

// --------------------------------------------------------------------------------------------------------------------

// What one world wants from the pixel-art renderer.
//
// Plain struct with public members rather than private _Members + CK_PROPERTY: this module cannot link CkCore
// at PostConfigInit. Same shape and reason as FCk_Iskm_BoneMatrix3x4 in the sibling PostConfigInit module; the
// reflected, accessor-bearing surface belongs in the Default-phase module that consumes this one.
struct FCk_PixelArt_RenderConfig
{
    bool Enabled = false;

    // Authored vertical texel count. The renderer derives the target WIDTH from this and the viewport aspect and
    // drives the screen percentage on width, because one fraction scales both axes and only one can be exact.
    int32 InternalHeight = 360;

    ECk_PixelArt_ResolutionMode ResolutionMode = ECk_PixelArt_ResolutionMode::FixedHeight;

    // Output pixels per texel in TexelsPerPixel mode; the internal height is the viewport height divided by it.
    int32 TexelsPerPixel = 4;

    // Texels rendered beyond the displayed window on each side, so re-applying the camera's sub-texel snap
    // remainder never samples texels that were not rendered.
    int32 MarginTexels = 2;

    bool SnapEnabled = true;

    ECk_PixelArt_UpscaleFilter FilterMode = ECk_PixelArt_UpscaleFilter::BoxFilter;
};

// --------------------------------------------------------------------------------------------------------------------

// What the renderer did to one frame, published for the frame it describes.
//
// The hooks run in a fixed order (SetupViewFamily, SetupViewProjectionMatrix, BeginRenderViewFamily) and the
// snap remainder computed in the middle one must reach the last. Anything outside the renderer that needs the
// camera's REAL position has no other source, because the snap never touches the gameplay camera.
//
// FrameNumber is GFrameCounter at publication: a mismatched reader holds last frame's answer and must treat it
// as absent rather than as slightly stale.
struct FCk_PixelArt_FrameReport
{
    uint64 FrameNumber = 0;

    // Texels the scene actually rasterizes into, including the margin on every side.
    FIntPoint RenderSize = FIntPoint::ZeroValue;

    // The displayed window inside that render. The offset is NOT always symmetric: the vertical margin is what the
    // engine's rounding leaves after the horizontal one is chosen, so the surplus row goes to the bottom.
    FIntPoint InnerOffsetTexels = FIntPoint::ZeroValue;
    FIntPoint InnerSizeTexels = FIntPoint::ZeroValue;

    // World units one texel spans. Zero when the view was not orthographic, which is also how a reader tells that
    // no snap was possible this frame.
    double TexelWorldSize = 0.0;

    FVector SnappedViewOrigin = FVector::ZeroVector;

    // Sub-texel part of the camera position the snap removed, in the view's right/up basis and the world's sense
    // of up. The upscaler flips the vertical sign to turn it into a UV shift.
    FVector2f RemainderTexels = FVector2f::ZeroVector;

    bool SnapApplied = false;
};

// --------------------------------------------------------------------------------------------------------------------

// The only channel between game-side configuration and the scene view extension.
//
// Keyed per world and WEAKLY: PIE runs several worlds at once, and a strong key would keep a dead world alive
// off the registry. Game thread only, asserted in every accessor - the render thread sees only the copy baked
// into the per-frame upscaler instance.
class CKPIXELARTRENDERER_API FCk_PixelArtRenderer_StateRegistry
{
public:
    // Replaces the world's configuration wholesale, and sweeps entries whose world is gone so the map cannot grow
    // across level transitions.
    static auto Set(
        const UWorld* InWorld,
        const FCk_PixelArt_RenderConfig& InConfig) -> void;

    // Forgetting a world that was never configured is a no-op, which is what lets teardown paths call this
    // unconditionally.
    static auto Clear(
        const UWorld* InWorld) -> void;

    // Unset means "nobody asked this world to render pixelated" - deliberately not a default-constructed config, so
    // an unconfigured world can never render pixelated by accident.
    static auto TryGet(
        const UWorld* InWorld) -> TOptional<FCk_PixelArt_RenderConfig>;

public:
    // Overwrites unconditionally: one report per world per frame, and a reader wanting the previous one was late.
    static auto Set_FrameReport(
        const UWorld* InWorld,
        const FCk_PixelArt_FrameReport& InReport) -> void;

    // Unset when the world has no report, or when the one it has belongs to an earlier frame — a caller must
    // never act on a stale snap.
    static auto TryGet_FrameReport(
        const UWorld* InWorld) -> TOptional<FCk_PixelArt_FrameReport>;

    // The report of the most recently ASSEMBLED frame, accepting one frame of age. For DIAGNOSTIC readers
    // only: a core ticker runs before the current frame's hooks, so the freshest report it can ever see is
    // last frame's. Anything that would act on the snap must use TryGet_FrameReport, whose strict freshness
    // is the contract.
    static auto TryGet_LastFrameReport(
        const UWorld* InWorld) -> TOptional<FCk_PixelArt_FrameReport>;
};
