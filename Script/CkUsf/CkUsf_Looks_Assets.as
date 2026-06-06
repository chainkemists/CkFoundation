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
}
