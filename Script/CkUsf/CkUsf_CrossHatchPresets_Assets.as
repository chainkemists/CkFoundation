// Language=angelscript

//============================================================================
// CROSS HATCH PRESETS - the four authored styles, applied via the subsystem:
//   UCkUsf_CrossHatchSubsystem::Get_CrossHatchSubsystem().Apply_Preset(CkUsf::DA_CrossHatch_Engraving);
//
// Needs the CrossHatch master on disk (console: "Ck_Usf_GenerateLooks CrossHatch"
// on a fresh checkout). Every field left unstated is the
// FCk_Usf_CrossHatch_Params default, which IS the Sketch style - so each preset
// below reads as its delta from Sketch.
//
// Every colour here is SCENE-REFERRED LINEAR (the look sits pre-tonemap), which
// is why the paper values are above 1: a PaperColor of 1.0 tonemaps to a mid
// grey rather than to paper white.
//============================================================================

namespace CkUsf
{
    // The reference style: loose graphite on a warm sheet. Three stroke layers at a wide angle step, a
    // pencil pattern with real wander, and enough thickness left under 1 that the darkest regions still
    // show paper between the strokes. This is what the defaults render.
    asset DA_CrossHatch_Sketch of UCkUsf_CrossHatchPreset
    {
        _Enabled        = ECk_EnableDisable::Enable;
        _StyleStrength  = 1.0;
        _StrokePattern  = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;
        _LayerCount     = 3;
        _Spacing        = 8.0;
    }

    // Copperplate: fine, tightly-spaced, highly regular lines at four layers, on a cool bright sheet.
    // Irregularity is pulled almost to zero on purpose - an engraved line is cut, not drawn, and any
    // wobble reads as a mistake rather than as a hand.
    asset DA_CrossHatch_Engraving of UCkUsf_CrossHatchPreset
    {
        _Enabled            = ECk_EnableDisable::Enable;
        _StrokePattern      = ECk_Usf_HandDrawnStrokePattern::Crosshatch;
        _Spacing            = 5.0;
        _LayerCount         = 4;
        _LayerAngleStep     = 40.0;
        _StrokeThickness    = 0.45;
        _StrokeIrregularity = 0.05;
        _DarknessContrast   = 1.5;
        _InkColor           = FLinearColor(0.015, 0.014, 0.02, 1.0);
        _PaperColor         = FLinearColor(2.35, 2.32, 2.20, 1.0);
    }

    // Technical drawing: white ink on a saturated blue ground, one layer only, and NO normal alignment
    // a blueprint's hatching is a fill convention at a fixed angle, not a description of form. The
    // NormalAlignment of 0 is what makes this preset the control that proves the alignment does
    // something in the other three.
    asset DA_CrossHatch_Blueprint of UCkUsf_CrossHatchPreset
    {
        _Enabled            = ECk_EnableDisable::Enable;
        _NormalAlignment    = 0.0;
        _AngleOffset        = 30.0;
        _StrokePattern      = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;
        _Spacing            = 7.0;
        _LayerCount         = 1;
        _StrokeThickness    = 0.4;
        _StrokeIrregularity = 0.0;
        _DarknessContrast   = 1.0;
        _DarknessBias       = 0.1;
        _InkColor           = FLinearColor(1.9, 2.0, 2.1, 1.0);
        _PaperColor         = FLinearColor(0.05, 0.14, 0.42, 1.0);
    }

    // The A/B reference. Everything else is Sketch, so switching to this and back proves the effect is
    // the only difference - the frame must come back completely clean.
    asset DA_CrossHatch_Off of UCkUsf_CrossHatchPreset
    {
        _Enabled = ECk_EnableDisable::Disable;
    }
}
