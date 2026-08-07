#pragma once

#include "CoreMinimal.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkUsf_HandDrawn_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Hatching pattern drawn in shadow regions. The order is a CONTRACT with CkUsf_Stylize_StrokePattern's
// dispatcher in /CkUsf/StylizeCommon.ush — the subsystem writes the enum's integer value straight into
// the look's StrokePattern parameter, so reordering it silently re-draws every hatched region.
UENUM(BlueprintType)
enum class ECk_Usf_HandDrawnStrokePattern : uint8
{
    DiagonalPencil,
    Crosshatch,
    LooseScribble,
    Stipple
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_HandDrawnStrokePattern);

// --------------------------------------------------------------------------------------------------------------------

// Where the stroke lattice lives. Order is a contract with HandDrawn.ush.
//
// ScreenStable is camera-aligned: it never swims on a moving subject, which is what makes it the choice
// for animated scenes. WorldAttached anchors the lattice to the depth-reconstructed scene surface, so it
// holds still under camera motion — and SLIDES on anything that translates or skins, because correcting
// that needs a frame history a post-process blendable does not have.
UENUM(BlueprintType)
enum class ECk_Usf_HandDrawnStrokeSpace : uint8
{
    ScreenStable,
    WorldAttached
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_HandDrawnStrokeSpace);

// --------------------------------------------------------------------------------------------------------------------

// Order is a contract with HandDrawn.ush's DebugMode dispatcher. Every mode past FinalImage bypasses the
// master StyleStrength lerp — a mask viewed at partial strength is a faint tint, not a diagnostic.
UENUM(BlueprintType)
enum class ECk_Usf_HandDrawn_DebugMode : uint8
{
    FinalImage,
    InkMask,
    ShadowStrokeMask,
    PaperPattern,
    WorldNormals,
    SceneDepth
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_HandDrawn_DebugMode);

// --------------------------------------------------------------------------------------------------------------------

// Every runtime-drivable setting of the HandDrawn look, in one reflected value. The subsystem owns one of
// these and projects it onto the look's MID; a preset data asset is just an authored instance.
// Defaults ARE the "StorybookInk" preset — an authored preset that differs from these differs on purpose.
USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_HandDrawn_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Usf_HandDrawn_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    // The master weight over the WHOLE composite — paint, ink, strokes and paper together. Unlike the cel
    // look's per-group strengths, a drawing is one medium: there is no state where the ink is at full
    // weight and the paper is absent. 0 renders the untouched frame (the subsystem's Enabled is still the
    // real passthrough — this one pays for the pass).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StyleStrength = 1.0f;

    // ---- Paint ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _SimplifyColor = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 16,
                      EditCondition = "_SimplifyColor == ECk_EnableDisable::Enable"))
    int32 _ColorLevels = 5;

    // Width of the blend between two colour regions, as a fraction of one region's slice. 0 is a hard
    // posterized edge; higher values read as a painted transition rather than a step.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_SimplifyColor == ECk_EnableDisable::Enable"))
    float _ColorSoftness = 0.15f;

    // Applied BEFORE the region reduction, so it changes which colours get merged rather than recolouring
    // the result.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Saturation = 0.9f;

    // Also pre-shaping, and applied in the look's normalized tone domain — contrast about 0.5 means
    // nothing in unbounded scene-referred linear.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Contrast = 1.05f;

    // Multiplicative tint of the dark regions.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _ShadowTint = FLinearColor(0.72f, 0.76f, 0.90f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _HighlightTint = FLinearColor(1.0f, 0.97f, 0.90f, 1.0f);

    // How far the pair of tints is applied. 0 leaves the paint's own colour.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _TintStrength = 0.35f;

    // The sky has no surface to draw on. Disabled, it is exempt from the paint AND the strokes — but NOT
    // from the ink: a silhouette against the sky is the drawing's most important contour. Paper still
    // covers it, because paper is the sheet the whole picture sits on. Caveat: the ink DISTANCE FADE
    // still applies out there, so the horizon contour survives only while _InkFadeEndDistance is 0.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AffectSky = ECk_EnableDisable::Disable;

    // Scene depth (uu) at or past which a pixel counts as sky.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1000.0, ClampMin = 1.0))
    float _SkyDistance = 100000.0f;

    // ---- Ink ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableInk = ECk_EnableDisable::Enable;

    // Drawn by BOTH the contours and the shadow strokes: hatching and outlines are the same medium in a
    // drawing, and a second colour would let the two disagree about what the pencil is.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _InkColor = FLinearColor(0.05f, 0.04f, 0.06f, 1.0f);

    // Detector tap radius in viewport pixels — the line's weight.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 1.0, UIMax = 8.0))
    float _InkThickness = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _InkOpacity = 0.9f;

    // Silhouettes and creases: the depth Laplacian normalized by depth, so the threshold is
    // distance-independent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.0001, UIMax = 1.0))
    float _DepthThreshold = 0.12f;

    // Form lines on a continuous surface: 1 - dot(N, TapN).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.0001, UIMax = 1.0))
    float _NormalThreshold = 0.25f;

    // Boundaries between two objects at the same depth and orientation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.0001, UIMax = 1.0))
    float _ColorEdgeThreshold = 0.12f;

    // How far, in ink thicknesses, the whole detector cluster may wander off the pixel grid. This is what
    // separates a drawn contour from a machined one; 0 gives a perfectly straight line.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 4.0))
    float _LineVariation = 1.2f;

    // Wavelength of that wander, in viewport pixels. Small values make the line ragged, large ones make
    // it curve slowly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 256.0))
    float _LineScale = 24.0f;

    // Distance (uu) at which the line starts thinning out.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0))
    float _InkFadeStartDistance = 0.0f;

    // Distance (uu) at which the line is gone. 0 disables the fade entirely; any other value must be
    // GREATER than the start — an inverted range collapses the ramp into a hard cutoff at this distance,
    // so the setting silently stops being a fade, and the subsystem rejects it rather than render that.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0))
    float _InkFadeEndDistance = 0.0f;

    // ---- Shadow strokes ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableShadowStrokes = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_HandDrawnStrokePattern _StrokePattern = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_HandDrawnStrokeSpace _StrokeSpace = ECk_Usf_HandDrawnStrokeSpace::ScreenStable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeStrength = 0.7f;

    // Normalized tone below which a pixel counts as shadow and starts accepting strokes. Higher values
    // hatch more of the picture.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeShadowThreshold = 0.35f;

    // Stroke cell size in viewport pixels — used only in ScreenStable space.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 64.0,
                      EditCondition = "_StrokeSpace == ECk_Usf_HandDrawnStrokeSpace::ScreenStable"))
    float _StrokePixelSize = 6.0f;

    // Stroke cell size in world units — used only in WorldAttached space.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 256.0,
                      EditCondition = "_StrokeSpace == ECk_Usf_HandDrawnStrokeSpace::WorldAttached"))
    float _StrokeWorldSize = 12.0f;

    // How far the ideal stroke lattice is allowed to wander — the difference between a printed screen and
    // a drawn one.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeIrregularity = 0.4f;

    // Planar-projection blend exponent for WorldAttached strokes: higher values make the three planes
    // meet more sharply at a corner instead of cross-fading over it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 16.0,
                      EditCondition = "_StrokeSpace == ECk_Usf_HandDrawnStrokeSpace::WorldAttached"))
    float _StrokeTriplanarSharpness = 4.0f;

    // ---- Paper ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnablePaper = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _GrainStrength = 0.12f;

    // Grain cell size in viewport pixels. The lattice is measured in output pixels, so the same scene at a
    // different output resolution gets a different number of grains across it — tune at target resolution
    // (the source feature's docs carry the same warning).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 32.0))
    float _GrainScale = 2.0f;

    // Directional fibre in the sheet — a low-frequency noise stretched along one axis, so it reads as a
    // grain DIRECTION rather than as a second speckle.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _FiberStrength = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _PaperWarmth = 0.5f;

    // ---- Debug ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_HandDrawn_DebugMode _DebugMode = ECk_Usf_HandDrawn_DebugMode::FinalImage;

