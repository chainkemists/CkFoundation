#include "CkUsf/Stylize/CkUsf_HandDrawn_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Usf_HandDrawn_Params::
    Get_InkFadeRangeIsOrdered() const
    -> bool
{
    if (_InkFadeEndDistance <= 0.0f)
    { return true; }

    return _InkFadeEndDistance > _InkFadeStartDistance;
}

auto
    FCk_Usf_HandDrawn_Params::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _Enabled == InOther._Enabled
        && _StyleStrength == InOther._StyleStrength
        && _SimplifyColor == InOther._SimplifyColor
        && _ColorLevels == InOther._ColorLevels
        && _ColorSoftness == InOther._ColorSoftness
        && _Saturation == InOther._Saturation
        && _Contrast == InOther._Contrast
        && _ShadowTint == InOther._ShadowTint
        && _HighlightTint == InOther._HighlightTint
        && _TintStrength == InOther._TintStrength
        && _AffectSky == InOther._AffectSky
        && _SkyDistance == InOther._SkyDistance
        && _EnableInk == InOther._EnableInk
        && _InkColor == InOther._InkColor
        && _InkThickness == InOther._InkThickness
        && _InkOpacity == InOther._InkOpacity
        && _DepthThreshold == InOther._DepthThreshold
        && _NormalThreshold == InOther._NormalThreshold
        && _ColorEdgeThreshold == InOther._ColorEdgeThreshold
        && _LineVariation == InOther._LineVariation
        && _LineScale == InOther._LineScale
        && _InkFadeStartDistance == InOther._InkFadeStartDistance
        && _InkFadeEndDistance == InOther._InkFadeEndDistance
        && _EnableShadowStrokes == InOther._EnableShadowStrokes
        && _StrokePattern == InOther._StrokePattern
        && _StrokeSpace == InOther._StrokeSpace
        && _StrokeStrength == InOther._StrokeStrength
        && _StrokeShadowThreshold == InOther._StrokeShadowThreshold
        && _StrokePixelSize == InOther._StrokePixelSize
        && _StrokeWorldSize == InOther._StrokeWorldSize
        && _StrokeIrregularity == InOther._StrokeIrregularity
        && _StrokeTriplanarSharpness == InOther._StrokeTriplanarSharpness
        && _EnablePaper == InOther._EnablePaper
        && _GrainStrength == InOther._GrainStrength
        && _GrainScale == InOther._GrainScale
        && _FiberStrength == InOther._FiberStrength
        && _PaperWarmth == InOther._PaperWarmth
        && _DebugMode == InOther._DebugMode;
}

// --------------------------------------------------------------------------------------------------------------------
