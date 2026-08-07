#include "CkUsf/Stylize/CkUsf_CelShadePreset.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CelShadePreset::
    Get_AsParams() const
    -> FCk_Usf_CelShade_Params
{
    auto Params = FCk_Usf_CelShade_Params{};

    Params.Set_Enabled(_Enabled)
          .Set_Bands(_Bands)
          .Set_Midpoint(_Midpoint)
          .Set_BandOffset(_BandOffset)
          .Set_Distribution(_Distribution)
          .Set_BandSoftness(_BandSoftness)
          .Set_ShadowLift(_ShadowLift)
          .Set_Strength(_Strength)
          .Set_QuantizeFinalColor(_QuantizeFinalColor)
          .Set_EnablePattern(_EnablePattern)
          .Set_Pattern(_Pattern)
          .Set_PatternSpace(_PatternSpace)
          .Set_PatternStrength(_PatternStrength)
          .Set_PatternContrast(_PatternContrast)
          .Set_PatternWorldSize(_PatternWorldSize)
          .Set_PatternPixelSize(_PatternPixelSize)
          .Set_TriplanarSharpness(_TriplanarSharpness)
          .Set_PatternDistanceScaling(_PatternDistanceScaling)
          .Set_PatternOctaveMin(_PatternOctaveMin)
          .Set_PatternOctaveMax(_PatternOctaveMax)
          .Set_PatternScrollSpeed(_PatternScrollSpeed)
          .Set_ShadowTint(_ShadowTint)
          .Set_LightTint(_LightTint)
          .Set_Saturation(_Saturation)
          .Set_MinimumAlbedo(_MinimumAlbedo)
          .Set_AffectUnlit(_AffectUnlit)
          .Set_EnableSky(_EnableSky)
          .Set_SkyDistance(_SkyDistance)
          .Set_SkyBands(_SkyBands)
          .Set_SkyStrength(_SkyStrength)
          .Set_SkyPattern(_SkyPattern)
          .Set_SkyPatternStrength(_SkyPatternStrength)
          .Set_SkyPatternScale(_SkyPatternScale)
          .Set_MetallicThreshold(_MetallicThreshold)
          .Set_MetallicBands(_MetallicBands)
          .Set_MetallicStrength(_MetallicStrength)
          .Set_MetallicPatternStrength(_MetallicPatternStrength)
          .Set_EnableSpecular(_EnableSpecular)
          .Set_SpecularSteps(_SpecularSteps)
          .Set_SpecularThreshold(_SpecularThreshold)
          .Set_SpecularIntensity(_SpecularIntensity)
          .Set_SpecularRoughnessCutoff(_SpecularRoughnessCutoff)
          .Set_EnableRimLight(_EnableRimLight)
          .Set_RimPower(_RimPower)
          .Set_RimThreshold(_RimThreshold)
          .Set_RimSoftness(_RimSoftness)
          .Set_RimIntensity(_RimIntensity)
          .Set_RimFollowsLighting(_RimFollowsLighting)
          .Set_RimColor(_RimColor)
          .Set_EnableOutline(_EnableOutline)
          .Set_OutlineThickness(_OutlineThickness)
          .Set_OutlineQuality(_OutlineQuality)
          .Set_OutlineColor(_OutlineColor)
          .Set_OutlineBlendMode(_OutlineBlendMode)
          .Set_OutlineOpacity(_OutlineOpacity)
          .Set_OutlineDepthThreshold(_OutlineDepthThreshold)
          .Set_OutlineNormalThreshold(_OutlineNormalThreshold)
          .Set_OutlineAlbedoThreshold(_OutlineAlbedoThreshold)
          .Set_OutlineDistanceFade(_OutlineDistanceFade)
          .Set_EnableStencilPatterns(_EnableStencilPatterns)
          .Set_StencilBase(_StencilBase)
          .Set_DebugMode(_DebugMode);

    return Params;
}

// --------------------------------------------------------------------------------------------------------------------
