// Language=angelscript

//============================================================================
// VEFECTS "Bomb" TOON-BANDED FAMILY — the ported parameterizations
//============================================================================
//
// The third Vefects material family, and the only one that draws a PROP rather
// than a particle: opaque, unlit, cel-shaded into flat colour bands.
// Shader: /CkUsf/Looks/ToonBand.ush.
//
// Every default below is the effective value on the source instance MI_VFX_Bomb
// (corpus sidecar materials/Vefects/Anime_VFX/Shared/Materials/Parents/MI_VFX_Bomb.json).
// Note Color_01 and Color_02 carry channels ABOVE 1 — the source authored this
// prop with HDR keys, and the look passes them through unclamped.
//
//============================================================================

namespace CkUsf
{
    // MI_VFX_Bomb — the bomb prop's shell. The band EDGES are not in the corpus (the source keeps them as
    // unnamed graph constants), so they start at an even three-way split.
    asset BombToon of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/ToonBand.ush";
        _UshFunctionName = n"CkUsf_Look_ToonBand";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Opaque;
        _LookName        = n"BombToon";

        // A prop mesh drawn as a mesh particle, so it takes the mesh-particle usage rather than the sprite one.
        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;

        _Parameters.Add(CkUsf::Usf_Vector(n"BandColorA",  FLinearColor(0.093,    0.819748, 3.0,      1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"BandColorB",  FLinearColor(0.228,    1.557,    2.0,      1.0)));
        _Parameters.Add(CkUsf::Usf_Vector(n"BandColorC",  FLinearColor(0.019065, 0.021971, 0.031,    1.0)));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdgeAB",  0.333333));
        _Parameters.Add(CkUsf::Usf_Scalar(n"BandEdgeBC",  0.666667));
        _Parameters.Add(CkUsf::Usf_Vector(n"ShadowColor", FLinearColor(0.635442, 0.724198, 1.0,      1.0)));
        _Parameters.Add(CkUsf::Usf_Scalar(n"ShadowAmount", 0.6));
        _Parameters.Add(CkUsf::Usf_Vector(n"MainColor",   FLinearColor(1.0,      1.0,      1.0,      1.0)));
        _Parameters.Add(CkUsf::Usf_Scalar(n"Glow",         2.0));
    }
}
