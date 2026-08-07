#include "CkUsf/Stylize/CkUsf_ScreenDitherPreset.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_ScreenDitherPreset::
    Get_AsParams() const
    -> FCk_Usf_ScreenDither_Params
{
    auto Params = FCk_Usf_ScreenDither_Params{};

    Params.Set_Enabled(_Enabled)
          .Set_Pattern(_Pattern)
          .Set_PixelScale(_PixelScale)
          .Set_DitherStrength(_DitherStrength)
          .Set_Animate(_Animate)
          .Set_AnimationPeriod(_AnimationPeriod)
          .Set_BoxFilterDownsample(_BoxFilterDownsample)
          .Set_StabilizeGrid(_StabilizeGrid)
          .Set_PaletteMode(_PaletteMode)
          .Set_ColorSteps(_ColorSteps)
          .Set_Palette(_Palette)
          .Set_ColorSpace(_ColorSpace)
          .Set_PreGamma(_PreGamma)
          .Set_Monochrome(_Monochrome)
          .Set_MonochromeShadowTint(_MonochromeShadowTint)
          .Set_MonochromeHighlightTint(_MonochromeHighlightTint)
          .Set_Weight(_Weight)
          .Set_Saturation(_Saturation)
          .Set_Contrast(_Contrast)
          .Set_DebugMode(_DebugMode);

    return Params;
}

// --------------------------------------------------------------------------------------------------------------------
