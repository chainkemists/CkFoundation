// Language=angelscript

//============================================================================
// SCREEN DITHER — the screen dithering / palette reduction look.
//
// Shader: /CkUsf/Looks/ScreenDither.ush. Driven at runtime by
// UCkUsf_ScreenDitherSubsystem, which owns the whole-view blendable and writes
// these parameters by NAME from FCk_Usf_ScreenDither_Params.
//
// _Parameters order MUST equal CkUsf_PP_ScreenDither's declaration order — the
// generator binds positionally and the validator enforces it. The defaults below
// are the "Balanced" preset, so an un-driven master already renders that style.
//============================================================================

namespace CkUsf
{
    asset ScreenDither of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/ScreenDither.ush";
        _UshFunctionName = n"CkUsf_PP_ScreenDither";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"ScreenDither";

        // Palette reduction must see the FINAL display-referred frame: quantizing scene-referred HDR
        // would band wherever the tonemapper is steep and leave the authored step count meaningless.
        // Nothing here reads Custom Depth/Stencil, so the pre-TAA requirement does not apply.
        _BlendableLocation = ECk_Usf_BlendableLocation::AfterTonemapping;

        // The default trio is stated explicitly because CustomStencil has to be added for the effect
        // mask, and _SceneTextures is all-or-nothing: a non-empty list replaces the default trio rather
        // than extending it. Only SceneColor and CustomStencil are actually read; depth and normal are
        // the historical default this look has always carried. The GBuffer reads stay out — nothing here
        // reconstructs illumination, so they would cost pins for nothing.
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneNormal);
        _SceneTextures.Add(ECk_Usf_SceneTexture::CustomStencil);

        // ---- Pattern ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"DitherPattern", 1.0));       // ECk_Usf_DitherPattern::Bayer4x4
        _Parameters.Add(CkUsf::Usf_Scalar(n"PixelScale", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"DitherStrength", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Animate", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"AnimationPeriod", 0.1));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BoxFilterDownsample", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StabilizeGrid", 1.0));

        // ---- Palette ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"PaletteMode", 0.0));         // ECk_Usf_PaletteMode::ColorSteps
        _Parameters.Add(CkUsf::Usf_Scalar(n"ColorSteps", 8.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PaletteCount", 1.0));

        // ---- Colour shaping ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"ColorSpace", 0.0));          // ECk_Usf_DitherColorSpace::Gamma
        _Parameters.Add(CkUsf::Usf_Scalar(n"PreGamma", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Monochrome", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Saturation", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Contrast", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Weight", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"DebugMode", 0.0));

        // ---- Effect mask ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskMode", 0.0));            // ECk_Usf_StylizeMaskMode::Off
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMin", 190.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMax", 190.0));

        // ---- Vectors ----
        _Parameters.Add(CkUsf::Usf_Vector(n"MonochromeShadowTint", FLinearColor(0.04, 0.06, 0.05, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"MonochromeHighlightTint", FLinearColor(0.78, 0.92, 0.70, 1.0)));

        // The custom palette is a FIXED 8 slots; PaletteCount says how many are live. Unused slots are
        // written black by the subsystem so a shrunk palette leaves nothing stale to snap to.
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor0", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor1", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor2", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor3", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor4", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor5", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor6", FLinearColor(0.0, 0.0, 0.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"PaletteColor7", FLinearColor(0.0, 0.0, 0.0, 1.0)));
    }
}
