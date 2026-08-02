// Language=angelscript

//============================================================================
// VEFECTS "DissolveAdd" FAMILY — the dash parameterization
//============================================================================
//
// ONE look, because NS_Dash's other three material instances are already
// carried: M_VFX_DisAdd_Wind01 is CkUsf_CastLooks_Assets.as's WindDisAdd01,
// M_VFX_DisAdd_Wind02 is its WindDisAdd02Mesh, and M_VFX_DisAdd_Part02 is
// CkUsf_HitLooks_Assets.as's PartDisAdd02 — matched by INSTANCE against the
// corpus, not by name, and checked value-by-value in NS_Dash.md §10.
//
// M_VFX_DisAdd_Wind03 is unique to this system: no other system in the pack
// references it, and no other references SM_VFX_Ring04, the cone it draws on.
//
// The family entry point and its parameter helper are
// CkUsf_SlashLooks_Assets.as's — one shader, N parameterizations, because the
// source ships ONE parent graph (M_VFX_DissolveAdd) and one material INSTANCE
// per emitter. Nothing here is a second copy of the math. A delta table states
// the FULL inherited pair, never just the changed axis, so every value below is
// stated explicitly rather than defaulted.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Wind03 — the "speed cone" of NS_Dash, and the most heavily RE-TILED instance in the whole
    // pack: it stretches a wind paint 4 x 0.2 for its shape and 3 x 0.15 for its dissolve, which is what wraps
    // one texture into a long thin streak around the funnel. Its Core_Intensity of 20 is an order of magnitude
    // above anything else the cookbook measures.
    //
    // Drawn on a MESH renderer (SM_VFX_Ring04, recreated as the Cone carrier), so it takes the mesh-particle
    // usage flag rather than the sprite one.
    //
    // FAMILY PARAMETERS IT DRIVES THAT THE LOOK DOES NOT PLUMB: MainTex_Offset_Y (0.35), Dissolve_Offset
    // (0.5, 0.41), the whole separate Color_Tex chain (Color_Scale 3 x 0.15, Color_Offset 0.5 x 0.41,
    // Color_Speed_X -0.3 — and this is the one instance in the cookbook where folding Color_Tex into the shape
    // is NOT a no-op, because the two are tiled differently), Core_Intensity (20) and Opacty_DepthFade (20).
    // Recorded in NS_Dash.md §13.
    asset WindDisAdd03 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"WindDisAdd03";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "WindBandMid", "WindBandMid",
            2.0,          // Brightness
            -0.1, 0.0,    // Dissolve_Speed — U only
            0.0,          // Dissolve (static bias)
            3.0, 0.15,    // Dissolve_Scale — the streak tiling
            0.0,          // Distortion_Intensity — dead on this instance
            0.0, 0.0,     // Distortion_Speed
            4.0, 0.2,     // MainTex_Scale — the widest re-tile in the family
            1.0,          // Opacity_Boldness
            "LutWhite",
            0.1,
            0.0,          // Gradient_Invert
            1.0,          // Distortion_Scale — the source's X, inert behind Intensity 0
            "TileNoise"); // Distortion_Tex — the source's T_VFX_Noise_02, distinct from this Dissolve_Tex
    }
}
