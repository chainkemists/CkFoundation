#pragma once

#include "CoreMinimal.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkUsf_CelShade_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Halftone pattern drawn across a band transition. The order is a DOUBLE contract: it indexes
// CkUsf_Stylize_HalftonePattern's dispatcher in /CkUsf/StylizeCommon.ush, and it is the per-object
// Custom-Stencil mapping (StencilBase + this value selects the pattern on that mesh). Reordering it
// silently re-skins every stencil-tagged mesh in every level.
UENUM(BlueprintType)
enum class ECk_Usf_CelPattern : uint8
{
    Bayer,
    RoundDots,
    SquareDots,
    Lines,
    Crosshatch,
    DiagonalLines,
    ConcentricCircles,
    Triangles,
    ClusteredNoise,
    Spiral
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CelPattern);

// --------------------------------------------------------------------------------------------------------------------

// Where the pattern cells live. World attaches them to the surface, so they hold still under camera
// motion — but they SLIDE on a translating or skinned mesh, because a post-process pass has no frame
// history to correct with. Screen is camera-aligned and always stable, at the cost of the cells
// swimming across a surface as the camera moves.
UENUM(BlueprintType)
enum class ECk_Usf_CelPatternSpace : uint8
{
    World,
    Screen
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CelPatternSpace);

// --------------------------------------------------------------------------------------------------------------------

// Tap count of the outline's edge detectors. Order is a contract with CelShade.ush.
UENUM(BlueprintType)
enum class ECk_Usf_CelOutlineQuality : uint8
{
    FourTap,   // axis-aligned only — enough for the depth Laplacian
    EightTap   // adds the diagonals; matters on edges running diagonally through a pixel
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CelOutlineQuality);

// --------------------------------------------------------------------------------------------------------------------

// How the ink line is combined with the shaded frame. Order is a contract with CelShade.ush.
UENUM(BlueprintType)
enum class ECk_Usf_CelOutlineBlend : uint8
{
    Blend,      // lerp toward the outline colour — an opaque ink line
    Multiply,   // darken by the outline colour, so lit surfaces keep their own hue under the line
    Additive    // add the outline colour, for glowing edges
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CelOutlineBlend);

// --------------------------------------------------------------------------------------------------------------------

// Order is a contract with CelShade.ush's DebugMode dispatcher. MotionOffset renders BLACK on purpose:
// moving-object pattern stabilization needs a frame history a post-process blendable does not have, so
// there is no offset to visualize — the mode exists to say so rather than to be quietly missing.
UENUM(BlueprintType)
enum class ECk_Usf_CelShade_DebugMode : uint8
{
    Final,
    BandIndex,
    OutlineMask,
    Illumination,
    Albedo,
    Normals,
    PatternThreshold,
    PatternCoordinates,
    MotionOffset,
    Stencil
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CelShade_DebugMode);

// --------------------------------------------------------------------------------------------------------------------

// Every runtime-drivable setting of the CelShade look, in one reflected value. The subsystem owns one of
// these and projects it onto the look's MID; a preset data asset is just an authored instance.
// Defaults ARE the "Balanced" preset — an authored preset that differs from these differs on purpose.
USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_CelShade_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Usf_CelShade_Params);

