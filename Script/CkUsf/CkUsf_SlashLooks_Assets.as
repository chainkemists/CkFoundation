// Language=angelscript

//============================================================================
// VEFECTS "DissolveAdd" FAMILY — the ported parameterizations
//============================================================================
//
// All of them share ONE shader (/CkUsf/Looks/DissolveAdd.ush) because the source
// ships one parent graph (M_VFX_DissolveAdd) and one material INSTANCE per
// emitter. A look here is therefore a set of parameter defaults, read out of
// the extracted corpus — never a second copy of the math.
//
// Every default below is quoted in a recipe's per-material delta table:
// CkFoundation/Source/CkParticles/Cookbook/NS_BasicAttack.md §4 (the five Slash
// looks) and NS_Gunshot_Projectile.md / NS_Arrow_Projectile.md §4 (PartDisAdd01).
//
// Textures are the plugin's own PROCEDURAL stand-ins, parameterized from
// measured characteristics of the source paints (recipe §7). No pack art is
// imported for this effect.
//
//============================================================================

namespace CkUsf
{
    FCk_Usf_ParamDesc Usf_Scalar(FName InName, float InDefault)
    {
        FCk_Usf_ParamDesc Param;
        Param._Name = InName;
        Param._Type = ECk_Usf_ParamType::Scalar;
        Param._DefaultScalar = InDefault;
        return Param;
    }

    FCk_Usf_ParamDesc Usf_Vector(FName InName, FLinearColor InDefault)
    {
        FCk_Usf_ParamDesc Param;
        Param._Name = InName;
        Param._Type = ECk_Usf_ParamType::Vector;
        Param._DefaultVector = InDefault;
        return Param;
    }

    FCk_Usf_ParamDesc Usf_ParticlesTexture(FName InName, FString InTextureName)
    {
        FCk_Usf_ParamDesc Param;
        Param._Name = InName;
        Param._Type = ECk_Usf_ParamType::Texture2D;
        Param._DefaultTexturePath =
            f"/CkFoundation/CkParticles/Textures/T_CkParticles_{InTextureName}.T_CkParticles_{InTextureName}";
        return Param;
    }

    // The family's parameter list, in the ORDER CkUsf_Look_DissolveAdd declares them — the validator
    // enforces that order positionally, so this function is the single place it is stated.
    TArray<FCk_Usf_ParamDesc> Usf_DissolveAddParams(
        FString InShapeTexture,
        FString InDissolveTexture,
        float   InBrightness,
        float   InDissolveSpeedX,
        float   InDissolveSpeedY,
        float   InDissolveBias,
        float   InDissolveScaleX,
        float   InDissolveScaleY,
        float   InDistortIntensity,
        float   InDistortSpeedX,
        float   InDistortSpeedY,
        float   InMainTexScaleX,
        float   InMainTexScaleY,
        // Trailing so the five NS_BasicAttack looks, which all resolve the family default, stay unchanged.
        float   InOpacityBoldness = 1.0,
        // The gradient-map chain. Its defaults are the PARENT graph's (a flat white ramp, displacement 0.1,
        // invert 0.5), so every look that does not name one renders exactly as it did before the chain existed.
        FString InGradientTexture = "LutWhite",
        float   InGradientMapDisplacement = 0.1,
        float   InGradientInvert = 0.5,
        // Distortion_Scale, and a Distortion_Tex that differs from Dissolve_Tex. Both are trailing and default to
        // what every look authored before them already resolved, so none of those regenerates changed. The two
        // Vefects hit instances that drive a LIVE distortion branch (Flames01, Smoke01) are the first to need them.
        float   InDistortScale = 0.1,
        FString InDistortTexture = "")
    {
        auto Params = TArray<FCk_Usf_ParamDesc>();

        Params.Add(CkUsf::Usf_ParticlesTexture(n"ShapeTex",    InShapeTexture));
        Params.Add(CkUsf::Usf_ParticlesTexture(n"DissolveTex", InDissolveTexture));
        // Distortion_Tex is the same asset as Dissolve_Tex on all but two instances in this family.
        Params.Add(CkUsf::Usf_ParticlesTexture(n"DistortTex",
            InDistortTexture.IsEmpty() ? InDissolveTexture : InDistortTexture));

        Params.Add(CkUsf::Usf_Vector(n"CoreColor", FLinearColor(1.0, 1.0, 1.0, 1.0)));
        Params.Add(CkUsf::Usf_Scalar(n"Brightness", InBrightness));
        Params.Add(CkUsf::Usf_Scalar(n"DissolveSpeed", InDissolveSpeedX));
        // Erosion edge softness — the source expresses this through a SmoothStep in the parent graph, so the
        // value is inferred rather than read. Shared with RingDissolveAdd.
        Params.Add(CkUsf::Usf_Scalar(n"DissolveEdge", 0.15));
        Params.Add(CkUsf::Usf_Scalar(n"DistortScale", InDistortScale));
        Params.Add(CkUsf::Usf_Scalar(n"OpacityBoldness", InOpacityBoldness));

        Params.Add(CkUsf::Usf_Scalar(n"DissolveSpeedY", InDissolveSpeedY));
        Params.Add(CkUsf::Usf_Scalar(n"DissolveBias", InDissolveBias));
        Params.Add(CkUsf::Usf_Vector(n"DissolveScale", FLinearColor(InDissolveScaleX, InDissolveScaleY, 0.0, 1.0)));
        Params.Add(CkUsf::Usf_Scalar(n"DistortIntensity", InDistortIntensity));
        Params.Add(CkUsf::Usf_Vector(n"DistortSpeed", FLinearColor(InDistortSpeedX, InDistortSpeedY, 0.0, 1.0)));
        Params.Add(CkUsf::Usf_Vector(n"MainTexScale", FLinearColor(InMainTexScaleX, InMainTexScaleY, 0.0, 1.0)));

        Params.Add(CkUsf::Usf_ParticlesTexture(n"GradientMap", InGradientTexture));
        Params.Add(CkUsf::Usf_Scalar(n"GradientMapDisplacement", InGradientMapDisplacement));
        Params.Add(CkUsf::Usf_Scalar(n"GradientInvert", InGradientInvert));

        return Params;
    }

