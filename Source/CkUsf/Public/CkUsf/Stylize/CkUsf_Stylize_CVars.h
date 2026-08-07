#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

// Console overrides for the four Stylize effects:
// `ck.Usf.{HandDrawn,CelShade,ScreenDither,CrossHatch}.{Enabled,Debug}`.
//
// Both kinds use -1 as "no override" so that a project's settings and presets stay the source of truth and
// the console is strictly a developer overlay on top: an absent CVar cannot be distinguished from a CVar
// that happens to agree with the settings, which is what makes it safe to leave one set in an .ini.
namespace ck::usf::stylize
{
    DECLARE_MULTICAST_DELEGATE(FCk_Usf_Stylize_OnCVarChanged);

    // -1 = the subsystem's own settings decide; 0 = force disabled; 1 = force enabled.
    CKUSF_API auto Get_EnabledOverride_HandDrawn() -> int32;
    CKUSF_API auto Get_EnabledOverride_CelShade() -> int32;
    CKUSF_API auto Get_EnabledOverride_ScreenDither() -> int32;
    CKUSF_API auto Get_EnabledOverride_CrossHatch() -> int32;

    // -1 = the subsystem's own settings decide; otherwise the effect's DebugMode enum index to force.
    CKUSF_API auto Get_DebugOverride_HandDrawn() -> int32;
    CKUSF_API auto Get_DebugOverride_CelShade() -> int32;
    CKUSF_API auto Get_DebugOverride_ScreenDither() -> int32;
    CKUSF_API auto Get_DebugOverride_CrossHatch() -> int32;

    // Broadcast on every change to any of the eight. The subsystems re-sync from it, which is the whole
    // difference between a console flip taking effect when it is typed and taking effect at whatever
    // later moment something else happens to write settings.
    CKUSF_API auto Get_OnCVarChanged() -> FCk_Usf_Stylize_OnCVarChanged&;
}

// --------------------------------------------------------------------------------------------------------------------