public:
    // How many Custom-Stencil values above the base the pattern contract owns — one per ECk_Usf_CelPattern
    // entry. With the suppress value at base-1 the whole reserved span is [base-1, base+PatternSlots-1].
    static constexpr int32 PatternSlots = 10;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    // ---- Bands ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _Bands = 4;

    // The illumination value that lands in the MIDDLE of the band ramp. Pre-tonemap light is unbounded,
    // so this is what anchors the ramp to the scene's actual exposure — it is the first knob to tune.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.05, ClampMin = 0.01, UIMax = 4.0))
    float _Midpoint = 0.5f;

    // Slides every band boundary along the ramp without changing how many there are.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = -1.0, ClampMin = -1.0, UIMax = 1.0, ClampMax = 1.0))
    float _BandOffset = 0.0f;

    // Exponent on the ramp: > 1 spends more bands on the shadows, < 1 on the highlights.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.1, ClampMin = 0.01, UIMax = 4.0))
    float _Distribution = 1.0f;

    // Width of the smooth ramp between two bands when the pattern is off. 0 = hard cel edges.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _BandSoftness = 0.0f;

    // Raises the DARKEST band only, so shadows read as coloured rather than black; the lit side is
    // untouched.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _ShadowLift = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _Strength = 1.0f;

    // Band the luminance of the FINAL scene colour instead of the reconstructed illumination. The
    // reconstruction (SceneColor / BaseColor) is the only lighting a blendable can approximate; this is
    // the fallback for scenes where it reads as albedo-driven rather than light-driven.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _QuantizeFinalColor = ECk_EnableDisable::Disable;

    // ---- Pattern ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnablePattern = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_CelPattern _Pattern = ECk_Usf_CelPattern::RoundDots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_CelPatternSpace _PatternSpace = ECk_Usf_CelPatternSpace::World;

    // How much of the band transition the pattern owns. 0 = the transition falls back to Band Softness.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _PatternStrength = 1.0f;

    // Steepens the pattern's threshold ramp: 1 = the raw pattern, higher approaches a binary stencil.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.1, ClampMin = 0.01, UIMax = 8.0))
    float _PatternContrast = 1.0f;

    // Cell size in world units (World space).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 200.0,
                      EditCondition = "_PatternSpace == ECk_Usf_CelPatternSpace::World"))
    float _PatternWorldSize = 16.0f;

    // Cell size in pixels (Screen space).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.01, UIMax = 64.0,
                      EditCondition = "_PatternSpace == ECk_Usf_CelPatternSpace::Screen"))
    float _PatternPixelSize = 6.0f;

    // How sharply the three triplanar projections give way to each other at a corner.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 0.1, UIMax = 16.0,
                      EditCondition = "_PatternSpace == ECk_Usf_CelPatternSpace::World"))
    float _TriplanarSharpness = 4.0f;

    // 0 = cells keep their world size and shrink on screen with distance; 1 = the cell size steps up by
    // whole octaves with distance, so density stays roughly constant.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_PatternSpace == ECk_Usf_CelPatternSpace::World"))
    float _PatternDistanceScaling = 1.0f;

    // Octave range the distance scaling may pick from, in powers of two around the authored cell size.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = -4.0, ClampMin = -8.0, UIMax = 8.0,
                      EditCondition = "_PatternSpace == ECk_Usf_CelPatternSpace::World"))
    float _PatternOctaveMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = -4.0, ClampMin = -8.0, UIMax = 8.0,
                      EditCondition = "_PatternSpace == ECk_Usf_CelPatternSpace::World"))
    float _PatternOctaveMax = 4.0f;

    // Pattern-space units per second. Non-zero turns a printed screen into a crawling one.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 32.0))
    float _PatternScrollSpeed = 0.0f;

    // ---- Colour ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _ShadowTint = FLinearColor(0.72f, 0.78f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _LightTint = FLinearColor(1.0f, 0.98f, 0.92f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Saturation = 1.0f;

    // Floor on the albedo the illumination reconstruction divides by. Too low and a near-black surface
    // turns the quotient into amplified noise; too high and dark materials stop banding at all.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.001, ClampMin = 0.001, UIMax = 0.5))
    float _MinimumAlbedo = 0.04f;

    // Whether pixels with no diffuse response (unlit / emissive) are shaded at all. Off leaves them
    // exactly as the scene rendered them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AffectUnlit = ECk_EnableDisable::Disable;

    // ---- Sky ----

    // The sky has no GBuffer albedo, so it can never go through the illumination reconstruction — this
    // group bands its scene luminance directly, with its own ramp.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableSky = ECk_EnableDisable::Enable;

    // Scene depth (uu) past which a pixel counts as sky.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1000.0, ClampMin = 1.0,
                      EditCondition = "_EnableSky == ECk_EnableDisable::Enable"))
    float _SkyDistance = 100000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 16,
                      EditCondition = "_EnableSky == ECk_EnableDisable::Enable"))
    int32 _SkyBands = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableSky == ECk_EnableDisable::Enable"))
    float _SkyStrength = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_EnableSky == ECk_EnableDisable::Enable"))
    ECk_Usf_CelPattern _SkyPattern = ECk_Usf_CelPattern::Bayer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableSky == ECk_EnableDisable::Enable"))
    float _SkyPatternStrength = 0.4f;

    // Multiplies the global cell size for sky pixels only — sky is far away, so it usually wants
    // coarser cells than the subject.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.1, ClampMin = 0.01, UIMax = 16.0,
                      EditCondition = "_EnableSky == ECk_EnableDisable::Enable"))
    float _SkyPatternScale = 2.0f;

    // ---- Metallic ----

    // Metals get their light from reflections, not from albedo, so the illumination reconstruction is
    // worst exactly there. This group is how they stay legible instead of being tuned around.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _MetallicThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _MetallicBands = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _MetallicStrength = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _MetallicPatternStrength = 0.3f;

    // ---- Specular ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableSpecular = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 8,
                      EditCondition = "_EnableSpecular == ECk_EnableDisable::Enable"))
    int32 _SpecularSteps = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableSpecular == ECk_EnableDisable::Enable"))
    float _SpecularThreshold = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 4.0,
                      EditCondition = "_EnableSpecular == ECk_EnableDisable::Enable"))
    float _SpecularIntensity = 0.6f;

    // Rougher than this and the surface gets no stylized highlight — a matte object with a hard
    // specular step reads as a rendering error, not as a style.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableSpecular == ECk_EnableDisable::Enable"))
    float _SpecularRoughnessCutoff = 0.4f;

    // ---- Rim ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableRimLight = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.5, ClampMin = 0.01, UIMax = 16.0,
                      EditCondition = "_EnableRimLight == ECk_EnableDisable::Enable"))
    float _RimPower = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableRimLight == ECk_EnableDisable::Enable"))
    float _RimThreshold = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.001, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableRimLight == ECk_EnableDisable::Enable"))
    float _RimSoftness = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 4.0,
                      EditCondition = "_EnableRimLight == ECk_EnableDisable::Enable"))
    float _RimIntensity = 0.5f;

    // How much the rim is gated by the pixel's own band level. 1 keeps it off shadowed surfaces, which
    // is what stops it reading as a uniform outline glow.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableRimLight == ECk_EnableDisable::Enable"))
    float _RimFollowsLighting = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_EnableRimLight == ECk_EnableDisable::Enable"))
    FLinearColor _RimColor = FLinearColor(1.0f, 0.95f, 0.85f, 1.0f);

    // ---- Outline ----

    // The look's OWN ink line (depth / normal / albedo discontinuities). Unrelated to the Custom-Stencil
    // silhouettes of UCkUsf_OutlineSubsystem, which mark chosen objects rather than every edge.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableOutline = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1.0, ClampMin = 1.0, UIMax = 8.0,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    float _OutlineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    ECk_Usf_CelOutlineQuality _OutlineQuality = ECk_Usf_CelOutlineQuality::FourTap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    FLinearColor _OutlineColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    ECk_Usf_CelOutlineBlend _OutlineBlendMode = ECk_Usf_CelOutlineBlend::Blend;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    float _OutlineOpacity = 1.0f;

    // Relative depth Laplacian above which a pixel is a silhouette or crease.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.001, ClampMin = 0.0001, UIMax = 2.0,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    float _OutlineDepthThreshold = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.001, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    float _OutlineNormalThreshold = 0.25f;

    // Albedo distance across which a material change alone draws a line — the detector the pure
    // depth/normal outline has no way to see.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.001, UIMax = 2.0,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    float _OutlineAlbedoThreshold = 0.35f;

    // Scene depth (uu) at which the line has faded out completely. <= 1 disables the fade.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0,
                      EditCondition = "_EnableOutline == ECk_EnableDisable::Enable"))
    float _OutlineDistanceFade = 0.0f;

    // ---- Per-object stencil ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableStencilPatterns = ECk_EnableDisable::Enable;

    // First Custom-Stencil value of the pattern contract: base + N selects ECk_Usf_CelPattern N, and
    // base - 1 suppresses transitions. The whole span must stay clear of UCkUsf_OutlineSubsystem's
    // range — the CelShade subsystem rejects a colliding base rather than letting one feature's meshes
    // silently restyle under the other.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 245,
                      EditCondition = "_EnableStencilPatterns == ECk_EnableDisable::Enable"))
    int32 _StencilBase = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_CelShade_DebugMode _DebugMode = ECk_Usf_CelShade_DebugMode::Final;