    TArray<FString> Usf_DissolveAddChannelNames()
    {
        auto Names = TArray<FString>();
        Names.Add("dissolve");
        Names.Add("distortion");
        Names.Add("offset");
        Names.Add("core_color");
        return Names;
    }
}

//============================================================================

namespace CkUsf
{
    // M_VFX_DisAdd_Slash01 — the reference instance. The only one in the family with a live distortion
    // branch (Distortion_Intensity 1) and a two-axis dissolve pan.
    asset SlashDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"SlashDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SlashArc01", "TileNoise",
            5.0,          // Brightness
            0.3, -0.1,    // Dissolve_Speed
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            1.0,          // Distortion_Intensity
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0);    // MainTex_Scale
    }

    // M_VFX_DisAdd_Slash02 — the hot core line: Brightness 40, no dissolve pan, no distortion.
    asset SlashDisAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"SlashDisAdd02";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SlashArc02", "SoftParticle",
            40.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0);
    }

    // M_VFX_DisAdd_Slash04 — worn by the emitter named Slash_03 (a naming skew in the source that is
    // deliberately preserved). Carries a -0.1 static dissolve bias and a stretched main-tex V.
    asset SlashDisAdd04 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"SlashDisAdd04";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticle", "SoftParticle",
            5.0,
            -0.1, -0.1,
            -0.1,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.2);
    }

    // M_VFX_DisAdd_Pan_Wind02 — the dim wind ghost: Brightness 1, a shrunken dissolve tile and the only
    // instance whose distortion UVs pan.
    asset WindDisAdd02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"WindDisAdd02";
        _TwoSided        = true;

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticle", "WindBand",
            1.0,
            -0.1, 0.0,
            0.0,
            0.7, 0.6,
            0.0,
            0.1, 0.1,
            1.0, 1.0);
    }

    // M_VFX_DisAdd_Part01 — the reference instance of the whole family: every scalar sits at the parent's
    // default. Worn by the Glow_01 head of both projectile recreations (behaviors 18 and 19), on a
    // velocity-aligned sprite for the Gunshot and on the shared camera sprite for the Arrow.
    //
    // OpacityBoldness 0.5 is the first value in the cookbook that is NOT 1.0, and the family shader multiplies
    // it straight into Opacity — so it halves this layer's coverage rather than being inert.
    asset PartDisAdd01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd01";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SoftParticle", "SoftParticle",
            1.0,          // Brightness
            0.0, 0.0,     // Dissolve_Speed — static erosion, the pattern never pans
            0.0,          // Dissolve (static bias)
            1.0, 1.0,     // Dissolve_Scale
            0.0,          // Distortion_Intensity — the distortion branch is dead on this instance
            0.0, 0.0,     // Distortion_Speed
            1.0, 1.0,     // MainTex_Scale
            0.5);         // Opacity_Boldness
    }

    // M_VFX_DisAdd_Part04 — the spark streaks, and the two tails of both projectile recreations. On a SPRITE
    // renderer, so it takes the sprite usage flag rather than the mesh-particle one.
    asset PartDisAdd04 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DissolveAdd.ush";
        _UshFunctionName = n"CkUsf_Look_DissolveAdd";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"PartDisAdd04";
        _TwoSided        = true;

        _UsedWithNiagaraSprites   = true;
        _ParticleColor            = true;
        _ParticleDynamicParameter = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_DissolveAddParams(
            "SparkStreak", "SparkStreak",
            6.0,
            0.0, 0.0,
            0.0,
            1.0, 1.0,
            0.0,
            0.0, 0.0,
            1.0, 1.0);
    }
}
