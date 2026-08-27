// Language=angelscript

//============================================================================
// VEFECTS "FresnelBomb" FAMILY - the ported parameterizations
//============================================================================
//
// The fourth Vefects material family the cookbook carries, and the explosion
// batch's shell layer: a translucent bubble coloured by a Fresnel lerp between
// an interior tint and a grazing-angle tint, eroded by a panning noise.
// Shader: /CkUsf/Looks/FresnelBomb.ush.
//
// It is NOT a DissolveAdd parameterization - the source ships a separate parent
// graph (M_VFX_FresnelBomb) with no Main_Tex, no Color_Tex, no distortion and no
// gradient chain, and it replaces the whole colour chain with the Fresnel lerp
// (NS_Bomb_Explosion.md Sec.4.3).
//
// Every default below is a cell of that sheet's Sec.4.3 delta table, which states
// all three instances for every parameter - nothing here is inherited implicitly.
//
//============================================================================

namespace CkUsf
{
    // The family's parameter list, in the ORDER CkUsf_Look_FresnelBomb declares them - the validator enforces
    // that order positionally, so this function is the single place it is stated.
    TArray<FCk_Usf_ParamDesc> Usf_FresnelBombParams(
        FString      InDissolveTexture,
        FLinearColor InColorInt,
        FLinearColor InColorExt,
        float        InFresnelExponent,
        float        InFresnelBaseReflect,
        float        InBrightness,
        float        InDissolveBias,
        float        InDissolveScaleX,
        float        InDissolveScaleY,
        float        InDissolveSpeedX,
        float        InDissolveSpeedY)
    {
        auto Params = TArray<FCk_Usf_ParamDesc>();

        Params.Add(CkUsf::Usf_ParticlesTexture(n"DissolveTex", InDissolveTexture));
        Params.Add(CkUsf::Usf_Vector(n"ColorInt", InColorInt));
        Params.Add(CkUsf::Usf_Vector(n"ColorExt", InColorExt));
        Params.Add(CkUsf::Usf_Scalar(n"FresnelExponent", InFresnelExponent));
        Params.Add(CkUsf::Usf_Scalar(n"FresnelBaseReflect", InFresnelBaseReflect));
        Params.Add(CkUsf::Usf_Scalar(n"Brightness", InBrightness));
        // Erosion edge softness. The source cuts with a pair of Step nodes whose thresholds are unnamed graph
        // constants, so the value is INFERRED - held at the DissolveAdd family's own 0.15 so both cookbook
        // families feather identically rather than carrying two readings of one idea.
        Params.Add(CkUsf::Usf_Scalar(n"DissolveEdge", 0.15));
        Params.Add(CkUsf::Usf_Scalar(n"DissolveBias", InDissolveBias));
        Params.Add(CkUsf::Usf_Vector(n"DissolveScale", FLinearColor(InDissolveScaleX, InDissolveScaleY, 0.0, 1.0)));
        Params.Add(CkUsf::Usf_Vector(n"DissolveSpeed", FLinearColor(InDissolveSpeedX, InDissolveSpeedY, 0.0, 1.0)));

        return Params;
    }
}

//============================================================================

namespace CkUsf
{
    // MI_VFX_FresnelBomb_FirstExplo01 - worn by Bubble_First_Explo. The only instance with a live base-reflect
    // floor (0.1) and the only one carrying a static dissolve bias (AddDiss -0.6).
    asset ExpFresnelBomb01 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/FresnelBomb.ush";
        _UshFunctionName = n"CkUsf_Look_FresnelBomb";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ExpFresnelBomb01";

        // All three ride MESH renderers (the five Bubble emitters are mesh particles), so this family takes the
        // mesh-particle usage and never the sprite one.
        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        // The source declares the identical four channel names as the DissolveAdd parent graph (Sec.4.3); this
        // family reads only the first of them.
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_FresnelBombParams(
            // T_VFX_Noise_05, MEASURED at the port (NS_Bomb_Explosion.md Sec.7): rejected against all four existing
            // noise bakes' sources - best correlation at any roll 0.11 - so it carries its own TileNoiseFine bake.
            "TileNoiseFine",
            FLinearColor(2.0, 2.20833, 2.5, 1.0),   // Color_Int
            FLinearColor(0.0, 0.566667, 1.0, 1.0),  // Color_Ext
            0.5,                                     // Fresnel_01_Expo
            0.1,                                     // Fresnel_01_BaseReflec
            7.0,                                     // Brightness
            -0.6,                                    // AddDiss
            1.0, 1.4,                                // Dissolve_Scale
            0.0, 0.3);                               // Dissolve_Speed
    }

    // MI_VFX_FresnelBomb_FirstExplo02 - worn by Bubble_Noise02. Same two colours as FirstExplo01; the deltas
    // are a dimmer Brightness, a sharper Fresnel and a faster V pan.
    asset ExpFresnelBomb02 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/FresnelBomb.ush";
        _UshFunctionName = n"CkUsf_Look_FresnelBomb";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ExpFresnelBomb02";

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_FresnelBombParams(
            "TileNoiseFine",
            FLinearColor(2.0, 2.20833, 2.5, 1.0),
            FLinearColor(0.0, 0.566667, 1.0, 1.0),
            2.0,
            0.0,
            5.0,
            0.0,
            1.0, 1.4,
            0.0, 0.5);
    }

    // MI_VFX_FresnelBomb_BubbleNoise02 - worn by Bubble_Fresnel. The family's outlier: a deep-blue interior
    // against a brighter exterior, the sharpest Fresnel of the three, and a dissolve stretched 5x along U.
    asset ExpFresnelBomb03 of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/FresnelBomb.ush";
        _UshFunctionName = n"CkUsf_Look_FresnelBomb";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"ExpFresnelBomb03";

        _UsedWithNiagaraMeshParticles = true;
        _ParticleColor                = true;
        _ParticleDynamicParameter     = true;
        _ParticleDynamicParameterNames = CkUsf::Usf_DissolveAddChannelNames();

        _Parameters = CkUsf::Usf_FresnelBombParams(
            "TileNoiseFine",
            FLinearColor(0.0, 0.05,     1.5, 1.0),
            FLinearColor(0.0, 0.166665, 2.0, 1.0),
            3.0,
            0.0,
            7.0,
            0.0,
            0.2, 2.0,
            0.0, 0.75);
    }
}
