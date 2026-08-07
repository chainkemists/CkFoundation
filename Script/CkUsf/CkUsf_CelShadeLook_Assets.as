// Language=angelscript

//============================================================================
// CEL SHADE — the quantized-band / halftone-transition look.
//
// Shader: /CkUsf/Looks/CelShade.ush. Driven at runtime by
// UCkUsf_CelShadeSubsystem, which owns the whole-view blendable and writes
// these parameters by NAME from FCk_Usf_CelShade_Params.
//
// _Parameters order MUST equal CkUsf_PP_CelShade's declaration order — the
// generator binds positionally and the validator enforces it. The defaults
// below are the "Balanced" preset, so an un-driven master already renders
// that style.
//============================================================================

namespace CkUsf
{
    asset CelShade of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/CelShade.ush";
        _UshFunctionName = n"CkUsf_PP_CelShade";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"CelShade";

        // Custom Stencil is rendered with the TAA-jittered projection, so a post-tonemap placement would
        // threshold a mask that shifts sub-pixel every frame with no temporal resolve ever seeing it.
        // Pre-TAA is also pre-tonemap: the look's input and output are scene-referred linear.
        _BlendableLocation = ECk_Usf_BlendableLocation::SceneColorAfterDOF;

        // The illumination reconstruction needs BaseColor; the Metallic group and the specular roughness
        // cutoff need Metallic and Roughness; the per-object contract needs CustomStencil. PPI_Specular
        // is deliberately NOT wired — nothing in the shader reads it, and every wired input costs a pin
        // and a GBuffer fetch on this already-wide node.
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneNormal);
        _SceneTextures.Add(ECk_Usf_SceneTexture::CustomStencil);
        _SceneTextures.Add(ECk_Usf_SceneTexture::BaseColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::Metallic);
        _SceneTextures.Add(ECk_Usf_SceneTexture::Roughness);

        // World-space pattern cells are anchored to the depth-reconstructed scene surface position.
        _PostProcessWorldPosition = true;

        // ---- Bands ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"Bands", 4.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Midpoint", 0.5));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandOffset", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Distribution", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandSoftness", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"ShadowLift", 0.15));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Strength", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"QuantizeFinalColor", 0.0));

        // ---- Pattern ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnablePattern", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Pattern", 1.0));            // ECk_Usf_CelPattern::RoundDots
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternSpace", 0.0));       // ECk_Usf_CelPatternSpace::World
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternStrength", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternContrast", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternWorldSize", 16.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternPixelSize", 6.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"TriplanarSharpness", 4.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternDistanceScaling", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternOctaveMin", 0.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternOctaveMax", 4.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"PatternScrollSpeed", 0.0));

        // ---- Colour ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"Saturation", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MinimumAlbedo", 0.04));
        _Parameters.Add(CkUsf::Usf_Scalar(n"AffectUnlit", 0.0));

        // ---- Sky ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableSky", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyDistance", 100000.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyBands", 3.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyStrength", 0.6));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyPattern", 0.0));         // ECk_Usf_CelPattern::Bayer
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyPatternStrength", 0.4));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SkyPatternScale", 2.0));

        // ---- Metallic ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"MetallicThreshold", 0.5));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MetallicBands", 6.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MetallicStrength", 0.75));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MetallicPatternStrength", 0.3));

        // ---- Specular ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableSpecular", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SpecularSteps", 2.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SpecularThreshold", 0.8));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SpecularIntensity", 0.6));
        _Parameters.Add(CkUsf::Usf_Scalar(n"SpecularRoughnessCutoff", 0.4));

        // ---- Rim ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableRimLight", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"RimPower", 3.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"RimThreshold", 0.4));
        _Parameters.Add(CkUsf::Usf_Scalar(n"RimSoftness", 0.2));
        _Parameters.Add(CkUsf::Usf_Scalar(n"RimIntensity", 0.5));
        _Parameters.Add(CkUsf::Usf_Scalar(n"RimFollowsLighting", 1.0));

        // ---- Outline ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableOutline", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineThickness", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineQuality", 0.0));     // ECk_Usf_CelOutlineQuality::FourTap
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineOpacity", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineBlendMode", 0.0));   // ECk_Usf_CelOutlineBlend::Blend
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineDepthThreshold", 0.15));
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineNormalThreshold", 0.25));
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineAlbedoThreshold", 0.35));
        _Parameters.Add(CkUsf::Usf_Scalar(n"OutlineDistanceFade", 0.0));

        // ---- Per-object stencil ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"EnableStencilPatterns", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"StencilBase", 200.0));

        _Parameters.Add(CkUsf::Usf_Scalar(n"DebugMode", 0.0));

        // ---- Band distribution ----
        // Appended after every pre-existing scalar rather than inserted next to Distribution: the
        // generator binds POSITIONALLY, so inserting would silently re-bind every parameter after the
        // insertion point in any master not regenerated in lockstep. Still BEFORE the vectors, because
        // the whole list has to stay grouped by type in the order the .ush declares it.
        _Parameters.Add(CkUsf::Usf_Scalar(n"DistributionMode", 0.0));   // ECk_Usf_CelDistribution::Exponent
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdgeCount", 1.0));
        // The list is a FIXED 8 slots + a live count, the ScreenDither palette precedent — a material has
        // no array parameters. Unused slots are written 1.0 by the subsystem: past the count the shader
        // ignores them, and 1.0 is the only value that could not fabricate a band if it ever did not.
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge0", 0.5));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge1", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge2", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge3", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge4", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge5", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge6", 1.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdge7", 1.0));

        // ---- Effect mask ----
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskMode", 0.0));           // ECk_Usf_StylizeMaskMode::Off
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMin", 190.0));
        _Parameters.Add(CkUsf::Usf_Scalar(n"MaskStencilMax", 190.0));

        // ---- Vectors ----
        _Parameters.Add(CkUsf::Usf_Vector(n"ShadowTint", FLinearColor(0.72, 0.78, 1.0, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"LightTint", FLinearColor(1.0, 0.98, 0.92, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"RimColor", FLinearColor(1.0, 0.95, 0.85, 1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"OutlineColor", FLinearColor(0.02, 0.02, 0.03, 1.0)));
    }
}
