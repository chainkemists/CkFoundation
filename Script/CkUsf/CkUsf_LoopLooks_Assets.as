// Language=angelscript

//============================================================================
// VEFECTS "DissolveAdd" FAMILY — the idle-loop parameterizations
//============================================================================
//
// The fourth batch of DissolveAdd instances the cookbook carries. They share
// the family entry point and its parameter helper with
// CkUsf_SlashLooks_Assets.as, CkUsf_HitLooks_Assets.as and
// CkUsf_CastLooks_Assets.as — one shader, N parameterizations, because the
// source ships ONE parent graph (M_VFX_DissolveAdd) and one material INSTANCE
// per emitter. Nothing here is a second copy of the math.
//
// Only TWO looks, for four ports: the Pickup loop's ring and the arrow paint
// the Buff and Debuff loops share. Every other material those four systems
// drive is an instance an earlier batch already carries — NS_HealLoop in
// particular introduces nothing at all, which is what made it worth porting
// alongside its siblings rather than first.
//
// Every default below is quoted in a recipe's per-material delta table:
// CkFoundation/Source/CkParticles/Cookbook/NS_PickupLoop.md §4 and
// NS_BuffLoop.md §4. A delta table states the FULL inherited pair, never just
// the changed axis — anything a table does not list resolves to that sheet's
// REFERENCE instance, not to zero, so every value below is stated explicitly.
//
// FAMILY PARAMETERS THESE INSTANCES DRIVE THAT THE LOOK DOES NOT PLUMB:
// Core_Intensity, Color_CoreDifferent and Opacty_DepthFade (20 on the ring).
// Their chains are not reconstructible from the corpus — the parent graph is
// not exported, only an expression histogram — so they are recorded as known
// differences in each recipe's §13 rather than guessed at.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Ring03 — NS_PickupLoop's slow four-second halo, and the ONLY instance in that system with a
    // live distortion branch. It is also the only one that does not override Dissolve_Tex, so its erosion rides
    // the parent's coarse noise paint rather than its own shape — the ring dissolves against something other
    // than itself, which is what gives it a travelling edge instead of a uniform fade.
    asset RingDisAdd03 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"RingDisAdd03";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "RingUneven", "TileNoiseCoarse",
            10.0,         // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.5,          // Distortion_Intensity — LIVE, alone in its system
            0.1, 0.1,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0,          // Opacity_Boldness
            "LutWhite",   // GradientMap_Tex
            0.1,          // GradientMap_Displacement
            0.0,          // Gradient_Invert
            1.0,          // Distortion_Scale
            "TileNoise"); // Distortion_Tex, distinct from this instance's inherited Dissolve_Tex
    }

    // M_VFX_DisAdd_Arrows — the rising chevrons of NS_BuffLoop and the falling ones of NS_DebuffLoop. THREE source
    // emitters draw with it (Buff's Arrow, Debuff's Arrow_Green and Arrow_Purple), told apart by nothing but their
    // particle-colour curve: the cleanest illustration in the cookbook that a layer and a look are different things.
    //
    // Drawn on VELOCITY-ALIGNED sprite renderers, whose stretch axis is the quad's Y — which is why the chevron
    // paint fills only the upper half of its square rather than being centred.
    asset ArrowsDisAdd of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ArrowsDisAdd";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "ArrowChevron", "ArrowChevron",
            10.0,         // Brightness
            0.0, 0.0,     // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity — dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            1.0);         // Opacity_Boldness
    }
}
