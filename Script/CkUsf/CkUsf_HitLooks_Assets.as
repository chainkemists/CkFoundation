// Language=angelscript

//============================================================================
// VEFECTS "DissolveAdd" FAMILY - the impact/hit parameterizations
//============================================================================
//
// The second batch of DissolveAdd instances the cookbook carries. They share
// the family entry point and its parameter helper with
// CkUsf_SlashLooks_Assets.as - one shader, N parameterizations, because the
// source ships ONE parent graph (M_VFX_DissolveAdd) and one material INSTANCE
// per emitter. Nothing here is a second copy of the math.
//
// Every default below is quoted in a recipe's per-material delta table:
// CkFoundation/Source/CkParticles/Cookbook/NS_Fire.md Sec.4,
// NS_FireBall_Hit.md Sec.4.1 and NS_Gunshot_Hit.md Sec.4.1. A delta table states the
// FULL inherited pair, never just the changed axis - anything a table does not
// list resolves to the family REFERENCE instance (M_VFX_DisAdd_Part01), not to
// zero, so every value below is stated explicitly rather than defaulted.
//
// Textures are the plugin's own PROCEDURAL stand-ins, parameterized from
// measured characteristics of the source paints (recipes Sec.7). Each was measured
// against the nearest EXISTING stand-in first and every one of them failed that
// comparison on a structural statistic, so each carries its own bake. No pack
// art is imported for these effects.
//
// FAMILY PARAMETERS THESE INSTANCES DRIVE THAT THE LOOK DOES NOT PLUMB:
// Core_Intensity, Core_Power, CamOffset, Opacty_DepthFade, Opacty_StepAdd, a
// GradientShape_Tex separate from the shape, and a Color_Tex separate from
// Main_Tex. Their chains are not reconstructible from the corpus (the parent
// graph is not exported, only an expression histogram), so they are recorded as
// known differences in each recipe's Sec.13 rather than guessed at. Glow_Intensity
// IS reproduced, folded into Brightness - on an unlit additive composite the two
// are the same emissive scale.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Part01_Bright - the fine sparkle paint. Brightness 10 over the reference's 1, full opacity
    // boldness, and the Part_02 shoulder-and-fall bell rather than Part_01's power curve.
    asset PartDisAdd01Bright of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd01Bright";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticleBright", "SoftParticleBright",
            10.0,         // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity - dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0);         // Opacity_Boldness
    }

    // M_VFX_DisAdd_Part02 - the one instance in the batch that DIMS: Glow_Intensity 0.3 against the family's 1,
    // folded into Brightness. It is the only Gunshot_Hit instance that keeps the reference's 0.5 boldness.
    asset PartDisAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd02";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticleBright", "SoftParticleBright",
            0.3,          // Brightness 1 x Glow_Intensity 0.3
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            0.5);         // Opacity_Boldness - inherited from the reference
    }

    // M_VFX_DisAdd_Part03_Bright - the hot flash paint, on the exponential Part_03 falloff. Its CamOffset of 50
    // is the family's camera-toward world-position push and is not plumbed here.
    asset PartDisAdd03Bright of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd03Bright";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticleFine", "SoftParticleFine",
            10.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0);
    }

    // M_VFX_DisAdd_Star01 - the four-point star. First instance in the cookbook to resolve Gradient_Invert to 0
    // rather than the family's 0.5; inert either way while the ramp is flat white.
    asset StarDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"StarDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "StarFour", "StarFour",
            6.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0,          // Opacity_Boldness
            "LutWhite",   // GradientMap_Tex
            0.1,          // GradientMap_Displacement
            0.0);         // Gradient_Invert
    }

    // M_VFX_DisAdd_Impact01 - the flare-impact five-ray star, and the brightest non-HDR instance in the batch.
    asset ImpactDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ImpactDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "ImpactStar", "ImpactStar",
            12.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0);
    }

    // M_VFX_DisAdd_Impact02 - the six sub-UV impact flashes of NS_Gunshot_Hit, and the effect's dominant element:
    // Brightness 15 under a colour curve whose first key is 5x blue-white. Its shape is a 2x2 flipbook, divided
    // by the row renderer's SubImageSize rather than by anything in this look.
    asset ImpactDisAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ImpactDisAdd02";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "ImpactSheet", "ImpactSheet",
            15.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0,
            "LutWhite",
            0.1,
            0.0);         // Gradient_Invert
    }

    // M_VFX_DisAdd_Ring01 - the shockwave ring, on the hand-painted uneven annulus rather than a clean SDF one.
    asset RingDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"RingDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "RingUneven", "RingUneven",
            10.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0,
            "LutWhite",
            0.1,
            0.0);
    }

    // M_VFX_DisAdd_Rainbow - the lens ring. The ONLY instance in the cookbook whose source GradientMap_Tex is a
    // real colour ramp (T_VFX_LUT_Rainbow_01) rather than the family's white pixel, and the only one whose
    // Gradient_Invert resolves past 1.
    //
    // It ships against the FLAT WHITE ramp pending campaign decision [P1-D1]: the family's Gradient_Invert remap
    // is not recoverable from the corpus (the parent graph is unexported and the four resolved values constrain
    // several readings), and against a white ramp the whole chain is a provable multiply by one - so this look
    // renders exactly as it would with no gradient chain at all until that decision lands. Recorded in
    // NS_FireBall_Hit.md Sec.13.
    asset RainbowDisAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"RainbowDisAdd";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "RingFlare", "SoftParticle",
            1.0,          // Brightness - inherited from the reference
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.5,          // Opacity_Boldness
            "LutWhite",   // GradientMap_Tex - the source's Rainbow LUT, held back by [P1-D1]
            0.9,          // GradientMap_Displacement
            2.0);         // Gradient_Invert
    }

    // M_VFX_DisAdd_Flare01 - the second lens instance, sharing Rainbow's ring paint at a quarter of its opacity
    // boldness and the family's own gradient displacement.
    asset FlareDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"FlareDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "RingFlare", "SoftParticle",
            2.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0,
            "LutWhite",
            0.1,
            0.847619);    // Gradient_Invert
    }

    // M_VFX_DisAdd_LightStrip - the lightning cards. On a MESH renderer, so it takes the mesh-particle usage flag
    // rather than the sprite one.
    asset LightStripDisAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"LightStripDisAdd";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "LightStrip", "LightStrip",
            7.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0);
    }

    // M_VFX_DisAdd_Flames01 - the sub-UV flame puffs, shared by NS_Fire and NS_FireBall_Hit. One of only two
    // instances in the whole cookbook with a LIVE distortion branch, and the only one that drives a static
    // dissolve bias, a tiled dissolve and a panning distortion at once.
    asset FlamesDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"FlamesDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "WindSheet", "TileNoiseCoarse",
            20.0,         // Brightness 10 x Glow_Intensity 2
            0.0, 0.0,     // Dissolve_Speed
            -0.1,         // Dissolve (static bias)
            2.0, 2.0,     // Dissolve_Scale
            0.5,          // Distortion_Intensity - LIVE
            -0.3, -0.3,   // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",
            0.1,
            0.0,          // Gradient_Invert
            2.0);         // Distortion_Scale
    }

    // M_VFX_DisAdd_Smoke01 - the smoke puffs. The only instance whose Dissolve_Tex and Distortion_Tex are
    // DIFFERENT assets, and the only one whose opacity boldness resolves past 1 (3, against the reference's 0.5).
    // Its separate Color_Tex (the darker of the two cloud paints) is not plumbed; the shape carries Main_Tex.
    asset SmokeDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"SmokeDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "Cloud05", "TileNoiseBanded",
            10.0,
            0.0, 0.0,
            -0.1,         // Dissolve (static bias)
            1.0, 1.0,
            0.4,          // Distortion_Intensity - LIVE
            0.1, 0.1,     // Distortion_Speed
            1.0, 1.0,
            3.0,          // Opacity_Boldness
            "LutWhite",
            0.75,         // GradientMap_Displacement
            0.0,          // Gradient_Invert
            0.1,          // Distortion_Scale - inherited
            "TileNoiseCoarse"); // Distortion_Tex, distinct from Dissolve_Tex on this instance alone
    }
}
