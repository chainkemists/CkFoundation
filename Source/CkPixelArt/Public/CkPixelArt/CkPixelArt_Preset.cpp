#include "CkPixelArt/CkPixelArt_Preset.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkPixelArt_Preset::
    Get_AsParams() const
    -> FCk_PixelArt_Params
{
    auto Look = FCk_PixelArt_LookParams{};
    Look.Set_EnableOutline(_EnableOutline);
    Look.Set_DepthThreshold(_DepthThreshold);
    Look.Set_AngleZCutoff(_AngleZCutoff);
    Look.Set_AngleZScale(_AngleZScale);
    Look.Set_NormalSmoothLow(_NormalSmoothLow);
    Look.Set_NormalSmoothHigh(_NormalSmoothHigh);
    Look.Set_LineDarken(_LineDarken);
    Look.Set_CreaseBrighten(_CreaseBrighten);
    Look.Set_EdgeMode(_EdgeMode);
    Look.Set_Bands(_Bands);
    Look.Set_ThresholdGradientSize(_ThresholdGradientSize);
    Look.Set_DitherStrength(_DitherStrength);
    Look.Set_ColorSpace(_ColorSpace);
    Look.Set_WarmShift(_WarmShift);
    Look.Set_PaletteMode(_PaletteMode);
    Look.Set_PaletteCount(_PaletteCount);
    Look.Set_PaletteColor0(_PaletteColor0);
    Look.Set_PaletteColor1(_PaletteColor1);
    Look.Set_PaletteColor2(_PaletteColor2);
    Look.Set_PaletteColor3(_PaletteColor3);
    Look.Set_PaletteColor4(_PaletteColor4);
    Look.Set_PaletteColor5(_PaletteColor5);
    Look.Set_PaletteColor6(_PaletteColor6);
    Look.Set_PaletteColor7(_PaletteColor7);

    auto Params = FCk_PixelArt_Params{};
    Params.Set_ResolutionMode(_ResolutionMode);
    Params.Set_InternalHeight(_InternalHeight);
    Params.Set_TexelsPerPixel(_TexelsPerPixel);
    Params.Set_MarginTexels(_MarginTexels);
    Params.Set_SnapEnabled(_SnapEnabled);
    Params.Set_UpscaleFilter(_UpscaleFilter);
    Params.Set_ApplyLook(_ApplyLook);
    Params.Set_Look(Look);

    return Params;
}