public:
    CK_PROPERTY(_Enabled);
    CK_PROPERTY(_Bands);
    CK_PROPERTY(_Midpoint);
    CK_PROPERTY(_BandOffset);
    CK_PROPERTY(_Distribution);
    CK_PROPERTY(_BandSoftness);
    CK_PROPERTY(_ShadowLift);
    CK_PROPERTY(_Strength);
    CK_PROPERTY(_QuantizeFinalColor);
    CK_PROPERTY(_EnablePattern);
    CK_PROPERTY(_Pattern);
    CK_PROPERTY(_PatternSpace);
    CK_PROPERTY(_PatternStrength);
    CK_PROPERTY(_PatternContrast);
    CK_PROPERTY(_PatternWorldSize);
    CK_PROPERTY(_PatternPixelSize);
    CK_PROPERTY(_TriplanarSharpness);
    CK_PROPERTY(_PatternDistanceScaling);
    CK_PROPERTY(_PatternOctaveMin);
    CK_PROPERTY(_PatternOctaveMax);
    CK_PROPERTY(_PatternScrollSpeed);
    CK_PROPERTY(_ShadowTint);
    CK_PROPERTY(_LightTint);
    CK_PROPERTY(_Saturation);
    CK_PROPERTY(_MinimumAlbedo);
    CK_PROPERTY(_AffectUnlit);
    CK_PROPERTY(_EnableSky);
    CK_PROPERTY(_SkyDistance);
    CK_PROPERTY(_SkyBands);
    CK_PROPERTY(_SkyStrength);
    CK_PROPERTY(_SkyPattern);
    CK_PROPERTY(_SkyPatternStrength);
    CK_PROPERTY(_SkyPatternScale);
    CK_PROPERTY(_MetallicThreshold);
    CK_PROPERTY(_MetallicBands);
    CK_PROPERTY(_MetallicStrength);
    CK_PROPERTY(_MetallicPatternStrength);
    CK_PROPERTY(_EnableSpecular);
    CK_PROPERTY(_SpecularSteps);
    CK_PROPERTY(_SpecularThreshold);
    CK_PROPERTY(_SpecularIntensity);
    CK_PROPERTY(_SpecularRoughnessCutoff);
    CK_PROPERTY(_EnableRimLight);
    CK_PROPERTY(_RimPower);
    CK_PROPERTY(_RimThreshold);
    CK_PROPERTY(_RimSoftness);
    CK_PROPERTY(_RimIntensity);
    CK_PROPERTY(_RimFollowsLighting);
    CK_PROPERTY(_RimColor);
    CK_PROPERTY(_EnableOutline);
    CK_PROPERTY(_OutlineThickness);
    CK_PROPERTY(_OutlineQuality);
    CK_PROPERTY(_OutlineColor);
    CK_PROPERTY(_OutlineBlendMode);
    CK_PROPERTY(_OutlineOpacity);
    CK_PROPERTY(_OutlineDepthThreshold);
    CK_PROPERTY(_OutlineNormalThreshold);
    CK_PROPERTY(_OutlineAlbedoThreshold);
    CK_PROPERTY(_OutlineDistanceFade);
    CK_PROPERTY(_EnableStencilPatterns);
    CK_PROPERTY(_StencilBase);
    CK_PROPERTY(_DebugMode);

public:
    // Lowest Custom-Stencil value the pattern contract claims — the suppress value, one below the base.
    auto Get_StencilRangeMin() const -> int32;

    // Highest Custom-Stencil value the pattern contract claims — the last selectable pattern.
    auto Get_StencilRangeMax() const -> int32;

public:
    auto operator==(const ThisType& InOther) const -> bool;
};
