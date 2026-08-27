// Language=angelscript

//============================================================================
// PIXEL ART - the 1-texel outline / band / palette look.
//
// Shader: /CkUsf/Looks/PixelArt.ush. Driven at runtime by UCkPixelArt_Subsystem,
// which writes these parameters by NAME from FCk_PixelArt_LookParams.
//
// _Parameters order MUST equal CkUsf_PP_PixelArt's declaration order - the
// generator binds positionally and the validator enforces it. The defaults below
// are the "Crisp16" preset, so an un-driven master already renders that style.
//============================================================================

namespace CkUsf
{
    asset PixelArt of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/PixelArt.ush";
        _UshFunctionName = n"CkUsf_PP_PixelArt";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"PixelArt";

        // RENDER resolution, pre-TAA - the one placement this look can have. An outline is exactly one
        // texel wide only if it is computed at the resolution the scene was rasterized at; after
        // tonemapping the kernel steps in display pixels instead, and a "1-pixel" line comes out as wide
        // as the upscale factor. That is the entire thing the pixel-art renderer exists to prevent.
        _BlendableLocation = ECk_Usf_BlendableLocation::SceneColorAfterDOF;

        // The default trio, stated explicitly: all three are genuinely read here - colour for the banding
        // and palette, depth for silhouettes, world normal for creases and the convex/concave decision.
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneNormal);

        // ---- Outline ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"EnableOutline", 1.0, n"Outline", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DepthThreshold", 0.15, n"Outline", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"AngleZCutoff", 0.25, n"Outline", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"AngleZScale", 24.0, n"Outline", 3));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"NormalSmoothLow", 0.1, n"Outline", 4));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"NormalSmoothHigh", 0.4, n"Outline", 5));

        // ---- Edges ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"LineDarken", 0.4, n"Edges", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"CreaseBrighten", 0.3, n"Edges", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"EdgeMode", 1.0, n"Edges", 2));            // ECk_PixelArt_EdgeMode::BandShift

        // ---- Banding ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"Bands", 4.0, n"Banding", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"ThresholdGradientSize", 0.05, n"Banding", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DitherStrength", 0.0, n"Banding", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"ColorSpace", 0.0, n"Banding", 3));          // ECk_PixelArt_ColorSpace::Linear
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"WarmShift", 0.0, n"Banding", 4));

        // ---- Palette ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"PaletteMode", 1.0, n"Palette", 0));         // ECk_PixelArt_PaletteMode::CustomPalette
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"PaletteCount", 8.0, n"Palette", 1));

        // The custom palette is a FIXED 8 slots; PaletteCount says how many are live. The defaults are a
        // dusk ramp so an un-driven master renders something coherent rather than eight blacks.
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor0", FLinearColor(0.05, 0.05, 0.09, 1.0), n"Palette", 2));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor1", FLinearColor(0.14, 0.12, 0.24, 1.0), n"Palette", 3));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor2", FLinearColor(0.28, 0.20, 0.33, 1.0), n"Palette", 4));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor3", FLinearColor(0.45, 0.29, 0.35, 1.0), n"Palette", 5));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor4", FLinearColor(0.63, 0.42, 0.36, 1.0), n"Palette", 6));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor5", FLinearColor(0.80, 0.59, 0.42, 1.0), n"Palette", 7));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor6", FLinearColor(0.92, 0.78, 0.57, 1.0), n"Palette", 8));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"PaletteColor7", FLinearColor(0.99, 0.95, 0.85, 1.0), n"Palette", 9));
    }
}
