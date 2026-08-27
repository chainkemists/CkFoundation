// Language=angelscript

//============================================================================
// HAND DRAWN - the paint / ink / shadow-stroke / paper illustration look.
//
// Shader: /CkUsf/Looks/HandDrawn.ush. Driven at runtime by
// UCkUsf_HandDrawnSubsystem, which owns the whole-view blendable and writes
// these parameters by NAME from FCk_Usf_HandDrawn_Params.
//
// _Parameters order MUST equal CkUsf_PP_HandDrawn's declaration order - the
// generator binds positionally and the validator enforces it. The defaults
// below are the "StorybookInk" preset, so an un-driven master already renders
// that style.
//============================================================================

namespace CkUsf
{
    asset HandDrawn of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/HandDrawn.ush";
        _UshFunctionName = n"CkUsf_PP_HandDrawn";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"HandDrawn";

        // Pre-TAA, and therefore pre-tonemap: the look's input and output are scene-referred linear, which
        // is why the paint path normalizes tone before it posterizes. The placement is also what lets the
        // world-attached stroke lattice ride an un-scaled WorldPosition reconstruction (at after-tonemap
        // locations that reconstruction is dynamic-resolution scaled).
        _BlendableLocation = ECk_Usf_BlendableLocation::SceneColorAfterDOF;

        // The default trio (SceneColor/Depth/Normal) is exactly what the ink detectors, the sky test and
        // the stroke projection read; CustomStencil is the effect mask's. Stated explicitly because
        // _SceneTextures is all-or-nothing - a non-empty list REPLACES the default trio rather than
        // extending it. Nothing here needs the GBuffer: this look never reconstructs illumination the way
        // CelShade does, so opting in would cost pins for nothing.
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneNormal);
        _SceneTextures.Add(ECk_Usf_SceneTexture::CustomStencil);

        // World-attached strokes are anchored to the depth-reconstructed scene surface position.
        _PostProcessWorldPosition = true;

        // ---- Master ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"StyleStrength", 1.0));

        // ---- Paint ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"SimplifyColor", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"ColorLevels", 5.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"ColorSoftness", 0.15));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Saturation", 0.9));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Contrast", 1.05));
        _Parameters.Add(CkUsf::Usf_Scalar(n"TintStrength", 0.35));
        _Parameters.Add(CkUsf::Usf_Scalar(n"AffectSky", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyDistance", 100000.0));

        // ---- Ink ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableInk", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"InkThickness", 1.5));
        _Parameters.Add(CkUsf::Usf_Scalar(n"InkOpacity", 0.9));
        _Parameters.Add(CkUsf::Usf_Scalar(n"DepthThreshold", 0.12));
        _Parameters.Add(CkUsf::Usf_Scalar(n"NormalThreshold", 0.25));
        _Parameters.Add(CkUsf::Usf_Scalar(n"ColorEdgeThreshold", 0.12));
        _Parameters.Add(CkUsf::Usf_Scalar(n"LineVariation", 1.2));
        _Parameters.Add(CkUsf::Usf_Scalar(n"LineScale", 24.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"InkFadeStartDistance", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"InkFadeEndDistance", 0.0));   // 0 = no fade

        // ---- Shadow strokes ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableShadowStrokes", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokePattern", 0.0));        // ECk_Usf_HandDrawnStrokePattern::DiagonalPencil
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeSpace", 0.0));          // ECk_Usf_HandDrawnStrokeSpace::ScreenStable
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeStrength", 0.7));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeShadowThreshold", 0.35));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokePixelSize", 6.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeWorldSize", 12.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeIrregularity", 0.4));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeTriplanarSharpness", 4.0));

        // ---- Paper ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnablePaper", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"GrainStrength", 0.12));
        _Parameters.Add(CkUsf::Usf_Scalar(n"GrainScale", 2.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"FiberStrength", 0.08));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PaperWarmth", 0.5));

        _Parameters.Add(CkUsf::Usf_Scalar(n"DebugMode", 0.0));

        // ---- Effect mask ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskMode", 0.0));             // ECk_Usf_StylizeMaskMode::Off
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMin", 190.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMax", 190.0));

        // ---- Vectors ----
        _Parameters.Add(CkUsf::Usf_Vector(n"ShadowTint", FLinearColor(0.72, 0.76, 0.90, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"HighlightTint", FLinearColor(1.0, 0.97, 0.90, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"InkColor", FLinearColor(0.05, 0.04, 0.06, 1.0)));
    }
}
