#pragma once

#include "CoreMinimal.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPixelArtRender/CkPixelArtRender_State.h"

#include "CkPixelArt_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How the outline pass paints the edges it finds.
//
// FlatColors is the conventional answer: darken silhouettes, brighten creases, by a fixed multiplier. BandShift
// is the t3ssel8r signature — an edge moves the pixel one step along the QUANTIZED ladder instead of tinting it,
// so the outline is always a colour the palette already contains and the image never grows a colour the artist
// did not choose.
UENUM(BlueprintType)
enum class ECk_PixelArt_EdgeMode : uint8
{
    FlatColors,
    BandShift
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PixelArt_EdgeMode);

// --------------------------------------------------------------------------------------------------------------------

// How colour is reduced after banding. Order is a contract with PixelArt.ush — the shader receives the index.
UENUM(BlueprintType)
enum class ECk_PixelArt_PaletteMode : uint8
{
    // Quantize each channel to a fixed number of steps. Cheap, and the classic look.
    ColorSteps,

    // Snap to the nearest entry of the authored palette. The only mode where the output is guaranteed to
    // contain nothing but the artist's colours.
    CustomPalette,

    // Quantize luminance only, leaving hue alone.
    LuminanceSteps
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PixelArt_PaletteMode);

// --------------------------------------------------------------------------------------------------------------------

// The look half of the pixel-art configuration: outlines, banding and palette, applied at the internal
// resolution so a one-texel line is one texel on screen.
//
// Declared in full here even though the shader that consumes it lands later, so that the params struct callers
// author against does not change shape underneath them.
USTRUCT(BlueprintType)
struct CKPIXELART_API FCk_PixelArt_LookParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_PixelArt_LookParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableOutline = ECk_EnableDisable::Enable;

    // Depth difference, relative to the sample's own depth, that counts as a silhouette.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline",
              meta = (AllowPrivateAccess = true, UIMin = 0.001, ClampMin = 0.0001, UIMax = 2.0))
    float _DepthThreshold = 0.15f;

    // A surface seen edge-on has a huge depth gradient without any actual edge, which is what makes a flat
    // ground plane grow a staircase of false outlines. These two scale the threshold with how grazing the
    // view is: below the cutoff nothing changes, above it the threshold grows by up to the scale.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _AngleZCutoff = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 64.0))
    float _AngleZScale = 24.0f;

    // Normal-contrast window for creases: below Low nothing is an edge, above High everything is, and the
    // range between is the soft ramp that keeps a crease one texel wide instead of two.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _NormalSmoothLow = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _NormalSmoothHigh = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edges",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _LineDarken = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edges",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _CreaseBrighten = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edges",
              meta = (AllowPrivateAccess = true))
    ECk_PixelArt_EdgeMode _EdgeMode = ECk_PixelArt_EdgeMode::BandShift;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banding",
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _Bands = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banding",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _ThresholdGradientSize = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banding",
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _DitherStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    ECk_PixelArt_PaletteMode _PaletteMode = ECk_PixelArt_PaletteMode::ColorSteps;

    // Steps per channel, palette entries used, or luminance steps, depending on the mode above. Capped at 8
    // in CustomPalette because that is how many colour slots the shader carries.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 32))
    int32 _PaletteCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor0 = FLinearColor{0.05f, 0.05f, 0.09f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor1 = FLinearColor{0.14f, 0.12f, 0.24f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor2 = FLinearColor{0.28f, 0.20f, 0.33f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor3 = FLinearColor{0.45f, 0.29f, 0.35f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor4 = FLinearColor{0.63f, 0.42f, 0.36f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor5 = FLinearColor{0.80f, 0.59f, 0.42f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor6 = FLinearColor{0.92f, 0.78f, 0.57f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette",
              meta = (AllowPrivateAccess = true))
    FLinearColor _PaletteColor7 = FLinearColor{0.99f, 0.95f, 0.85f, 1.0f};

public:
    CK_PROPERTY(_EnableOutline);
    CK_PROPERTY(_DepthThreshold);
    CK_PROPERTY(_AngleZCutoff);
    CK_PROPERTY(_AngleZScale);
    CK_PROPERTY(_NormalSmoothLow);
    CK_PROPERTY(_NormalSmoothHigh);
    CK_PROPERTY(_LineDarken);
    CK_PROPERTY(_CreaseBrighten);
    CK_PROPERTY(_EdgeMode);
    CK_PROPERTY(_Bands);
    CK_PROPERTY(_ThresholdGradientSize);
    CK_PROPERTY(_DitherStrength);
    CK_PROPERTY(_PaletteMode);
    CK_PROPERTY(_PaletteCount);
    CK_PROPERTY(_PaletteColor0);
    CK_PROPERTY(_PaletteColor1);
    CK_PROPERTY(_PaletteColor2);
    CK_PROPERTY(_PaletteColor3);
    CK_PROPERTY(_PaletteColor4);
    CK_PROPERTY(_PaletteColor5);
    CK_PROPERTY(_PaletteColor6);
    CK_PROPERTY(_PaletteColor7);

};

// --------------------------------------------------------------------------------------------------------------------

// Everything one world's pixel-art rendering is configured by.
//
// Two halves that are deliberately independent: the RENDERER (what resolution the scene rasterizes at, whether
// the camera snaps) and the LOOK (outlines, banding, palette). Either works without the other — the look is an
// ordinary full-resolution stylization with the renderer off, and the renderer is a plain sharp downscale with
// the look off. Pairing them is composition, not coupling.
USTRUCT(BlueprintType)
struct CKPIXELART_API FCk_PixelArt_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_PixelArt_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Renderer",
              meta = (AllowPrivateAccess = true))
    ECk_PixelArt_ResolutionMode _ResolutionMode = ECk_PixelArt_ResolutionMode::FixedHeight;

    // Vertical texel count when the mode is FixedHeight — the "360p" knob. The width follows from the viewport
    // aspect, because the renderer can only make one axis land exactly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Renderer",
              meta = (AllowPrivateAccess = true, UIMin = 90, ClampMin = 90, UIMax = 1080, ClampMax = 1080,
                      EditCondition = "_ResolutionMode == ECk_PixelArt_ResolutionMode::FixedHeight"))
    int32 _InternalHeight = 360;

    // Output pixels per texel when the mode is TexelsPerPixel. Keeps the texel grid a whole number of screen
    // pixels at any window size, at the cost of the internal resolution changing when the window does.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Renderer",
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 16, ClampMax = 16,
                      EditCondition = "_ResolutionMode == ECk_PixelArt_ResolutionMode::TexelsPerPixel"))
    int32 _TexelsPerPixel = 4;

    // Texels rendered beyond the displayed window on each side. The camera's sub-texel snap remainder is
    // re-applied by shifting the sampling window, and without margin that shift reads texels that were never
    // rendered. 0 disables it and smears the borders.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Renderer",
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0, UIMax = 8, ClampMax = 8))
    int32 _MarginTexels = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Renderer",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _SnapEnabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Renderer",
              meta = (AllowPrivateAccess = true))
    ECk_PixelArt_UpscaleFilter _UpscaleFilter = ECk_PixelArt_UpscaleFilter::BoxFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _ApplyLook = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look",
              meta = (AllowPrivateAccess = true))
    FCk_PixelArt_LookParams _Look;

