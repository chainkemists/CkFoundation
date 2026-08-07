// Language=angelscript

//============================================================================
// CEL SHADE PRESETS — the six authored styles, applied via the subsystem:
//   UCkUsf_CelShadeSubsystem::Get_CelShadeSubsystem().Apply_Preset(CkUsf::DA_Cel_CleanAnime);
//
// Needs the CelShade master on disk (console: "Ck_Usf_GenerateLooks CelShade"
// on a fresh checkout) and r.CustomDepth 3 for the per-object stencil contract.
// Every field left unstated is the FCk_Usf_CelShade_Params default, which IS
// the Balanced style — so each preset below reads as its delta from Balanced.
//============================================================================

namespace CkUsf
{
    // The reference style: four bands anchored at mid exposure, world-space round dots across every
    // transition, a thin ink line. Reads as "stylized" without committing to a genre.
    asset DA_Cel_Balanced of UCkUsf_CelShadePreset
    {
        _Enabled         = ECk_EnableDisable::Enable;
        _Bands           = 4;
        _Midpoint        = 0.5;
        _Pattern         = ECk_Usf_CelPattern::RoundDots;
        _PatternSpace    = ECk_Usf_CelPatternSpace::World;
        _PatternStrength = 1.0;
        _Strength        = 1.0;
    }

    // Flat cel with almost no texture: three hard bands, the pattern off, a slightly cool shadow and a
    // crisper line. Anime cels are painted regions with ink on top, so the transition machinery is the
    // first thing to switch off rather than to tune down.
    asset DA_Cel_CleanAnime of UCkUsf_CelShadePreset
    {
        _Enabled                = ECk_EnableDisable::Enable;
        _Bands                  = 3;
        _ShadowLift             = 0.22;
        _BandSoftness           = 0.02;
        _EnablePattern          = ECk_EnableDisable::Enable;
        _Pattern                = ECk_Usf_CelPattern::RoundDots;
        _PatternStrength        = 0.35;
        _PatternWorldSize       = 10.0;
        _PatternContrast        = 3.0;
        _ShadowTint             = FLinearColor(0.62, 0.70, 0.98, 1.0);
        _Saturation             = 1.1;
        _OutlineThickness       = 1.0;
        _OutlineNormalThreshold = 0.18;
        _OutlineAlbedoThreshold = 0.25;
        _EnableRimLight         = ECk_EnableDisable::Enable;
        _RimIntensity           = 0.35;
    }

    // Print reproduction: coarse world-space dots at full strength over few bands, high pattern contrast
    // so the dots read as ink rather than as a gradient, and a heavy multiplied line.
    asset DA_Cel_ComicHalftone of UCkUsf_CelShadePreset
    {
        _Enabled                = ECk_EnableDisable::Enable;
        _Bands                  = 3;
        _ShadowLift             = 0.1;
        _Pattern                = ECk_Usf_CelPattern::RoundDots;
        _PatternSpace           = ECk_Usf_CelPatternSpace::World;
        _PatternStrength        = 1.0;
        _PatternContrast        = 2.5;
        _PatternWorldSize       = 24.0;
        _Saturation             = 0.85;
        _ShadowTint             = FLinearColor(0.85, 0.84, 0.92, 1.0);
        _OutlineThickness       = 2.0;
        _OutlineQuality         = ECk_Usf_CelOutlineQuality::EightTap;
        _OutlineBlendMode       = ECk_Usf_CelOutlineBlend::Multiply;
        _OutlineColor           = FLinearColor(0.05, 0.04, 0.06, 1.0);
        _EnableSpecular         = ECk_EnableDisable::Disable;
        _SkyPattern             = ECk_Usf_CelPattern::RoundDots;
        _SkyPatternScale        = 3.0;
    }

    // Pen-and-ink: crosshatch instead of dots, more bands so the hatching has several densities to move
    // through, and no specular — a hatched drawing has no highlights, it has unhatched paper.
    asset DA_Cel_InkCrosshatch of UCkUsf_CelShadePreset
    {
        _Enabled                = ECk_EnableDisable::Enable;
        _Bands                  = 5;
        _ShadowLift             = 0.08;
        _Distribution           = 1.4;
        _Pattern                = ECk_Usf_CelPattern::Crosshatch;
        _PatternSpace           = ECk_Usf_CelPatternSpace::World;
        _PatternStrength        = 1.0;
        _PatternContrast        = 1.8;
        _PatternWorldSize       = 8.0;
        _Saturation             = 0.35;
        _ShadowTint             = FLinearColor(0.78, 0.76, 0.72, 1.0);
        _LightTint              = FLinearColor(1.0, 0.99, 0.95, 1.0);
        _OutlineThickness       = 1.0;
        _OutlineQuality         = ECk_Usf_CelOutlineQuality::EightTap;
        _OutlineBlendMode       = ECk_Usf_CelOutlineBlend::Multiply;
        _EnableSpecular         = ECk_EnableDisable::Disable;
        _EnableRimLight         = ECk_EnableDisable::Disable;
        _SkyPattern             = ECk_Usf_CelPattern::DiagonalLines;
    }

    // The gentle one: many bands with wide softness and the pattern nearly off, so shading reads as
    // simplified rather than as posterized, and no ink line at all.
    asset DA_Cel_SoftToon of UCkUsf_CelShadePreset
    {
        _Enabled          = ECk_EnableDisable::Enable;
        _Bands            = 6;
        _BandSoftness     = 0.55;
        _ShadowLift       = 0.28;
        _Distribution     = 0.85;
        _EnablePattern    = ECk_EnableDisable::Disable;
        _ShadowTint       = FLinearColor(0.80, 0.84, 0.98, 1.0);
        _Saturation       = 1.05;
        _EnableOutline    = ECk_EnableDisable::Disable;
        _RimIntensity     = 0.7;
        _RimSoftness      = 0.35;
        _SkyStrength      = 0.3;
        _MetallicStrength = 0.5;
    }

    // The A/B reference. Everything else is Balanced, so switching to this and back proves the effect is
    // the only difference — the frame must come back completely clean.
    asset DA_Cel_Off of UCkUsf_CelShadePreset
    {
        _Enabled = ECk_EnableDisable::Disable;
    }
}
