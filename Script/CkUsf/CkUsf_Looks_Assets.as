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
}
