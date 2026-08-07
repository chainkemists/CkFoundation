// Language=angelscript

//============================================================================
// CROSS HATCH — the normal-aligned engraving / hatching look.
//
// Shader: /CkUsf/Looks/CrossHatch.ush. Driven at runtime by
// UCkUsf_CrossHatchSubsystem, which owns the whole-view blendable and writes
// these parameters by NAME from FCk_Usf_CrossHatch_Params.
//
// _Parameters order MUST equal CkUsf_PP_CrossHatch's declaration order — the
// generator binds positionally and the validator enforces it. The defaults
// below are the "Sketch" preset, so an un-driven master already renders that
// style.
//============================================================================

namespace CkUsf
{
    asset CrossHatch of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/CrossHatch.ush";
        _UshFunctionName = n"CkUsf_PP_CrossHatch";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"CrossHatch";

        // Reads Custom Stencil for the effect mask, which is rendered with the TAA-jittered projection —
        // thresholding it after tonemapping shimmers on a stationary camera. Consequence: the input and
        // output are scene-referred LINEAR, which is the space PaperColor is authored in.
        _BlendableLocation = ECk_Usf_BlendableLocation::SceneColorAfterDOF;

        // The default trio plus CustomStencil. SceneNormal is the load-bearing one here: the hatch
        // direction IS the projected normal, so without it every stroke would run at AngleOffset and the
        // look would be a screen-space texture rather than hatching. No GBuffer reads — nothing here
        // reconstructs illumination.
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneNormal);
        _SceneTextures.Add(ECk_Usf_SceneTexture::CustomStencil);

        // Deliberately NOT opted into: the stroke lattice is screen-space by construction (the direction
        // comes from the normal, not from a world anchor), so there is no world position to reconstruct.

        // ---- Master ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"StyleStrength", 1.0));

        // ---- Direction ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"UseWorldSpaceNormals", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"AngleOffset", 45.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"NormalAlignment", 1.0));

        // ---- Strokes ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"Spacing", 8.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"LayerCount", 3.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"LayerAngleStep", 55.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokePattern", 0.0));  // ECk_Usf_HandDrawnStrokePattern::DiagonalPencil
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeThickness", 0.55));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StrokeIrregularity", 0.35));

        // ---- Tone ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"DarknessBias", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"DarknessContrast", 1.2));

        // ---- Background ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"BackgroundMode", 1.0));  // ECk_Usf_CrossHatchBackground::Paper
        _Parameters.Add(CkUsf::Usf_Scalar(n"Saturation", 1.0));

        // ---- Sky ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"AffectSky", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyDistance", 100000.0));

        _Parameters.Add(CkUsf::Usf_Scalar(n"DebugMode", 0.0));

        // ---- Effect mask ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskMode", 0.0));        // ECk_Usf_StylizeMaskMode::Off
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMin", 190.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMax", 190.0));

        // ---- Vectors ----
        // Scene-referred LINEAR: paper white has to sit well above 1 or it tonemaps to a mid grey.
        _Parameters.Add(CkUsf::Usf_Vector(n"InkColor", FLinearColor(0.02, 0.02, 0.03, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaperColor", FLinearColor(2.20, 2.14, 1.98, 1.0)));
    }
}
