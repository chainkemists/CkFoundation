#include "CkUsf/Stylize/CkUsf_ScreenDither_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Usf_ScreenDither_Params::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _Enabled == InOther._Enabled
        && _Pattern == InOther._Pattern
        && _PixelScale == InOther._PixelScale
        && _DitherStrength == InOther._DitherStrength
        && _Animate == InOther._Animate
        && _AnimationPeriod == InOther._AnimationPeriod
        && _BoxFilterDownsample == InOther._BoxFilterDownsample
        && _StabilizeGrid == InOther._StabilizeGrid
        && _PaletteMode == InOther._PaletteMode
        && _ColorSteps == InOther._ColorSteps
        && _Palette == InOther._Palette
        && _ColorSpace == InOther._ColorSpace
        && _PreGamma == InOther._PreGamma
        && _Monochrome == InOther._Monochrome
        && _MonochromeShadowTint == InOther._MonochromeShadowTint
        && _MonochromeHighlightTint == InOther._MonochromeHighlightTint
        && _Weight == InOther._Weight
        && _Saturation == InOther._Saturation
        && _Contrast == InOther._Contrast
        && _DebugMode == InOther._DebugMode;
}

// --------------------------------------------------------------------------------------------------------------------
