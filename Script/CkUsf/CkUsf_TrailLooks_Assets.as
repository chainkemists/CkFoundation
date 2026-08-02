// Language=angelscript

//============================================================================
// VEFECTS TRAIL PARAMETERIZATIONS — the RIBBON-drawn looks
//============================================================================
//
// Three looks, from two different Vefects families, sharing one property no
// earlier look has: they are drawn by a Niagara RIBBON renderer.
//
// `_UsedWithNiagaraRibbons` is a THIRD independent usage flag, not a synonym
// for the sprite one. A master that never declares it renders as the engine's
// default material under a ribbon renderer — silently, exactly like the
// mesh-particle miss `LightStripDisAddSprite` was created to fix. The template
// builder refuses to emit a ribbon renderer over a master without the flag,
// so the two are checked against each other rather than trusted.
//
// Every default below is quoted in a recipe's per-material delta table:
// CkFoundation/Source/CkParticles/Cookbook/NS_FireBall_Projectile.md §4.2,
// NS_Bomb_Projectile.md §4.1 and NS_BuffCast.md §4. A delta table states the
// FULL inherited pair, so every value is stated here explicitly rather than
// defaulted.
//
// FAMILY PARAMETERS TrailDisAdd01 DRIVES THAT THE LOOK DOES NOT PLUMB:
// Core_Intensity (2), Color_CoreDifferent (1), the Color_Core tint
// RGBA(0.057805, 0.313989, 0.590619, 1) — the family helper pins CoreColor
// white — and Opacty_DepthFade (30). Its GradientMap_Tex points at a REAL
// 512x2 ramp (T_VFX_LUT_Bomb_01) rather than the family's white pixel, and it
// is the only instance in the cookbook that does; the ramp stays LutWhite here
// pending [P1-D1], the open Gradient_Invert remap question, exactly as
// RainbowDisAdd does. All recorded in NS_Bomb_Projectile.md §13.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_FlatAdd used DIRECTLY — the fireball's two counter-twisting streamers draw through the parent
    // graph itself, not through an instance of it, so every value here is the parent's own default. The whole
    // material is ParticleColor x Brightness behind a depth fade, which makes this the simplest look in the
    // cookbook and the reason the trail's identity lives entirely in the behavior's colour curve.
    //
    // FlatAdd02 is the same shader at Brightness 10 on sprites and meshes; a ribbon needs its own master
    // because the usage flags are per-master, and the brightness differs anyway.
    asset TrailFlatAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/FlatAdd.ush";
        _UshFunctionName = n"CkUsf_Look_FlatAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"TrailFlatAdd";
        _TwoSided        = true;

        _UsedWithNiagaraRibbons = true;
        _ParticleColor          = true;

        _Parameters.Add(CkUsf::Usf_Scalar(n"Brightness", 1.0));
    }

    // M_VFX_DisAdd_Trail01 — the bomb's trail. Its dissolve pans along u at -1 per second while the behavior
    // drives the dissolve channel from +0.5 to -0.5, so the ribbon erodes from the head backwards rather than
    // fading uniformly.
    //
    // ShapeTex is WindBandMid rather than a new bake: T_VFX_Wind_02 IS T_VFX_Wind_03 rolled 141 of its 512
    // rows (measured to quantization), and WindBandMid already carries exactly that roll.
    asset TrailDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"TrailDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraRibbons   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "WindBandMid", "TileNoiseSparse",
            6.0,          // Brightness
            -1.0, 0.0,    // Dissolve_Speed — the only look in the cookbook that pans its dissolve at unit speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.3,          // Distortion_Intensity — LIVE
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",   // GradientMap — the source's T_VFX_LUT_Bomb_01, held white pending [P1-D1]
            0.1,          // GradientMap_Displacement
            0.0,          // Gradient_Invert
            0.1,          // Distortion_Scale — inherited from the reference
            "TileNoise"); // Distortion_Tex is T_VFX_Noise_02, distinct from this instance's dissolve paint
    }

    // M_VFX_DisAdd_Trail03 — the sparkle trail of NS_BuffCast, and the only look in the cookbook whose SHAPE,
    // COLOUR and DISSOLVE paints are all the same pure gradient: T_VFX_Gradient_02 is the ramp 1 - u, so the
    // strand simply darkens from head to tail with nothing painted on it at all. Its dissolve never pans, so
    // the fade is entirely the behavior's Emitter.Age-indexed alpha.
    //
    // FAMILY PARAMETERS IT DRIVES THAT THE LOOK DOES NOT PLUMB: Opacty_DepthFade (30). Everything else on the
    // instance resolves to the family reference. Recorded in NS_BuffCast.md §13.
    asset TrailDisAdd03 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"TrailDisAdd03";
        _TwoSided        = true;

        _UsedWithNiagaraRibbons   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "LinearRamp", "LinearRamp",
            4.0,          // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity — dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",   // GradientMap_Tex — the family's white pixel, so the gradient chain is inert here
            0.1,          // GradientMap_Displacement
            0.0,          // Gradient_Invert
            1.0,          // Distortion_Scale
            "TileNoise"); // Distortion_Tex is T_VFX_Noise_02, distinct from this instance's ramp
    }
}
