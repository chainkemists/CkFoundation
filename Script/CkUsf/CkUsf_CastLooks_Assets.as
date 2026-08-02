// Language=angelscript

//============================================================================
// VEFECTS "DissolveAdd" FAMILY — the cast/spawn parameterizations
//============================================================================
//
// The third batch of DissolveAdd instances the cookbook carries. They share the
// family entry point and its parameter helper with CkUsf_SlashLooks_Assets.as
// and CkUsf_HitLooks_Assets.as — one shader, N parameterizations, because the
// source ships ONE parent graph (M_VFX_DissolveAdd) and one material INSTANCE
// per emitter. Nothing here is a second copy of the math.
//
// Every default below is quoted in a recipe's per-material delta table:
// CkFoundation/Source/CkParticles/Cookbook/NS_Arrow_Cast.md §4.1,
// NS_Bomb_Spawn.md §4.1, NS_HealCast.md §4, NS_Gunshot_Cast.md §4.1 and
// NS_Lightning_Cast.md §4. A delta table states the FULL inherited pair, never
// just the changed axis — anything a table does not list resolves to the family
// REFERENCE instance (M_VFX_DisAdd_Part01), not to zero, so every value below is
// stated explicitly rather than defaulted.
//
// NS_Arrow_Hit adds NO look of its own: its ten materials are all carried by
// this file, the hit file or the slash file already. That is what made porting
// the Cast variant first worth the ordering.
//
// FAMILY PARAMETERS THESE INSTANCES DRIVE THAT THE LOOK DOES NOT PLUMB:
// Core_Intensity (1 on Wind01), Color_Speed_X (-0.3 on Wind02), a per-axis
// Distortion_Scale (Wind02 resolves X 1 / Y 0.6 where the look carries one
// scalar), Opacty_DepthFade and Opacty_StepAdd. Their chains are not
// reconstructible from the corpus — the parent graph is not exported, only an
// expression histogram — so they are recorded as known differences in each
// recipe's §13 rather than guessed at.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Star02 — the second four-point star, on its own paint rather than Star_01's. Measured
    // against the StarFour bake and rejected: this one has a saturated core a tenth of the radius wide behind
    // rays that stay bright out to r 0.85, where Star_01's core fills half the texture.
    asset StarDisAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"StarDisAdd02";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "StarFourTight", "StarFourTight",
            6.0,          // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity — dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",   // GradientMap_Tex
            0.1,          // GradientMap_Displacement
            0.0);         // Gradient_Invert
    }

    // M_VFX_DisAdd_Star03 — the lens-flare streak star of NS_Bomb_Spawn, and the only paint in the pack whose
    // horizontal rays reach twice as far as its vertical ones.
    asset StarDisAdd03 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"StarDisAdd03";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "StarFourSplit", "StarFourSplit",
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

    // M_VFX_DisAdd_Part03 — the mid-brightness streak paint of the bomb's stretched flares, on the exponential
    // Part_03 falloff its Bright sibling already established.
    asset PartDisAdd03 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd03";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticleFine", "SoftParticleFine",
            3.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0,
            1.0);
    }

    // M_VFX_DisAdd_Wind01 — the sub-UV wind puffs of NS_Arrow_Cast. Its shape is a 2x2 flipbook, divided by the
    // row renderer's SubImageSize rather than by anything in this look, and its dissolve rides a SEPARATE noise
    // paint that pans down V — the only instance in the cookbook with a one-axis dissolve pan.
    asset WindDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"WindDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "WindSheet", "TileNoise",
            3.0,          // Brightness
            0.0, -0.15,   // Dissolve_Speed — V only
            0.0,
            1.0, 1.0,
            0.5,          // Distortion_Intensity — LIVE
            0.0, 0.0,
            1.0, 1.0,
            1.0,          // Opacity_Boldness
            "LutWhite",
            0.1,
            0.0);         // Gradient_Invert
    }

    // M_VFX_DisAdd_Wind02 — the travelling tube of NS_Arrow_Cast, and the brightest wind instance in the pack.
    // NAMING TRAP: a look called WindDisAdd02 already exists (NS_BasicAttack, from M_VFX_DisAdd_Pan_Wind02) and
    // is a DIFFERENT source material; this one takes the Mesh suffix rather than colliding with it.
    //
    // Drawn on a MESH renderer, so it takes the mesh-particle usage flag rather than the sprite one.
    asset WindDisAdd02Mesh of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"WindDisAdd02Mesh";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "WindBandMid", "WindBandMid",
            7.0,          // Brightness
            -0.1, 0.0,    // Dissolve_Speed — U only
            0.0,
            0.7, 0.95,    // Dissolve_Scale
            1.0,          // Distortion_Intensity — LIVE, and the strongest in the cookbook
            0.1, 0.1,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",
            0.1,
            0.0,          // Gradient_Invert
            1.0,          // Distortion_Scale — the source's X; its Y of 0.6 has no home in the signature
            "TileNoise"); // Distortion_Tex, the family default, distinct from this instance's Dissolve_Tex
    }

    // M_VFX_DisAdd_Part07 — the lens-flare streaks of NS_HealCast, and the ONLY instance in the cookbook that is
    // both a 2x2 flipbook AND drawn on a velocity-aligned quad: its sheet is divided by the row renderer's
    // SubImageSize, and its long axis tracks the particle's climb rather than the screen.
    //
    // Its CamOffset of 30 is the family's camera-ward world-position push and is not plumbed here — see
    // NS_HealCast.md §13.
    asset PartDisAdd07 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd07";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "LensSheet", "LensSheet",
            2.0,          // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity — dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",   // GradientMap_Tex
            0.1,          // GradientMap_Displacement
            0.0);         // Gradient_Invert
    }

    // M_VFX_DisAdd_LightStrip on a SPRITE renderer. Same instance, same parameters, as LightStripDisAdd in
    // CkUsf_HitLooks_Assets.as — the Arrow systems draw it on SM_VFX_Plane01 while NS_Gunshot_Cast and
    // NS_FireBall_Cast draw it as a velocity-aligned sprite, and a material declares its Niagara usage per
    // renderer class. A mesh-only master bound to a sprite renderer silently falls back to the default
    // material, so the two carriers need two masters; the parameters below are the same eleven values.
    asset LightStripDisAddSprite of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"LightStripDisAddSprite";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "LightStrip", "LightStrip",
            7.0,          // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity — dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0);         // Opacity_Boldness
    }

    // M_VFX_DisAdd_Lightning02 — the bolt sheet of NS_Lightning_Cast. Two things make it the odd instance of the
    // whole family: it is the ONLY one whose distortion branch is live AND panning fast (Intensity 0.5 at speed
    // 0.7 on both axes), and its Core_Power resolves to 0. Its shape is a 2x2 flipbook, divided by the row
    // renderer's SubImageSize rather than by anything here.
    //
    // Core_Power 0 has no home in the look's signature — recorded in NS_Lightning_Cast.md §13.
    asset LightningDisAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"LightningDisAdd02";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "LightningSheet", "LightningSheet",
            15.0,             // Brightness
            0.0, 0.0,         // Dissolve_Speed
            0.0,              // Dissolve (static bias)
            1.0, 1.0,         // Dissolve_Scale
            0.5,              // Distortion_Intensity — LIVE
            0.7, 0.7,         // Distortion_Speed — the fastest pan in the cookbook
            1.0, 1.0,         // MainTex_Scale
            1.0,              // Opacity_Boldness
            "LutWhite",
            0.1,
            0.0,              // Gradient_Invert — inherited from the Ring04 family reference
            1.0,              // Distortion_Scale
            "TileNoiseCoarse"); // Distortion_Tex — the source's T_VFX_Noise_04, distinct from Dissolve_Tex
    }
}
