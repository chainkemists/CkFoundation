#include "CkUsf/Stylize/CkUsf_HandDrawnPreset.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_HandDrawnPreset::
    Get_AsParams() const
    -> FCk_Usf_HandDrawn_Params
{
    auto Params = FCk_Usf_HandDrawn_Params{};

    Params.Set_Enabled(_Enabled)
          .Set_StyleStrength(_StyleStrength)
          .Set_SimplifyColor(_SimplifyColor)
          .Set_ColorLevels(_ColorLevels)
          .Set_ColorSoftness(_ColorSoftness)
          .Set_Saturation(_Saturation)
          .Set_Contrast(_Contrast)
          .Set_ShadowTint(_ShadowTint)
          .Set_HighlightTint(_HighlightTint)
          .Set_TintStrength(_TintStrength)
          .Set_AffectSky(_AffectSky)
          .Set_SkyDistance(_SkyDistance)
          .Set_EnableInk(_EnableInk)
          .Set_InkColor(_InkColor)
          .Set_InkThickness(_InkThickness)
          .Set_InkOpacity(_InkOpacity)
          .Set_DepthThreshold(_DepthThreshold)
          .Set_NormalThreshold(_NormalThreshold)
          .Set_ColorEdgeThreshold(_ColorEdgeThreshold)
          .Set_LineVariation(_LineVariation)
          .Set_LineScale(_LineScale)
          .Set_InkFadeStartDistance(_InkFadeStartDistance)
          .Set_InkFadeEndDistance(_InkFadeEndDistance)
          .Set_EnableShadowStrokes(_EnableShadowStrokes)
          .Set_StrokePattern(_StrokePattern)
          .Set_StrokeSpace(_StrokeSpace)
          .Set_StrokeStrength(_StrokeStrength)
          .Set_StrokeShadowThreshold(_StrokeShadowThreshold)
          .Set_StrokePixelSize(_StrokePixelSize)
          .Set_StrokeWorldSize(_StrokeWorldSize)
          .Set_StrokeIrregularity(_StrokeIrregularity)
          .Set_StrokeTriplanarSharpness(_StrokeTriplanarSharpness)
          .Set_EnablePaper(_EnablePaper)
          .Set_GrainStrength(_GrainStrength)
          .Set_GrainScale(_GrainScale)
          .Set_FiberStrength(_FiberStrength)
          .Set_PaperWarmth(_PaperWarmth)
          .Set_Mask(_Mask)
          .Set_DebugMode(_DebugMode);

    return Params;
}

// --------------------------------------------------------------------------------------------------------------------
