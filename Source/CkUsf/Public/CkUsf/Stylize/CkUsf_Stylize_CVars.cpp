#include "CkUsf/Stylize/CkUsf_Stylize_CVars.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::usf::stylize
{
    auto
        Get_OnCVarChanged()
        -> FCk_Usf_Stylize_OnCVarChanged&
    {
        static FCk_Usf_Stylize_OnCVarChanged OnCVarChanged;
        return OnCVarChanged;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_stylize_cvars
{
    auto DoBroadcast_Changed(IConsoleVariable* InCVar) -> void
    {
        ck::usf::stylize::Get_OnCVarChanged().Broadcast();
    }

    auto Get_ChangedDelegate() -> FConsoleVariableDelegate
    {
        return FConsoleVariableDelegate::CreateStatic(&DoBroadcast_Changed);
    }

    int32 CVarUsfStylize_HandDrawn_Enabled = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_Enabled_Ref(
        TEXT("ck.Usf.HandDrawn.Enabled"),
        CVarUsfStylize_HandDrawn_Enabled,
        TEXT("Override the HandDrawn stylize effect: -1 settings-driven, 0 force off, 1 force on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_HandDrawn_Debug = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_Debug_Ref(
        TEXT("ck.Usf.HandDrawn.Debug"),
        CVarUsfStylize_HandDrawn_Debug,
        TEXT("Override the HandDrawn debug view: -1 settings-driven, otherwise the "
             "ECk_Usf_HandDrawn_DebugMode index (0 FinalImage, 1 InkMask, 2 ShadowStrokeMask, "
             "3 PaperPattern, 4 WorldNormals, 5 SceneDepth)."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CelShade_Enabled = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_Enabled_Ref(
        TEXT("ck.Usf.CelShade.Enabled"),
        CVarUsfStylize_CelShade_Enabled,
        TEXT("Override the CelShade stylize effect: -1 settings-driven, 0 force off, 1 force on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CelShade_Debug = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_Debug_Ref(
        TEXT("ck.Usf.CelShade.Debug"),
        CVarUsfStylize_CelShade_Debug,
        TEXT("Override the CelShade debug view: -1 settings-driven, otherwise the "
             "ECk_Usf_CelShade_DebugMode index (0 Final, 1 BandIndex, 2 OutlineMask, 3 Illumination, "
             "4 Albedo, 5 Normals, 6 PatternThreshold, 7 PatternCoordinates, 8 MotionOffset, 9 Stencil)."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_ScreenDither_Enabled = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_Enabled_Ref(
        TEXT("ck.Usf.ScreenDither.Enabled"),
        CVarUsfStylize_ScreenDither_Enabled,
        TEXT("Override the ScreenDither stylize effect: -1 settings-driven, 0 force off, 1 force on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_ScreenDither_Debug = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_Debug_Ref(
        TEXT("ck.Usf.ScreenDither.Debug"),
        CVarUsfStylize_ScreenDither_Debug,
        TEXT("Override the ScreenDither debug view: -1 settings-driven, otherwise the "
             "ECk_Usf_ScreenDither_DebugMode index (0 Final, 1 Pattern, 2 QuantizationError, "
             "3 QuantizedWithoutDither, 4 DownsampledInput)."),
        Get_ChangedDelegate(),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::usf::stylize
{
    auto
        Get_EnabledOverride_HandDrawn()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_Enabled;
    }

    auto
        Get_EnabledOverride_CelShade()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_Enabled;
    }

    auto
        Get_EnabledOverride_ScreenDither()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_Enabled;
    }

    auto
        Get_DebugOverride_HandDrawn()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_Debug;
    }

    auto
        Get_DebugOverride_CelShade()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_Debug;
    }

    auto
        Get_DebugOverride_ScreenDither()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_Debug;
    }
}

// --------------------------------------------------------------------------------------------------------------------
