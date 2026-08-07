#pragma once

#include "CoreMinimal.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkUsf_ScreenDither_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Threshold pattern the dither offset is sampled from. The order is a CONTRACT with
// CkUsf_Stylize_DitherThreshold's dispatcher in /CkUsf/StylizeCommon.ush — the subsystem writes the
// enum's integer value straight into the look's DitherPattern parameter.
UENUM(BlueprintType)
enum class ECk_Usf_DitherPattern : uint8
{
    Bayer2x2,
    Bayer4x4,
    Bayer8x8,
    InterleavedGradientNoise,
    WhiteNoise,
    BlueNoise
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_DitherPattern);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Usf_PaletteMode : uint8
{
    ColorSteps,     // posterize each channel to _ColorSteps levels
    CustomPalette   // snap to the nearest entry of _Palette (up to 8 colours)
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_PaletteMode);

// --------------------------------------------------------------------------------------------------------------------

// Which space the quantization steps are evenly spaced in. The look runs after tonemapping, so its
// input is display-encoded: `Gamma` quantizes those code values directly (what period hardware did),
// `Linear` undoes the transfer first so the steps are even in light and the shadows get fewer bands.
UENUM(BlueprintType)
enum class ECk_Usf_DitherColorSpace : uint8
{
    Gamma,
    Linear
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_DitherColorSpace);

// --------------------------------------------------------------------------------------------------------------------

// Order is a contract with ScreenDither.ush's DebugMode dispatcher.
UENUM(BlueprintType)
enum class ECk_Usf_ScreenDither_DebugMode : uint8
{
    Final,
    Pattern,                 // the raw threshold field
    QuantizationError,       // |value - quantized|, normalized to one step
    QuantizedWithoutDither,  // the same reduction with the threshold offset removed
    DownsampledInput         // the pixelation result, before any colour work
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_ScreenDither_DebugMode);

// --------------------------------------------------------------------------------------------------------------------

// Every runtime-drivable setting of the ScreenDither look, in one reflected value. The subsystem owns
// one of these and projects it onto the look's MID; a preset data asset is just an authored instance.
// Defaults ARE the "Balanced" preset — an authored preset that differs from these differs on purpose.
USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_ScreenDither_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Usf_ScreenDither_Params);

public:
    static constexpr int32 MaxPaletteEntries = 8;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_DitherPattern _Pattern = ECk_Usf_DitherPattern::Bayer4x4;

    // Block size in pixels. 1 = no pixelation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 32))
    float _PixelScale = 1.0f;

    // How far the pattern may push a value before quantization, in units of one quantization step.
    // 0 = plain banding; 1 = a full step, the classic ordered-dither look.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _DitherStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Animate = ECk_EnableDisable::Disable;

    // Seconds the pattern holds one phase. Smaller = faster crawl.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.01, ClampMin = 0.001,
                      EditCondition = "_Animate == ECk_EnableDisable::Enable"))
    float _AnimationPeriod = 0.1f;

    // Average the whole block instead of point-sampling its centre. Costs 4 taps; stops thin bright
    // detail from flickering as the camera moves.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _BoxFilterDownsample = ECk_EnableDisable::Disable;

    // Round the block to a WHOLE number of pixels. A fractional block size puts a different sub-pixel
    // phase in every block and crawls as the viewport resizes; an integral one tiles exactly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StabilizeGrid = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_PaletteMode _PaletteMode = ECk_Usf_PaletteMode::ColorSteps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2, ClampMin = 2, UIMax = 64,
                      EditCondition = "_PaletteMode == ECk_Usf_PaletteMode::ColorSteps"))
    int32 _ColorSteps = 8;

    // Entries past MaxPaletteEntries are ignored (the shader carries a fixed 8-wide palette). Must be
    // non-empty when _PaletteMode is CustomPalette — the subsystem rejects an empty one rather than
    // rendering the black every pixel would otherwise snap to.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_PaletteMode == ECk_Usf_PaletteMode::CustomPalette"))
    TArray<FLinearColor> _Palette;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_DitherColorSpace _ColorSpace = ECk_Usf_DitherColorSpace::Gamma;

    // Biases WHERE the bands land without changing overall brightness (it is inverted after
    // quantization). > 1 spends more steps on the shadows.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.1, ClampMin = 0.01, UIMax = 4.0))
    float _PreGamma = 1.0f;

    // Collapse to luminance before quantization, then remap the bands through the tint ramp below —
    // N tinted levels rather than N desaturated ones. Overrides CustomPalette when both are set.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Monochrome = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_Monochrome == ECk_EnableDisable::Enable"))
    FLinearColor _MonochromeShadowTint = FLinearColor(0.04f, 0.06f, 0.05f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_Monochrome == ECk_EnableDisable::Enable"))
    FLinearColor _MonochromeHighlightTint = FLinearColor(0.78f, 0.92f, 0.70f, 1.0f);

    // Blend of the reduced frame against the untouched original. 0 = the look is inert.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Saturation = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Contrast = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_ScreenDither_DebugMode _DebugMode = ECk_Usf_ScreenDither_DebugMode::Final;

public:
    CK_PROPERTY(_Enabled);
    CK_PROPERTY(_Pattern);
    CK_PROPERTY(_PixelScale);
    CK_PROPERTY(_DitherStrength);
    CK_PROPERTY(_Animate);
    CK_PROPERTY(_AnimationPeriod);
    CK_PROPERTY(_BoxFilterDownsample);
    CK_PROPERTY(_StabilizeGrid);
    CK_PROPERTY(_PaletteMode);
    CK_PROPERTY(_ColorSteps);
    CK_PROPERTY(_Palette);
    CK_PROPERTY(_ColorSpace);
    CK_PROPERTY(_PreGamma);
    CK_PROPERTY(_Monochrome);
    CK_PROPERTY(_MonochromeShadowTint);
    CK_PROPERTY(_MonochromeHighlightTint);
    CK_PROPERTY(_Weight);
    CK_PROPERTY(_Saturation);
    CK_PROPERTY(_Contrast);
    CK_PROPERTY(_DebugMode);

public:
    auto operator==(const ThisType& InOther) const -> bool;
};
