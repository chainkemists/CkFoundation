#include "CkUsf/Stylize/CkUsf_CrossHatch_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Usf_CrossHatch_Params::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _Enabled == InOther._Enabled
        && _StyleStrength == InOther._StyleStrength
        && _UseWorldSpaceNormals == InOther._UseWorldSpaceNormals
        && _AngleOffset == InOther._AngleOffset
        && _NormalAlignment == InOther._NormalAlignment
        && _Spacing == InOther._Spacing
        && _LayerCount == InOther._LayerCount
        && _LayerAngleStep == InOther._LayerAngleStep
        && _StrokePattern == InOther._StrokePattern
        && _StrokeThickness == InOther._StrokeThickness
        && _StrokeIrregularity == InOther._StrokeIrregularity
        && _DarknessBias == InOther._DarknessBias
        && _DarknessContrast == InOther._DarknessContrast
        && _BackgroundMode == InOther._BackgroundMode
        && _PaperColor == InOther._PaperColor
        && _InkColor == InOther._InkColor
        && _Saturation == InOther._Saturation
        && _AffectSky == InOther._AffectSky
        && _SkyDistance == InOther._SkyDistance
        && _Mask == InOther._Mask
        && _DebugMode == InOther._DebugMode;
}

// --------------------------------------------------------------------------------------------------------------------
