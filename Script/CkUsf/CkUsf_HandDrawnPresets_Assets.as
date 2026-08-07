// Language=angelscript

//============================================================================
// HAND DRAWN PRESETS — the six authored styles, applied via the subsystem:
//   UCkUsf_HandDrawnSubsystem::Get_HandDrawnSubsystem().Apply_Preset(CkUsf::DA_HandDrawn_SoftPainted);
//
// Needs the HandDrawn master on disk (console: "Ck_Usf_GenerateLooks HandDrawn"
// on a fresh checkout). Every field left unstated is the
// FCk_Usf_HandDrawn_Params default, which IS the StorybookInk style — so each
// preset below reads as its delta from StorybookInk.
//============================================================================

namespace CkUsf
{
    // The reference style: five colour regions, a wandering ink contour, screen-stable pencil hatching in
    // the shadows, and a warm grainy sheet under all of it. This is what the defaults render.
    asset DA_HandDrawn_StorybookInk of UCkUsf_HandDrawnPreset
    {
        _Enabled       = ECk_EnableDisable::Enable;
        _StyleStrength = 1.0;
        _ColorLevels   = 5;
        _StrokePattern = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;
        _StrokeSpace   = ECk_Usf_HandDrawnStrokeSpace::ScreenStable;
    }

    // Watercolour rather than pen: many soft-edged regions, almost no line, no hatching at all, and the
    // paper doing most of the work. A painting has no contours — its edges are where two washes meet, so
    // the ink is turned down rather than retuned.
    asset DA_HandDrawn_SoftPainted of UCkUsf_HandDrawnPreset
    {
        _Enabled              = ECk_EnableDisable::Enable;
        _ColorLevels          = 8;
        _ColorSoftness        = 0.6;
        _Saturation           = 1.05;
        _Contrast             = 0.95;
        _TintStrength         = 0.5;
        _ShadowTint           = FLinearColor(0.70, 0.78, 0.94, 1.0);
        _HighlightTint        = FLinearColor(1.0, 0.96, 0.86, 1.0);
        _InkOpacity           = 0.25;
        _InkThickness         = 1.0;
        _LineVariation        = 2.0;
        _LineScale            = 48.0;
        _EnableShadowStrokes  = ECk_EnableDisable::Disable;
        _GrainStrength        = 0.18;
        _GrainScale           = 3.0;
        _FiberStrength        = 0.14;
        _PaperWarmth          = 0.7;
    }

    // Flat animation cel: few hard-edged regions, a thick confident line with no wander, no hatching and
    // no paper. The whole point of a cel is that it does NOT look like it was drawn on a sheet.
    asset DA_HandDrawn_BoldAnimation of UCkUsf_HandDrawnPreset
    {
        _Enabled             = ECk_EnableDisable::Enable;
        _ColorLevels         = 3;
        _ColorSoftness       = 0.0;
        _Saturation          = 1.15;
        _Contrast            = 1.15;
        _TintStrength        = 0.2;
        _InkThickness        = 3.0;
        _InkOpacity          = 1.0;
        _LineVariation       = 0.0;
        _DepthThreshold      = 0.08;
        _NormalThreshold     = 0.2;
        _ColorEdgeThreshold  = 0.1;
        _EnableShadowStrokes = ECk_EnableDisable::Disable;
        _EnablePaper         = ECk_EnableDisable::Disable;
    }

    // Woodcut: two colour regions, a heavy ragged line, and dense crosshatch reaching well past the
    // midtones. The threshold is raised rather than the strength, so the hatching spreads into the
    // half-lit areas instead of just darkening the blacks.
    asset DA_HandDrawn_DarkGothic of UCkUsf_HandDrawnPreset
    {
        _Enabled               = ECk_EnableDisable::Enable;
        _ColorLevels           = 2;
        _ColorSoftness         = 0.0;
        _Saturation            = 0.35;
        _Contrast              = 1.35;
        _TintStrength          = 0.6;
        _ShadowTint            = FLinearColor(0.55, 0.56, 0.68, 1.0);
        _HighlightTint         = FLinearColor(0.94, 0.92, 0.88, 1.0);
        _InkColor              = FLinearColor(0.01, 0.01, 0.015, 1.0);
        _InkThickness          = 2.5;
        _InkOpacity            = 1.0;
        _LineVariation         = 2.5;
        _LineScale             = 14.0;
        _StrokePattern         = ECk_Usf_HandDrawnStrokePattern::Crosshatch;
        _StrokeSpace           = ECk_Usf_HandDrawnStrokeSpace::WorldAttached;
        _StrokeStrength        = 1.0;
        _StrokeShadowThreshold = 0.6;
        _StrokeWorldSize       = 9.0;
        _StrokeIrregularity    = 0.55;
        _GrainStrength         = 0.16;
        _PaperWarmth           = 0.2;
    }

    // Graphite on paper: colour nearly gone, no posterized regions to speak of, a light line and loose
    // scribbled shading. World-attached strokes so the hatching sits ON the architecture rather than
    // sliding across it — this is the preset the gym's static judge scene is built to test.
    asset DA_HandDrawn_PencilWash of UCkUsf_HandDrawnPreset
    {
        _Enabled                 = ECk_EnableDisable::Enable;
        _ColorLevels             = 4;
        _ColorSoftness           = 0.35;
        _Saturation              = 0.2;
        _Contrast                = 1.1;
        _TintStrength            = 0.45;
        _ShadowTint              = FLinearColor(0.78, 0.78, 0.82, 1.0);
        _HighlightTint           = FLinearColor(0.99, 0.98, 0.94, 1.0);
        _InkColor                = FLinearColor(0.12, 0.12, 0.14, 1.0);
        _InkThickness            = 1.0;
        _InkOpacity              = 0.6;
        _LineVariation           = 1.8;
        _LineScale               = 18.0;
        _StrokePattern           = ECk_Usf_HandDrawnStrokePattern::LooseScribble;
        _StrokeSpace             = ECk_Usf_HandDrawnStrokeSpace::WorldAttached;
        _StrokeStrength          = 0.55;
        _StrokeShadowThreshold   = 0.5;
        _StrokeWorldSize         = 14.0;
        _StrokeIrregularity      = 0.75;
        _StrokeTriplanarSharpness = 6.0;
        _GrainStrength           = 0.2;
        _GrainScale              = 2.5;
        _FiberStrength           = 0.16;
        _PaperWarmth             = 0.85;
    }

    // The A/B reference. Everything else is StorybookInk, so switching to this and back proves the effect
    // is the only difference — the frame must come back completely clean.
    asset DA_HandDrawn_Off of UCkUsf_HandDrawnPreset
    {
        _Enabled = ECk_EnableDisable::Disable;
    }
}
