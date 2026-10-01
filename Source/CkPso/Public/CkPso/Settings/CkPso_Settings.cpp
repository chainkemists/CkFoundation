#include "CkPso_Settings.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pso_cvars
{
    static int32 WaitForPrecache = -1;
    static FAutoConsoleVariableRef CVarWaitForPrecache(
        TEXT("ck.Pso.WaitForPrecache"),
        WaitForPrecache,
        TEXT("Hold the loading screen until pending PSO work drains. -1 = use the project setting (default), 0 = off, 1 = on. Any other value uses the project setting."),
        ECVF_Default);

#if NOT UE_BUILD_SHIPPING
    static int32 SimulatedRemaining = -1;
    static FAutoConsoleVariableRef CVarSimulatedRemaining(
        TEXT("ck.Pso.Debug.SimulatedRemaining"),
        SimulatedRemaining,
        TEXT("When >= 0, replaces the engine's pending PSO count fed to the drain tracker (and makes the gate active even where precaching is unsupported, e.g. editor/PIE). -1 = off (default)."),
        ECVF_Default);

    static float StallTimeoutOverride = -1.0f;
    static FAutoConsoleVariableRef CVarStallTimeoutOverride(
        TEXT("ck.Pso.Debug.StallTimeoutOverride"),
        StallTimeoutOverride,
        TEXT("When >= 0, replaces the project's drain stall timeout, in seconds, for windows armed afterwards (0 disables the stall check). -1 = off (default)."),
        ECVF_Default);
#endif

    static bool LogDrainEveryFrame = false;
    static FAutoConsoleVariableRef CVarLogDrainEveryFrame(
        TEXT("ck.Pso.LogDrainEveryFrame"),
        LogDrainEveryFrame,
        TEXT("When true, the PSO drain window's state, pending count and progress are logged every frame while it holds the loading screen."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pso_Settings_UE::
    Get_WaitForPsoPrecache()
    -> ECk_EnableDisable
{
    if (ck_pso_cvars::WaitForPrecache == 0)
    { return ECk_EnableDisable::Disable; }

    if (ck_pso_cvars::WaitForPrecache == 1)
    { return ECk_EnableDisable::Enable; }

    return GetDefault<UCk_Pso_ProjectSettings_UE>()->Get_WaitForPsoPrecache();
}

auto
    UCk_Utils_Pso_Settings_UE::
    Get_DrainQuietPeriod()
    -> FCk_Time
{
    return GetDefault<UCk_Pso_ProjectSettings_UE>()->Get_DrainQuietPeriod();
}

auto
    UCk_Utils_Pso_Settings_UE::
    Get_DrainMaxWait()
    -> FCk_Time
{
    return GetDefault<UCk_Pso_ProjectSettings_UE>()->Get_DrainMaxWait();
}

auto
    UCk_Utils_Pso_Settings_UE::
    Get_DrainStallTimeout()
    -> FCk_Time
{
#if NOT UE_BUILD_SHIPPING
    if (ck_pso_cvars::StallTimeoutOverride >= 0.0f)
    { return FCk_Time{static_cast<double>(ck_pso_cvars::StallTimeoutOverride)}; }
#endif

    return GetDefault<UCk_Pso_ProjectSettings_UE>()->Get_DrainStallTimeout();
}

auto
    UCk_Utils_Pso_Settings_UE::
    Get_SimulatedRemaining()
    -> TOptional<uint32>
{
#if NOT UE_BUILD_SHIPPING
    if (ck_pso_cvars::SimulatedRemaining >= 0)
    { return static_cast<uint32>(ck_pso_cvars::SimulatedRemaining); }
#endif

    return {};
}

auto
    UCk_Utils_Pso_Settings_UE::
    Get_LogDrainEveryFrame()
    -> bool
{
    return ck_pso_cvars::LogDrainEveryFrame;
}

// --------------------------------------------------------------------------------------------------------------------
