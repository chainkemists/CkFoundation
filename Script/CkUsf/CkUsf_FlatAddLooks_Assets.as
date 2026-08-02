// Language=angelscript

//============================================================================
// VEFECTS "FlatAdd" FAMILY — the ported parameterizations
//============================================================================
//
// The second Vefects material family the cookbook carries. It shares ONE shader
// (/CkUsf/Looks/FlatAdd.ush) with everything derived from the source's
// M_VFX_FlatAdd parent graph, exactly as the DissolveAdd family does.
//
// NAMING TRAP, and the reason this file exists at all: the source instance is
// called M_VFX_DisAdd_Flat02 but its parent is M_VFX_FlatAdd, NOT
// M_VFX_DissolveAdd. Its corpus sidecar carries no dissolve, distortion or
// gradient parameters — only Brightness, a Color_Core it leaves at white, and
// two depth-fade scalars the CkUsf surface looks do not wire. Folding it into
// CkUsf_SlashLooks_Assets.as on the strength of the prefix would give it fifteen
// parameters it does not have.
//
//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Flat02 — Brightness 10 against the parent graph's own 1. Every other scalar the instance
    // carries resolves to the parent default, so this look is the family's one live delta.
    asset FlatAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/FlatAdd.ush";
        _UshFunctionName = n"CkUsf_Look_FlatAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"FlatAdd02";
        _TwoSided        = true;

        // No dynamic parameter: the family reads Particle Color and nothing else.
        // BOTH renderer usages: the source wears this instance on a MESH renderer (the spike pyramids of the two
        // hit ports) and a usage flag is about which renderer will accept the master, not what the shader reads —
        // a missing mesh-particle flag falls back to the default material in a packaged build only.
        _UsedWithNiagaraSprites       = true;
        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;

        _Parameters.Add(CkUsf::Usf_Scalar(n"Brightness", 10.0));
    }
}
