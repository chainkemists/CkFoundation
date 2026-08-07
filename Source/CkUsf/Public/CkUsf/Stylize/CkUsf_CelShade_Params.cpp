#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Usf_CelShade_Params::
    Get_StencilRangeMin() const
    -> int32
{
    return _StencilBase - 1;
}

auto
    FCk_Usf_CelShade_Params::
    Get_StencilRangeMax() const
    -> int32
{
    return _StencilBase + PatternSlots - 1;
}

auto
    FCk_Usf_CelShade_Params::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _Enabled == InOther._Enabled
        && _Bands == InOther._Bands
        && _Midpoint == InOther._Midpoint
        && _BandOffset == InOther._BandOffset
        && _Distribution == InOther._Distribution
        && _BandSoftness == InOther._BandSoftness
        && _ShadowLift == InOther._ShadowLift
        && _Strength == InOther._Strength
        && _QuantizeFinalColor == InOther._QuantizeFinalColor
        && _EnablePattern == InOther._EnablePattern
        && _Pattern == InOther._Pattern
        && _PatternSpace == InOther._PatternSpace
        && _PatternStrength == InOther._PatternStrength
        && _PatternContrast == InOther._PatternContrast
        && _PatternWorldSize == InOther._PatternWorldSize
        && _PatternPixelSize == InOther._PatternPixelSize
        && _TriplanarSharpness == InOther._TriplanarSharpness
        && _PatternDistanceScaling == InOther._PatternDistanceScaling
        && _PatternOctaveMin == InOther._PatternOctaveMin
        && _PatternOctaveMax == InOther._PatternOctaveMax
        && _PatternScrollSpeed == InOther._PatternScrollSpeed
        && _ShadowTint == InOther._ShadowTint
        && _LightTint == InOther._LightTint
        && _Saturation == InOther._Saturation
        && _MinimumAlbedo == InOther._MinimumAlbedo
        && _AffectUnlit == InOther._AffectUnlit
        && _EnableSky == InOther._EnableSky
        && _SkyDistance == InOther._SkyDistance
        && _SkyBands == InOther._SkyBands
        && _SkyStrength == InOther._SkyStrength
        && _SkyPattern == InOther._SkyPattern
        && _SkyPatternStrength == InOther._SkyPatternStrength
        && _SkyPatternScale == InOther._SkyPatternScale
        && _MetallicThreshold == InOther._MetallicThreshold
        && _MetallicBands == InOther._MetallicBands
        && _MetallicStrength == InOther._MetallicStrength
        && _MetallicPatternStrength == InOther._MetallicPatternStrength
        && _EnableSpecular == InOther._EnableSpecular
        && _SpecularSteps == InOther._SpecularSteps
        && _SpecularThreshold == InOther._SpecularThreshold
        && _SpecularIntensity == InOther._SpecularIntensity
        && _SpecularRoughnessCutoff == InOther._SpecularRoughnessCutoff
        && _EnableRimLight == InOther._EnableRimLight
        && _RimPower == InOther._RimPower
        && _RimThreshold == InOther._RimThreshold
        && _RimSoftness == InOther._RimSoftness
        && _RimIntensity == InOther._RimIntensity
        && _RimFollowsLighting == InOther._RimFollowsLighting
        && _RimColor == InOther._RimColor
        && _EnableOutline == InOther._EnableOutline
        && _OutlineThickness == InOther._OutlineThickness
        && _OutlineQuality == InOther._OutlineQuality
        && _OutlineColor == InOther._OutlineColor
        && _OutlineBlendMode == InOther._OutlineBlendMode
        && _OutlineOpacity == InOther._OutlineOpacity
        && _OutlineDepthThreshold == InOther._OutlineDepthThreshold
        && _OutlineNormalThreshold == InOther._OutlineNormalThreshold
        && _OutlineAlbedoThreshold == InOther._OutlineAlbedoThreshold
        && _OutlineDistanceFade == InOther._OutlineDistanceFade
        && _EnableStencilPatterns == InOther._EnableStencilPatterns
        && _StencilBase == InOther._StencilBase
        && _DebugMode == InOther._DebugMode;
}

// --------------------------------------------------------------------------------------------------------------------
