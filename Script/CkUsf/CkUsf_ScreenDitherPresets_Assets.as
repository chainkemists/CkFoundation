// Language=angelscript

//============================================================================
// SCREEN DITHER PRESETS — the six authored styles, applied via the subsystem:
//   UCkUsf_ScreenDitherSubsystem::Get_ScreenDitherSubsystem().Apply_Preset(CkUsf::DA_Dither_RetroPixel);
//
// Needs the ScreenDither master on disk (console: "Ck_Usf_GenerateLooks ScreenDither"
// on a fresh checkout). Every field left unstated is the FCk_Usf_ScreenDither_Params
// default, which IS the Balanced style — so each preset below reads as its delta
// from Balanced.
//============================================================================

namespace CkUsf
{
    // The reference style: a 4x4 ordered dither over 8 steps per channel, at full resolution.
    // Reads as a gentle print/screen texture rather than as an effect.
    asset DA_Dither_Balanced of UCkUsf_ScreenDitherPreset
    {
        _Enabled        = ECk_EnableDisable::Enable;
        _Pattern        = ECk_Usf_DitherPattern::Bayer4x4;
        _PixelScale     = 1.0;
        _DitherStrength = 1.0;
        _ColorSteps     = 8;
        _Weight         = 1.0;
    }

    // Banding cleanup rather than a style: 16 steps, a soft threshold and a half weight, so the frame
    // keeps its own colour and only the gradients gain texture. The finest pattern is used because at
    // this step count a coarse one is the most visible thing on screen.
    asset DA_Dither_SubtleColor of UCkUsf_ScreenDitherPreset
    {
        _Enabled        = ECk_EnableDisable::Enable;
        _Pattern        = ECk_Usf_DitherPattern::Bayer8x8;
        _PixelScale     = 1.0;
        _DitherStrength = 0.6;
        _ColorSteps     = 16;
        _Weight         = 0.5;
    }

    // The loud one: 4px blocks, 5 steps, and the box filter on because at this block size a point sample
    // makes thin bright detail flicker as the camera moves. Slightly lifted saturation/contrast keeps the
    // reduced palette from reading muddy.
    asset DA_Dither_RetroPixel of UCkUsf_ScreenDitherPreset
    {
        _Enabled             = ECk_EnableDisable::Enable;
        _Pattern             = ECk_Usf_DitherPattern::Bayer4x4;
        _PixelScale          = 4.0;
        _DitherStrength      = 1.0;
        _ColorSteps          = 5;
        _BoxFilterDownsample = ECk_EnableDisable::Enable;
        _StabilizeGrid       = ECk_EnableDisable::Enable;
        _Saturation          = 1.15;
        _Contrast            = 1.10;
        _Weight              = 1.0;
    }

    // Four colours, full stop — the custom-palette path rather than the monochrome one, so the four
    // entries are authored rather than derived from a tint ramp. The coarsest pattern (2x2) is what makes
    // the dither read as deliberate texture at this palette size instead of as noise.
    asset DA_Dither_FourColorHandheld of UCkUsf_ScreenDitherPreset
    {
        _Enabled             = ECk_EnableDisable::Enable;
        _Pattern             = ECk_Usf_DitherPattern::Bayer2x2;
        _PixelScale          = 3.0;
        _DitherStrength      = 1.0;
        // Same reason RetroPixel enables it: at a 3px block a point sample makes thin bright detail
        // flicker as the camera moves, and with four colours every flicker is a whole-colour jump.
        _BoxFilterDownsample = ECk_EnableDisable::Enable;
        _PaletteMode         = ECk_Usf_PaletteMode::CustomPalette;
        _Palette.Add(FLinearColor(0.06, 0.09, 0.06, 1.0));
        _Palette.Add(FLinearColor(0.19, 0.38, 0.19, 1.0));
        _Palette.Add(FLinearColor(0.54, 0.67, 0.06, 1.0));
        _Palette.Add(FLinearColor(0.61, 0.74, 0.06, 1.0));
        _Contrast            = 1.20;
        _Weight              = 1.0;
    }

    // Film-grain read: the blue-noise threshold re-rolled 20x a second. Blue noise rather than white
    // because white noise clumps at low frequencies and reads as blotching in motion, not as grain.
    asset DA_Dither_AnimatedGrain of UCkUsf_ScreenDitherPreset
    {
        _Enabled         = ECk_EnableDisable::Enable;
        _Pattern         = ECk_Usf_DitherPattern::BlueNoise;
        _PixelScale      = 1.0;
        _DitherStrength  = 1.2;
        _Animate         = ECk_EnableDisable::Enable;
        _AnimationPeriod = 0.05;
        _ColorSteps      = 10;
        _Weight          = 1.0;
    }

    // The A/B reference. Everything else is Balanced, so switching to this and back proves the effect is
    // the only difference — the frame must come back byte-for-byte clean.
    asset DA_Dither_Off of UCkUsf_ScreenDitherPreset
    {
        _Enabled = ECk_EnableDisable::Disable;
    }
}