public:
    CK_PROPERTY(_ResolutionMode);
    CK_PROPERTY(_InternalHeight);
    CK_PROPERTY(_TexelsPerPixel);
    CK_PROPERTY(_MarginTexels);
    CK_PROPERTY(_SnapEnabled);
    CK_PROPERTY(_UpscaleFilter);
    CK_PROPERTY(_ApplyLook);
    CK_PROPERTY(_Look);

};

// --------------------------------------------------------------------------------------------------------------------

// One engine setting that stands between the configuration and a pixel-art image.
//
// It carries the fix as data rather than as prose because the fix is the point: a caller that cannot render
// needs to know which console variable to move and to what, and a log line saying "preconditions failed" makes
// the reader go and find that out themselves.
USTRUCT(BlueprintType)
struct CKPIXELART_API FCk_PixelArt_PreconditionFailure
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_PixelArt_PreconditionFailure);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Name;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _CurrentValue;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _RequiredValue;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _FixCVar;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_CurrentValue);
    CK_PROPERTY_GET(_RequiredValue);
    CK_PROPERTY_GET(_FixCVar);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_PixelArt_PreconditionFailure, _Name, _CurrentValue, _RequiredValue, _FixCVar);
};

// --------------------------------------------------------------------------------------------------------------------

// Why the renderer will not run, or empty when nothing is in its way.
//
// Empty is the ONLY meaning of "ready" — the report is never a warning list alongside a successful enable, so a
// caller can branch on emptiness without also having to know which entries are fatal.
USTRUCT(BlueprintType)
struct CKPIXELART_API FCk_PixelArt_PreconditionReport
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_PixelArt_PreconditionReport);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_PixelArt_PreconditionFailure> _Failures;

public:
    CK_PROPERTY(_Failures);


public:
    auto Get_IsSatisfied() const -> bool { return _Failures.IsEmpty(); }

    auto ToText() const -> FString;
};
