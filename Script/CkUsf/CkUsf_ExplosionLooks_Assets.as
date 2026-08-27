// Language=angelscript

//============================================================================
// VEFECTS "DissolveAdd" FAMILY - the explosion parameterizations
//============================================================================
//
// Three looks for five ports. The four explosion variants between them need
// exactly ONE new parameterization (the ground scorch decal, which only the
// two Ground systems draw); the bomb explosion needs the two noise-bubble
// paints. Every other material those five systems wear is a look an earlier
// batch already carries, checked value-by-value against the sheets' Sec.4 delta
// tables before reuse:
//
//   Part01 -> PartDisAdd01, Part02 -> PartDisAdd02, Part03 -> PartDisAdd03,
//   Part04 -> PartDisAdd04, Ring01 -> RingDisAdd01, Star01 -> StarDisAdd01,
//   Flare01 -> FlareDisAdd01, Rainbow -> RainbowDisAdd, Smoke01 ->
//   SmokeDisAdd01, Flames01 -> FlamesDisAdd01, Trail03 -> TrailDisAdd03,
//   Impact01 -> ImpactDisAdd01, LightStrip -> LightStripDisAdd, Flat02 ->
//   FlatAdd02, and the three MI_VFX_FresnelBomb instances -> ExpFresnelBomb01/
//   02/03.
//
// Every default below is a cell of a recipe's per-material delta table:
// CkFoundation/Source/CkParticles/Cookbook/NS_ExplosionGround.md Sec.4.1 and
// NS_Bomb_Explosion.md Sec.4.1. A delta table states the FULL inherited pair, so
// every value is stated explicitly rather than defaulted.
//
// FAMILY PARAMETERS THESE INSTANCES DRIVE THAT THE LOOK DOES NOT PLUMB:
// Core_Intensity (1 / 20 / 5), Color_CoreDifferent (1 on BubbleOut_01),
// Opacty_DepthFade and a Color_Tex separate from Main_Tex. All four are the
// standing family gaps recorded in each recipe's Sec.13. The Color_Core tint on
// BubbleOut_01 - RGBA(0.226966, 0.520996, 1, 1) - is the one that WOULD be
// reachable (the look exposes CoreColor), and it is PROVABLY INERT anyway:
// the family blends toward it by the `core_color` dynamic channel, and every
// Bubble emitter writes that channel as 0 (NS_Bomb_Explosion.md Sec.5.9). So the
// family helper is left alone rather than gaining a parameter no look could
// make visible.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Star04 - the scorch decal the ground explosions leave behind, and the batch's only new
    // DissolveAdd parameterization. Its shape paint is NOT the four-point star its name suggests: measured
    // against T_VFX_Star_01 it carries no dominant angular harmonic at all, so it takes its own
    // ExpGroundScorch bake (recipe Sec.7).
    //
    // The one instance in this batch whose dissolve paint differs from its shape paint AND from its
    // distortion paint: shape is the scorch, dissolve is the soft particle, distortion is the family noise.
    asset ExpGroundMarkDisAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ExpGroundMarkDisAdd";
        _TwoSided        = true;

        // A CUSTOM-FACING sprite is still a sprite renderer, so this is the sprite usage bit.
        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "ExpGroundScorch", "SoftParticle",
            1.0,          // Brightness - inherited from the family reference
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity - dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness - inherited from the family reference
            "LutWhite",   // GradientMap_Tex - the family's white pixel, so the gradient chain is inert
            0.1,          // GradientMap_Displacement
            0.0,          // Gradient_Invert
            0.1,          // Distortion_Scale - inherited
            "TileNoise"); // Distortion_Tex is T_VFX_Noise_02, distinct from this instance's dissolve paint
    }

    // M_VFX_DisAdd_BubbleNoise_01 - the bomb explosion's first noise shell. Its shape paint is
    // T_VFX_Gradient_03, which the sheet flags as "a COLOUR shape texture in a slot the look treats as a
    // greyscale mask": measured, its channel spread is exactly 0.0000 over all 262 144 pixels, so the gap
    // does not exist and the GradientTrapezoid bake is a transcription rather than a stand-in (recipe Sec.7).
    //
    // The only look in the cookbook that pans its dissolve UPWARDS along V while the behavior holds the
    // dissolve channel at a static -0.5, so the shell erodes continuously rather than on a curve.
    asset ExpBubbleNoiseDisAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ExpBubbleNoiseDisAdd";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "GradientTrapezoid", "TileNoiseFine",
            10.0,         // Brightness
            0.0, -0.1,    // Dissolve_Speed - one axis only, up V
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity - dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",
            0.1,          // GradientMap_Displacement
            0.5,          // Gradient_Invert - inherited from the Part01 reference
            0.1,          // Distortion_Scale - inherited
            "TileNoise");
    }

    // M_VFX_DisAdd_BubbleOut_01 - the bomb explosion's expanding shock ring, drawn on the flat annulus
    // carrier. Its dissolve is squashed to 0.3 x 0.5 of the UV and pans down V at unit speed, which sweeps
    // the erosion around the ring as it grows; its dissolve paint is T_VFX_Noise_06, the hard-floored noise
    // the projectile batch already baked as TileNoiseSparse, so the ring clears in patches.
    asset ExpBubbleOutDisAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ExpBubbleOutDisAdd";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "GradientTrapezoid", "TileNoiseSparse",
            4.0,          // Brightness
            0.0, 1.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            0.3, 0.5,     // Dissolve_Scale
            0.0,          // Distortion_Intensity - dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",
            0.1,
            0.5,          // Gradient_Invert - inherited from the Part01 reference
            0.1,
            "TileNoise");
    }
}
