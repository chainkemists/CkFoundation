#pragma once

#include <CoreMinimal.h>
#include <UObject/Class.h>

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

// Console overrides for the four Stylize effects, all under `ck.Usf.<Effect>.<Setting>`.
//
// EVERY override uses a NEGATIVE value as "no override" so that a project's settings and presets stay the
// source of truth and the console is strictly a developer overlay on top: an absent CVar cannot be
// distinguished from a CVar that happens to agree with the settings, which is what makes it safe to leave
// one set in an .ini. That convention only works because every setting these reach has a non-negative
// domain — a negative value can never be a legitimate override, so one number means both.
//
// The overlay is folded in on the way to the MID by each subsystem's DoGet_EffectiveSettings. It is never
// written into the stored settings, so Get_Settings() keeps reporting what the game asked for while the
// screen shows what the console asked for, and clearing a CVar back to -1 restores the rendered value
// with no re-apply.
//
// Only `Enabled` may bring an effect into existence: the subsystems' re-sync guard refuses to instantiate
// an effect it did not already find alive unless Enabled is explicitly forced to 1. Every other override
// here is inert in a world that is not already running the effect.
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

    // ---- CelShade tuning ----
    CKUSF_API auto Get_BandsOverride_CelShade() -> int32;
    CKUSF_API auto Get_MidpointOverride_CelShade() -> float;
    CKUSF_API auto Get_PatternOverride_CelShade() -> int32;
    CKUSF_API auto Get_PatternStrengthOverride_CelShade() -> float;
    CKUSF_API auto Get_PatternSpaceOverride_CelShade() -> int32;
    CKUSF_API auto Get_OutlineOverride_CelShade() -> int32;

    // ---- ScreenDither tuning ----
    CKUSF_API auto Get_PatternOverride_ScreenDither() -> int32;
    CKUSF_API auto Get_ColorStepsOverride_ScreenDither() -> int32;
    CKUSF_API auto Get_PixelScaleOverride_ScreenDither() -> float;
    CKUSF_API auto Get_StrengthOverride_ScreenDither() -> float;
    CKUSF_API auto Get_WeightOverride_ScreenDither() -> float;
    CKUSF_API auto Get_MonochromeOverride_ScreenDither() -> int32;

    // ---- HandDrawn tuning ----
    CKUSF_API auto Get_StrengthOverride_HandDrawn() -> float;
    CKUSF_API auto Get_InkOverride_HandDrawn() -> int32;
    CKUSF_API auto Get_InkThicknessOverride_HandDrawn() -> float;
    CKUSF_API auto Get_StrokesOverride_HandDrawn() -> int32;
    CKUSF_API auto Get_PaperOverride_HandDrawn() -> int32;

    // Broadcast on every change to any of them. The subsystems re-sync from it, which is the whole
    // difference between a console flip taking effect when it is typed and taking effect at whatever
    // later moment something else happens to write settings.
    CKUSF_API auto Get_OnCVarChanged() -> FCk_Usf_Stylize_OnCVarChanged&;

    // ---- Fold-in helpers: unset means "leave the setting alone" ----

    // 0 = force Disable, 1 (or anything above) = force Enable. Mirrors the Enabled overlay's own
    // semantics rather than inventing a second convention for the same shape.
    CKUSF_API auto Get_FlagOverride(int32 InOverride) -> TOptional<ECk_EnableDisable>;

    CKUSF_API auto Get_ScalarOverride(float InOverride) -> TOptional<float>;

    CKUSF_API auto Get_CountOverride(int32 InOverride) -> TOptional<int32>;

    // The _MAX trap, in one place. UHT appends a _MAX enumerator to every UENUM and IsValidEnumValue
    // ACCEPTS it, so a one-past-the-end console value passes a naive check and then falls through the
    // shader's if-chain to whatever its else branch is — an override that silently is not one. A value
    // naming no enumerator is rejected LOUDLY and leaves the setting's own value in place.
    CKUSF_API auto Get_ValidatedEnumOverride(
        int32 InOverride,
        const UEnum* InEnum,
        const TCHAR* InCVarName) -> TOptional<int32>;

    template <typename T_Enum>
    auto Get_EnumOverride(
        int32 InOverride,
        const TCHAR* InCVarName) -> TOptional<T_Enum>
    {
        const auto Validated = Get_ValidatedEnumOverride(InOverride, StaticEnum<T_Enum>(), InCVarName);

        if (NOT Validated.IsSet())
        { return {}; }

        return static_cast<T_Enum>(*Validated);
    }
}

// --------------------------------------------------------------------------------------------------------------------
