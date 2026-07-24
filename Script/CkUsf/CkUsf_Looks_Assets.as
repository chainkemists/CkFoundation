namespace CkUsf
{
    asset Hologram of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Hologram.ush";
        _UshFunctionName = n"CkUsf_Look_Hologram";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Hologram";

        FCk_Usf_ParamDesc Tint;
        Tint._Name = n"TintColor";
        Tint._Type = ECk_Usf_ParamType::Vector;
        Tint._DefaultVector = FLinearColor(0.2, 0.8, 1.0, 1.0);
        _Parameters.Add(Tint);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"ScanSpeed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 3.0;
        _Parameters.Add(Speed);
    }

    asset Plasma of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Plasma.ush";
        _UshFunctionName = n"CkUsf_Look_Plasma";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Plasma";

        FCk_Usf_ParamDesc ColorA;
        ColorA._Name = n"ColorA";
        ColorA._Type = ECk_Usf_ParamType::Vector;
        ColorA._DefaultVector = FLinearColor(1.0, 0.1, 0.4, 1.0);
        _Parameters.Add(ColorA);

        FCk_Usf_ParamDesc ColorB;
        ColorB._Name = n"ColorB";
        ColorB._Type = ECk_Usf_ParamType::Vector;
        ColorB._DefaultVector = FLinearColor(0.1, 0.6, 1.0, 1.0);
        _Parameters.Add(ColorB);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.5;
        _Parameters.Add(Speed);
    }

    asset Voronoi of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Voronoi.ush";
        _UshFunctionName = n"CkUsf_Look_Voronoi";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Voronoi";

        FCk_Usf_ParamDesc CellColor;
        CellColor._Name = n"CellColor";
        CellColor._Type = ECk_Usf_ParamType::Vector;
        CellColor._DefaultVector = FLinearColor(0.2, 1.0, 0.6, 1.0);
        _Parameters.Add(CellColor);

        FCk_Usf_ParamDesc Scale;
        Scale._Name = n"Scale";
        Scale._Type = ECk_Usf_ParamType::Scalar;
        Scale._DefaultScalar = 8.0;
        _Parameters.Add(Scale);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    asset Julia of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Julia.ush";
        _UshFunctionName = n"CkUsf_Look_Julia";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Julia";

        FCk_Usf_ParamDesc ColorInner;
        ColorInner._Name = n"ColorInner";
        ColorInner._Type = ECk_Usf_ParamType::Vector;
        ColorInner._DefaultVector = FLinearColor(1.0, 0.5, 0.1, 1.0);
        _Parameters.Add(ColorInner);

        FCk_Usf_ParamDesc ColorOuter;
        ColorOuter._Name = n"ColorOuter";
        ColorOuter._Type = ECk_Usf_ParamType::Vector;
        ColorOuter._DefaultVector = FLinearColor(0.1, 0.2, 0.8, 1.0);
        _Parameters.Add(ColorOuter);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    asset FbmWarp of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/FbmWarp.ush";
        _UshFunctionName = n"CkUsf_Look_FbmWarp";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"FbmWarp";

        FCk_Usf_ParamDesc ColorLow;
        ColorLow._Name = n"ColorLow";
        ColorLow._Type = ECk_Usf_ParamType::Vector;
        ColorLow._DefaultVector = FLinearColor(0.05, 0.0, 0.1, 1.0);
        _Parameters.Add(ColorLow);

        FCk_Usf_ParamDesc ColorHigh;
        ColorHigh._Name = n"ColorHigh";
        ColorHigh._Type = ECk_Usf_ParamType::Vector;
        ColorHigh._DefaultVector = FLinearColor(1.0, 0.7, 0.2, 1.0);
        _Parameters.Add(ColorHigh);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    asset Seascape of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Seascape.ush";
        _UshFunctionName = n"CkUsf_Look_Seascape";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Seascape";

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    asset Aiekick of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Aiekick.ush";
        _UshFunctionName = n"CkUsf_Look_Aiekick";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Aiekick";

        FCk_Usf_ParamDesc Cube;
        Cube._Name = n"iChannel0";
        Cube._Type = ECk_Usf_ParamType::TextureCube;
        Cube._DefaultTexturePath = "/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap";
        _Parameters.Add(Cube);

        FCk_Usf_ParamDesc Noise;
        Noise._Name = n"iChannel1";
        Noise._Type = ECk_Usf_ParamType::Texture2D;
        Noise._DefaultTexturePath = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq.Good64x64TilingNoiseHighFreq";
        _Parameters.Add(Noise);
    }

    asset Truchet of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Truchet.ush";
        _UshFunctionName = n"CkUsf_Look_Truchet";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Truchet";

        FCk_Usf_ParamDesc ColorA;
        ColorA._Name = n"ColorA";
        ColorA._Type = ECk_Usf_ParamType::Vector;
        ColorA._DefaultVector = FLinearColor(0.1, 0.9, 1.0, 1.0);
        _Parameters.Add(ColorA);

        FCk_Usf_ParamDesc ColorB;
        ColorB._Name = n"ColorB";
        ColorB._Type = ECk_Usf_ParamType::Vector;
        ColorB._DefaultVector = FLinearColor(1.0, 0.3, 0.8, 1.0);
        _Parameters.Add(ColorB);

        FCk_Usf_ParamDesc Scale;
        Scale._Name = n"Scale";
        Scale._Type = ECk_Usf_ParamType::Scalar;
        Scale._DefaultScalar = 8.0;
        _Parameters.Add(Scale);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    asset Starfield of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Starfield.ush";
        _UshFunctionName = n"CkUsf_Look_Starfield";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Starfield";

        FCk_Usf_ParamDesc StarColor;
        StarColor._Name = n"StarColor";
        StarColor._Type = ECk_Usf_ParamType::Vector;
        StarColor._DefaultVector = FLinearColor(0.9, 0.95, 1.0, 1.0);
        _Parameters.Add(StarColor);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);

        FCk_Usf_ParamDesc Density;
        Density._Name = n"Density";
        Density._Type = ECk_Usf_ParamType::Scalar;
        Density._DefaultScalar = 1.0;
        _Parameters.Add(Density);
    }

    asset Caustics of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Caustics.ush";
        _UshFunctionName = n"CkUsf_Look_Caustics";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Caustics";

        FCk_Usf_ParamDesc WaterColor;
        WaterColor._Name = n"WaterColor";
        WaterColor._Type = ECk_Usf_ParamType::Vector;
        WaterColor._DefaultVector = FLinearColor(0.1, 0.6, 0.8, 1.0);
        _Parameters.Add(WaterColor);

        FCk_Usf_ParamDesc Scale;
        Scale._Name = n"Scale";
        Scale._Type = ECk_Usf_ParamType::Scalar;
        Scale._DefaultScalar = 12.0;
        _Parameters.Add(Scale);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    asset RimGlow of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/RimGlow.ush";
        _UshFunctionName = n"CkUsf_Look_RimGlow";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"RimGlow";

        FCk_Usf_ParamDesc RimColor;
        RimColor._Name = n"RimColor";
        RimColor._Type = ECk_Usf_ParamType::Vector;
        RimColor._DefaultVector = FLinearColor(0.2, 0.7, 1.0, 1.0);
        _Parameters.Add(RimColor);

        FCk_Usf_ParamDesc RimPower;
        RimPower._Name = n"RimPower";
        RimPower._Type = ECk_Usf_ParamType::Scalar;
        RimPower._DefaultScalar = 3.0;
        _Parameters.Add(RimPower);
    }

    asset Dissolve of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Dissolve.ush";
        _UshFunctionName = n"CkUsf_Look_Dissolve";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _BlendMode       = ECk_Usf_BlendMode::Masked;
        _TwoSided        = true;
        _LookName        = n"Dissolve";

        FCk_Usf_ParamDesc BaseColor;
        BaseColor._Name = n"BaseColor";
        BaseColor._Type = ECk_Usf_ParamType::Vector;
        BaseColor._DefaultVector = FLinearColor(0.30, 0.35, 0.40, 1.0);
        _Parameters.Add(BaseColor);

        FCk_Usf_ParamDesc EdgeColor;
        EdgeColor._Name = n"EdgeColor";
        EdgeColor._Type = ECk_Usf_ParamType::Vector;
        EdgeColor._DefaultVector = FLinearColor(1.0, 0.4, 0.05, 1.0);
        _Parameters.Add(EdgeColor);

        FCk_Usf_ParamDesc Scale;
        Scale._Name = n"Scale";
        Scale._Type = ECk_Usf_ParamType::Scalar;
        Scale._DefaultScalar = 5.0;
        _Parameters.Add(Scale);
    }

    asset LitMetal of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/LitMetal.ush";
        _UshFunctionName = n"CkUsf_Look_LitMetal";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _LookName        = n"LitMetal";

        FCk_Usf_ParamDesc ColorA;
        ColorA._Name = n"ColorA";
        ColorA._Type = ECk_Usf_ParamType::Vector;
        ColorA._DefaultVector = FLinearColor(0.90, 0.70, 0.30, 1.0);
        _Parameters.Add(ColorA);

        FCk_Usf_ParamDesc ColorB;
        ColorB._Name = n"ColorB";
        ColorB._Type = ECk_Usf_ParamType::Vector;
        ColorB._DefaultVector = FLinearColor(0.20, 0.25, 0.30, 1.0);
        _Parameters.Add(ColorB);

        FCk_Usf_ParamDesc Tiles;
        Tiles._Name = n"Tiles";
        Tiles._Type = ECk_Usf_ParamType::Scalar;
        Tiles._DefaultScalar = 6.0;
        _Parameters.Add(Tiles);
    }

    asset Displace of UCkUsf_LookDefinition
    {
        _UshIncludePath   = "/CkUsf/Looks/Displace.ush";
        _UshFunctionName  = n"CkUsf_Look_Displace";
        _WpoFunctionName  = n"CkUsf_Look_Displace_WPO";
        _Domain           = ECk_Usf_Domain::SurfaceLit;
        _LookName         = n"Displace";

        FCk_Usf_ParamDesc BaseColor;
        BaseColor._Name = n"BaseColor";
        BaseColor._Type = ECk_Usf_ParamType::Vector;
        BaseColor._DefaultVector = FLinearColor(0.20, 0.55, 0.85, 1.0);
        _Parameters.Add(BaseColor);

        FCk_Usf_ParamDesc Amplitude;
        Amplitude._Name = n"Amplitude";
        Amplitude._Type = ECk_Usf_ParamType::Scalar;
        Amplitude._DefaultScalar = 12.0;
        _Parameters.Add(Amplitude);

        FCk_Usf_ParamDesc Frequency;
        Frequency._Name = n"Frequency";
        Frequency._Type = ECk_Usf_ParamType::Scalar;
        Frequency._DefaultScalar = 0.05;
        _Parameters.Add(Frequency);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 3.0;
        _Parameters.Add(Speed);
    }

    asset PerInstanceHue of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/PerInstanceHue.ush";
        _UshFunctionName = n"CkUsf_Look_PerInstanceHue";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"PerInstanceHue";

        // Per-instance scalar: on an ISM each instance writes its own slot-0 value → distinct colours from ONE material.
        FCk_Usf_ParamDesc Hue;
        Hue._Name = n"Hue";
        Hue._Type = ECk_Usf_ParamType::Scalar;
        Hue._DefaultScalar = 0.0;
        Hue._PerInstance = true;
        _Parameters.Add(Hue);

        FCk_Usf_ParamDesc Brightness;
        Brightness._Name = n"Brightness";
        Brightness._Type = ECk_Usf_ParamType::Scalar;
        Brightness._DefaultScalar = 1.5;
        _Parameters.Add(Brightness);
    }

    asset Glass of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Glass.ush";
        _UshFunctionName = n"CkUsf_Look_Glass";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        // Lit translucency defaults to volumetric non-directional, which reads flat on glass —
        // forward per-pixel lighting is the mode glass-like surfaces want.
        _TranslucencyLighting = ECk_Usf_TranslucencyLighting::SurfacePerPixel;
        _LookName        = n"Glass";

        FCk_Usf_ParamDesc Tint;
        Tint._Name = n"TintColor";
        Tint._Type = ECk_Usf_ParamType::Vector;
        Tint._DefaultVector = FLinearColor(0.70, 0.85, 1.0, 1.0);
        _Parameters.Add(Tint);

        FCk_Usf_ParamDesc Alpha;
        Alpha._Name = n"Alpha";
        Alpha._Type = ECk_Usf_ParamType::Scalar;
        Alpha._DefaultScalar = 0.25;
        _Parameters.Add(Alpha);

        FCk_Usf_ParamDesc Ior;
        Ior._Name = n"Ior";
        Ior._Type = ECk_Usf_ParamType::Scalar;
        Ior._DefaultScalar = 1.12;
        _Parameters.Add(Ior);
    }

    asset Skin of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Skin.ush";
        _UshFunctionName = n"CkUsf_Look_Skin";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _ShadingModel    = ECk_Usf_ShadingModel::Subsurface;
        _LookName        = n"Skin";

        FCk_Usf_ParamDesc SkinColor;
        SkinColor._Name = n"SkinColor";
        SkinColor._Type = ECk_Usf_ParamType::Vector;
        SkinColor._DefaultVector = FLinearColor(0.85, 0.60, 0.50, 1.0);
        _Parameters.Add(SkinColor);

        FCk_Usf_ParamDesc ScatterColor;
        ScatterColor._Name = n"ScatterColor";
        ScatterColor._Type = ECk_Usf_ParamType::Vector;
        ScatterColor._DefaultVector = FLinearColor(0.80, 0.15, 0.10, 1.0);
        _Parameters.Add(ScatterColor);

        FCk_Usf_ParamDesc ScatterAmount;
        ScatterAmount._Name = n"ScatterAmount";
        ScatterAmount._Type = ECk_Usf_ParamType::Scalar;
        ScatterAmount._DefaultScalar = 0.7;
        _Parameters.Add(ScatterAmount);
    }

    asset CarPaint of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/CarPaint.ush";
        _UshFunctionName = n"CkUsf_Look_CarPaint";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _ShadingModel    = ECk_Usf_ShadingModel::ClearCoat;
        _LookName        = n"CarPaint";

        FCk_Usf_ParamDesc PaintColor;
        PaintColor._Name = n"PaintColor";
        PaintColor._Type = ECk_Usf_ParamType::Vector;
        PaintColor._DefaultVector = FLinearColor(0.60, 0.05, 0.10, 1.0);
        _Parameters.Add(PaintColor);

        FCk_Usf_ParamDesc Flake;
        Flake._Name = n"Flake";
        Flake._Type = ECk_Usf_ParamType::Scalar;
        Flake._DefaultScalar = 0.15;
        _Parameters.Add(Flake);

        FCk_Usf_ParamDesc CoatRoughness;
        CoatRoughness._Name = n"CoatRoughness";
        CoatRoughness._Type = ECk_Usf_ParamType::Scalar;
        CoatRoughness._DefaultScalar = 0.08;
        _Parameters.Add(CoatRoughness);
    }

    asset PainterlyRuins of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/PainterlyRuins.ush";
        _UshFunctionName = n"CkUsf_Look_PainterlyRuins";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"PainterlyRuins";

        FCk_Usf_ParamDesc SkyTint;
        SkyTint._Name = n"SkyTint";
        SkyTint._Type = ECk_Usf_ParamType::Vector;
        SkyTint._DefaultVector = FLinearColor(0.55, 0.65, 0.74, 1.0);
        _Parameters.Add(SkyTint);

        FCk_Usf_ParamDesc FoliageTint;
        FoliageTint._Name = n"FoliageTint";
        FoliageTint._Type = ECk_Usf_ParamType::Vector;
        FoliageTint._DefaultVector = FLinearColor(0.30, 0.50, 0.28, 1.0);
        _Parameters.Add(FoliageTint);

        FCk_Usf_ParamDesc Speed;
        Speed._Name = n"Speed";
        Speed._Type = ECk_Usf_ParamType::Scalar;
        Speed._DefaultScalar = 1.0;
        _Parameters.Add(Speed);
    }

    // ---- Multi-pass (render-texture) passes ----

    asset SmokeBuffer of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/SmokeBuffer.ush";
        _UshFunctionName = n"CkUsf_Pass_SmokeBuffer";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"SmokeBuffer";

        FCk_Usf_ParamDesc Ch0;
        Ch0._Name = n"iChannel0";
        Ch0._Type = ECk_Usf_ParamType::Texture2D;   // bound to a render target at runtime
        _Parameters.Add(Ch0);

        FCk_Usf_ParamDesc Resolution;
        Resolution._Name = n"iResolution";
        Resolution._Type = ECk_Usf_ParamType::Vector;
        Resolution._DefaultVector = FLinearColor(512.0, 512.0, 0.0, 1.0);
        _Parameters.Add(Resolution);

        FCk_Usf_ParamDesc Frame;
        Frame._Name = n"iFrame";
        Frame._Type = ECk_Usf_ParamType::Scalar;
        Frame._DefaultScalar = 0.0;
        _Parameters.Add(Frame);

        FCk_Usf_ParamDesc Delta;
        Delta._Name = n"iTimeDelta";
        Delta._Type = ECk_Usf_ParamType::Scalar;
        Delta._DefaultScalar = 0.016;
        _Parameters.Add(Delta);
    }

    asset SmokeImage of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/SmokeImage.ush";
        _UshFunctionName = n"CkUsf_Pass_SmokeImage";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"SmokeImage";

        FCk_Usf_ParamDesc Ch0;
        Ch0._Name = n"iChannel0";
        Ch0._Type = ECk_Usf_ParamType::Texture2D;
        Ch0._DefaultTexturePath = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq.Good64x64TilingNoiseHighFreq"; // placeholder; RT bound at runtime
        _Parameters.Add(Ch0);

        FCk_Usf_ParamDesc Resolution;
        Resolution._Name = n"iResolution";
        Resolution._Type = ECk_Usf_ParamType::Vector;
        Resolution._DefaultVector = FLinearColor(512.0, 512.0, 0.0, 1.0);
        _Parameters.Add(Resolution);

        FCk_Usf_ParamDesc Frame;
        Frame._Name = n"iFrame";
        Frame._Type = ECk_Usf_ParamType::Scalar;
        Frame._DefaultScalar = 0.0;
        _Parameters.Add(Frame);

        FCk_Usf_ParamDesc Delta;
        Delta._Name = n"iTimeDelta";
        Delta._Type = ECk_Usf_ParamType::Scalar;
        Delta._DefaultScalar = 0.016;
        _Parameters.Add(Delta);
    }

    asset Blit of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Blit.ush";
        _UshFunctionName = n"CkUsf_Blit";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _LookName        = n"Blit";

        FCk_Usf_ParamDesc Ch0;
        Ch0._Name = n"iChannel0";
        Ch0._Type = ECk_Usf_ParamType::Texture2D;
        Ch0._DefaultTexturePath = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq.Good64x64TilingNoiseHighFreq"; // placeholder; RT bound at runtime
        _Parameters.Add(Ch0);
    }

    // ---- PostProcess (#2: scene-texture access) ----

    asset EdgeOutline of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/EdgeOutline.ush";
        _UshFunctionName = n"CkUsf_PP_EdgeOutline";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"EdgeOutline";

        FCk_Usf_ParamDesc OutlineColor;
        OutlineColor._Name = n"OutlineColor";
        OutlineColor._Type = ECk_Usf_ParamType::Vector;
        OutlineColor._DefaultVector = FLinearColor(0.0, 0.0, 0.0, 1.0);
        _Parameters.Add(OutlineColor);

        // Relative depth-Laplacian scale (~0..0.3). The shader currently hardcodes 0.15;
        // this default is kept on the same scale so the exposed param is not misleading.
        FCk_Usf_ParamDesc DepthThreshold;
        DepthThreshold._Name = n"DepthThreshold";
        DepthThreshold._Type = ECk_Usf_ParamType::Scalar;
        DepthThreshold._DefaultScalar = 0.15;
        _Parameters.Add(DepthThreshold);

        FCk_Usf_ParamDesc NormalThreshold;
        NormalThreshold._Name = n"NormalThreshold";
        NormalThreshold._Type = ECk_Usf_ParamType::Scalar;
        NormalThreshold._DefaultScalar = 0.4;
        _Parameters.Add(NormalThreshold);

        FCk_Usf_ParamDesc Thickness;
        Thickness._Name = n"Thickness";
        Thickness._Type = ECk_Usf_ParamType::Scalar;
        Thickness._DefaultScalar = 1.0;
        _Parameters.Add(Thickness);
    }

    // ---- PostProcess (selective SOLID-COLOR OUTLINE via Custom Depth/Stencil) ----
    // Reads the Custom Stencil buffer to draw a per-preset silhouette around marked objects. Stencil value
    // selects a column of the OutlineParams LUT (written by UCkUsf_OutlineSubsystem). Needs the project's
    // Custom Depth-Stencil pass enabled with stencil (Config/DefaultEngine.ini r.CustomDepth=3).

    asset SolidOutline of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/SolidOutline.ush";
        _UshFunctionName = n"CkUsf_PP_SolidOutline";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"SolidOutline";

        // Pre-TSR/TAA so the stencil-derived outline is temporally accumulated like geometry edges —
        // after-tonemapping placement shimmers under the TAA projection jitter (see SolidOutline.ush).
        _BlendableLocation = ECk_Usf_BlendableLocation::SceneColorAfterDOF;

        // Square (Chebyshev) corners instead of the octagonal default — convex corners come to a sharp
        // point at full edge thickness rather than pinching to 0.707x. Global to all outlined objects
        // (one shared material); revert by removing this define. See SolidOutline.ush for the trade-off.
        _Defines.Add("CKUSF_OUTLINE_SQUARE_CORNERS=1");

        // Custom stencil/depth + scene color/depth (no scene normal needed for a stencil silhouette).
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneColor);
        _SceneTextures.Add(ECk_Usf_SceneTexture::SceneDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::CustomDepth);
        _SceneTextures.Add(ECk_Usf_SceneTexture::CustomStencil);

        // 16 x 2 RGBA16f params LUT, bound at runtime by the outline subsystem. The stock noise texture is a
        // compile-time placeholder so the generated master has a valid texture object.
        FCk_Usf_ParamDesc OutlineParams;
        OutlineParams._Name = n"OutlineParams";
        OutlineParams._Type = ECk_Usf_ParamType::Texture2D;
        OutlineParams._DefaultTexturePath = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq.Good64x64TilingNoiseHighFreq";
        _Parameters.Add(OutlineParams);

        FCk_Usf_ParamDesc GlobalThickness;
        GlobalThickness._Name = n"GlobalThickness";
        GlobalThickness._Type = ECk_Usf_ParamType::Scalar;
        GlobalThickness._DefaultScalar = 5.0;
        _Parameters.Add(GlobalThickness);

        FCk_Usf_ParamDesc StencilMin;
        StencilMin._Name = n"StencilMin";
        StencilMin._Type = ECk_Usf_ParamType::Scalar;
        StencilMin._DefaultScalar = 240.0;
        _Parameters.Add(StencilMin);

        FCk_Usf_ParamDesc StencilMax;
        StencilMax._Name = n"StencilMax";
        StencilMax._Type = ECk_Usf_ParamType::Scalar;
        StencilMax._DefaultScalar = 255.0;
        _Parameters.Add(StencilMax);
    }

    // ---- Decal (fake-light splash) ----
    // Emissive-only radial gradient projected onto whatever surface the decal box
    // intersects — reads as light cast on the surface without any actual light.
    // Apply via a UDecalComponent projecting toward the surface; tune per instance
    // through a MID (Create_MID_ForLook).

    asset GlowDecal of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/GlowDecal.ush";
        _UshFunctionName = n"CkUsf_Look_GlowDecal";
        _Domain          = ECk_Usf_Domain::Decal;
        _LookName        = n"GlowDecal";

        FCk_Usf_ParamDesc GlowColor;
        GlowColor._Name = n"GlowColor";
        GlowColor._Type = ECk_Usf_ParamType::Vector;
        GlowColor._DefaultVector = FLinearColor(1.0, 1.0, 1.0, 1.0);
        _Parameters.Add(GlowColor);

        FCk_Usf_ParamDesc Intensity;
        Intensity._Name = n"Intensity";
        Intensity._Type = ECk_Usf_ParamType::Scalar;
        Intensity._DefaultScalar = 5.0;
        _Parameters.Add(Intensity);

        FCk_Usf_ParamDesc Falloff;
        Falloff._Name = n"Falloff";
        Falloff._Type = ECk_Usf_ParamType::Scalar;
        Falloff._DefaultScalar = 2.5;
        _Parameters.Add(Falloff);

        FCk_Usf_ParamDesc PulseSpeed;
        PulseSpeed._Name = n"PulseSpeed";
        PulseSpeed._Type = ECk_Usf_ParamType::Scalar;
        PulseSpeed._DefaultScalar = 2.0;
        _Parameters.Add(PulseSpeed);

        FCk_Usf_ParamDesc PulseAmount;
        PulseAmount._Name = n"PulseAmount";
        PulseAmount._Type = ECk_Usf_ParamType::Scalar;
        PulseAmount._DefaultScalar = 0.15;
        _Parameters.Add(PulseAmount);
    }
}
