// Language=angelscript

//============================================================================
// CKUSF - CkVisualLod crossfade looks
//============================================================================
//
// The two halves of the CkVisualLod dithered crossfade. ONE fade alpha, two
// complementary masks over the SAME threshold expression
// (CkUsf_VisualLod_FadeThreshold in /CkUsf/Common.ush): the crowd look keeps
// pixels below the alpha, the near look keeps pixels at or above it, so the
// pair covers every pixel exactly once at every step.
//
// They differ ONLY in where the alpha comes from and which way the comparison
// runs. Far members are batched instances, so the crowd reads per-instance
// custom data ([0]/[1] are the batched frame bits - game data starts at [2],
// and 13 is what the arbiter's _FadeCustomDataSlot defaults to). A promoted
// near mesh is one SKMC, not an instance, so it reads custom PRIMITIVE data
// index 0 (the arbiter's _FadeNearCustomPrimitiveDataSlot).
//
// BaseColor is deliberately IDENTICAL on both: the two tiers have to read as
// one body, or the flip shows up as a colour change rather than as nothing.
// Generated masters: run `Ck_Usf_GenerateLooks` in the editor.
//============================================================================

namespace CkUsf
{
    // FAR: the batched crowd member. Solid by default (alpha 1) so an instance
    // nothing has written renders exactly as it would with no fade at all.
    asset VisualLodCrowdFade of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/VisualLodCrowdFade.ush";
        _UshFunctionName = n"CkUsf_Look_VisualLodCrowdFade";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        // Masked so the dither can clip pixels through OpacityMask. Single-sided
        // on purpose - the body stays single-sided, exactly as it renders unfaded.
        _BlendMode       = ECk_Usf_BlendMode::Masked;
        _LookName        = n"VisualLodCrowdFade";

        _UsedWithSkeletalMesh          = true;
        _UsedWithInstancedStaticMeshes = true;

        FCk_Usf_ParamDesc BaseColor;
        BaseColor._Name          = n"BaseColor";
        BaseColor._Type          = ECk_Usf_ParamType::Vector;
        BaseColor._DefaultVector = FLinearColor(0.30, 0.33, 0.38, 1.0);
        _Parameters.Add(BaseColor);

        // MUST stay LAST - param order IS the positional HLSL signature contract.
        FCk_Usf_ParamDesc FadeAlpha;
        FadeAlpha._Name            = n"FadeAlpha";
        FadeAlpha._Type            = ECk_Usf_ParamType::Scalar;
        FadeAlpha._DefaultScalar   = 1.0f;
        FadeAlpha._PerInstance     = true;
        FadeAlpha._PerInstanceSlot = 13;
        _Parameters.Add(FadeAlpha);
    }

    // NEAR: the promoted SKMC proxy. Solid by default (alpha 0) so an unwritten
    // custom-primitive-data slot - which reads 0 - renders the mesh whole.
    asset VisualLodNearFade of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/VisualLodNearFade.ush";
        _UshFunctionName = n"CkUsf_Look_VisualLodNearFade";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _BlendMode       = ECk_Usf_BlendMode::Masked;
        _LookName        = n"VisualLodNearFade";

        _UsedWithSkeletalMesh          = true;
        _UsedWithInstancedStaticMeshes = true;

        FCk_Usf_ParamDesc BaseColor;
        BaseColor._Name          = n"BaseColor";
        BaseColor._Type          = ECk_Usf_ParamType::Vector;
        BaseColor._DefaultVector = FLinearColor(0.30, 0.33, 0.38, 1.0);
        _Parameters.Add(BaseColor);

        // MUST stay LAST - param order IS the positional HLSL signature contract.
        FCk_Usf_ParamDesc FadeAlpha;
        FadeAlpha._Name = n"FadeAlpha";
        FadeAlpha._Type = ECk_Usf_ParamType::Scalar;
        FadeAlpha._DefaultScalar = 0.0f;
        FadeAlpha._CustomPrimitiveData = true;
        FadeAlpha._CustomPrimitiveDataIndex = 0;
        _Parameters.Add(FadeAlpha);
    }
}