public:
    CK_PROPERTY(_Enabled);
    CK_PROPERTY(_StyleStrength);
    CK_PROPERTY(_SimplifyColor);
    CK_PROPERTY(_ColorLevels);
    CK_PROPERTY(_ColorSoftness);
    CK_PROPERTY(_Saturation);
    CK_PROPERTY(_Contrast);
    CK_PROPERTY(_ShadowTint);
    CK_PROPERTY(_HighlightTint);
    CK_PROPERTY(_TintStrength);
    CK_PROPERTY(_AffectSky);
    CK_PROPERTY(_SkyDistance);
    CK_PROPERTY(_EnableInk);
    CK_PROPERTY(_InkColor);
    CK_PROPERTY(_InkThickness);
    CK_PROPERTY(_InkOpacity);
    CK_PROPERTY(_DepthThreshold);
    CK_PROPERTY(_NormalThreshold);
    CK_PROPERTY(_ColorEdgeThreshold);
    CK_PROPERTY(_LineVariation);
    CK_PROPERTY(_LineScale);
    CK_PROPERTY(_InkFadeStartDistance);
    CK_PROPERTY(_InkFadeEndDistance);
    CK_PROPERTY(_EnableShadowStrokes);
    CK_PROPERTY(_StrokePattern);
    CK_PROPERTY(_StrokeSpace);
    CK_PROPERTY(_StrokeStrength);
    CK_PROPERTY(_StrokeShadowThreshold);
    CK_PROPERTY(_StrokePixelSize);
    CK_PROPERTY(_StrokeWorldSize);
    CK_PROPERTY(_StrokeIrregularity);
    CK_PROPERTY(_StrokeTriplanarSharpness);
    CK_PROPERTY(_EnablePaper);
    CK_PROPERTY(_GrainStrength);
    CK_PROPERTY(_GrainScale);
    CK_PROPERTY(_FiberStrength);
    CK_PROPERTY(_PaperWarmth);
    CK_PROPERTY(_DebugMode);

public:
    // True when the ink distance fade is either off (end at 0) or ordered start-before-end. An inverted
    // range has no sensible reading: it collapses the ramp into a hard cutoff at the end distance, which
    // is not the setting anyone asked for.
    auto Get_InkFadeRangeIsOrdered() const -> bool;

public:
    auto operator==(const ThisType& InOther) const -> bool;
};
