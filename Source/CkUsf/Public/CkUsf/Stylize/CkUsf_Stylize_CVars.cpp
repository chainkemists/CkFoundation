#include "CkUsf/Stylize/CkUsf_Stylize_CVars.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

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

    // ---- HandDrawn ----

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
             "3 PaperPattern, 4 WorldNormals, 5 SceneDepth, 6 StylizeMask)."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_HandDrawn_Strength = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_Strength_Ref(
        TEXT("ck.Usf.HandDrawn.Strength"),
        CVarUsfStylize_HandDrawn_Strength,
        TEXT("Override the HandDrawn master style strength (0..1): negative = settings-driven. "
             "This one lerp governs the WHOLE composite — paint, ink, strokes and paper together."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_HandDrawn_Ink = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_Ink_Ref(
        TEXT("ck.Usf.HandDrawn.Ink"),
        CVarUsfStylize_HandDrawn_Ink,
        TEXT("Override the HandDrawn contour ink: -1 settings-driven, 0 off, 1 on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_HandDrawn_InkThickness = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_InkThickness_Ref(
        TEXT("ck.Usf.HandDrawn.InkThickness"),
        CVarUsfStylize_HandDrawn_InkThickness,
        TEXT("Override the HandDrawn ink detector radius in viewport pixels: negative = settings-driven."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_HandDrawn_Strokes = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_Strokes_Ref(
        TEXT("ck.Usf.HandDrawn.Strokes"),
        CVarUsfStylize_HandDrawn_Strokes,
        TEXT("Override the HandDrawn shadow strokes: -1 settings-driven, 0 off, 1 on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_HandDrawn_Paper = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_HandDrawn_Paper_Ref(
        TEXT("ck.Usf.HandDrawn.Paper"),
        CVarUsfStylize_HandDrawn_Paper,
        TEXT("Override the HandDrawn paper grain: -1 settings-driven, 0 off, 1 on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    // ---- CelShade ----

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
             "4 Albedo, 5 Normals, 6 PatternThreshold, 7 PatternCoordinates, 8 MotionOffset, 9 Stencil, "
             "10 StylizeMask)."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CelShade_Bands = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_Bands_Ref(
        TEXT("ck.Usf.CelShade.Bands"),
        CVarUsfStylize_CelShade_Bands,
        TEXT("Override the CelShade band count: negative = settings-driven. Ignored while the settings "
             "are in CustomEdges distribution mode, where the authored edge list IS the placement."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_CelShade_Midpoint = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_Midpoint_Ref(
        TEXT("ck.Usf.CelShade.Midpoint"),
        CVarUsfStylize_CelShade_Midpoint,
        TEXT("Override the CelShade exposure anchor — the illumination that lands mid-ramp: "
             "negative = settings-driven. It is the first knob to tune; every other band control is "
             "relative to it."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CelShade_Pattern = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_Pattern_Ref(
        TEXT("ck.Usf.CelShade.Pattern"),
        CVarUsfStylize_CelShade_Pattern,
        TEXT("Override the CelShade halftone pattern: -1 settings-driven, otherwise the "
             "ECk_Usf_CelPattern index (0 Bayer, 1 RoundDots, 2 SquareDots, 3 Lines, 4 Crosshatch, "
             "5 DiagonalLines, 6 ConcentricCircles, 7 Triangles, 8 ClusteredNoise, 9 Spiral). Meshes "
             "carrying a per-object stencil pattern keep theirs — this is the GLOBAL pattern only."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_CelShade_PatternStrength = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_PatternStrength_Ref(
        TEXT("ck.Usf.CelShade.PatternStrength"),
        CVarUsfStylize_CelShade_PatternStrength,
        TEXT("Override how much of a CelShade band transition the halftone owns (0..1): "
             "negative = settings-driven. 0 hands the transition back to Band Softness."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CelShade_PatternSpace = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_PatternSpace_Ref(
        TEXT("ck.Usf.CelShade.PatternSpace"),
        CVarUsfStylize_CelShade_PatternSpace,
        TEXT("Override where the CelShade pattern cells live: -1 settings-driven, otherwise the "
             "ECk_Usf_CelPatternSpace index (0 World, 1 Screen). Screen is the stable alternative on "
             "translating and skinned meshes."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CelShade_Outline = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CelShade_Outline_Ref(
        TEXT("ck.Usf.CelShade.Outline"),
        CVarUsfStylize_CelShade_Outline,
        TEXT("Override the CelShade look's own ink line: -1 settings-driven, 0 off, 1 on. Unrelated to "
             "the Custom-Stencil silhouettes of UCkUsf_OutlineSubsystem."),
        Get_ChangedDelegate(),
        ECVF_Default);

    // ---- ScreenDither ----

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
             "3 QuantizedWithoutDither, 4 DownsampledInput, 5 StylizeMask)."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_ScreenDither_Pattern = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_Pattern_Ref(
        TEXT("ck.Usf.ScreenDither.Pattern"),
        CVarUsfStylize_ScreenDither_Pattern,
        TEXT("Override the ScreenDither threshold pattern: -1 settings-driven, otherwise the "
             "ECk_Usf_DitherPattern index (0 Bayer2x2, 1 Bayer4x4, 2 Bayer8x8, "
             "3 InterleavedGradientNoise, 4 WhiteNoise, 5 BlueNoise)."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_ScreenDither_ColorSteps = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_ColorSteps_Ref(
        TEXT("ck.Usf.ScreenDither.ColorSteps"),
        CVarUsfStylize_ScreenDither_ColorSteps,
        TEXT("Override the ScreenDither quantization level count: negative = settings-driven. Drives "
             "the ColorSteps and LuminanceSteps palette modes; CustomPalette takes its count from the "
             "authored palette instead."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_ScreenDither_PixelScale = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_PixelScale_Ref(
        TEXT("ck.Usf.ScreenDither.PixelScale"),
        CVarUsfStylize_ScreenDither_PixelScale,
        TEXT("Override the ScreenDither pixelation block size, in input-viewport pixels: "
             "negative = settings-driven. 1 = no pixelation."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_ScreenDither_Strength = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_Strength_Ref(
        TEXT("ck.Usf.ScreenDither.Strength"),
        CVarUsfStylize_ScreenDither_Strength,
        TEXT("Override the ScreenDither dither strength, in units of one quantization step: "
             "negative = settings-driven. 0 = plain banding, 1 = the classic ordered-dither look."),
        Get_ChangedDelegate(),
        ECVF_Default);

    float CVarUsfStylize_ScreenDither_Weight = -1.0f;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_Weight_Ref(
        TEXT("ck.Usf.ScreenDither.Weight"),
        CVarUsfStylize_ScreenDither_Weight,
        TEXT("Override the ScreenDither blend of the reduced frame against the untouched original "
             "(0..1): negative = settings-driven. 0 leaves the look inert but still paying for the pass; "
             "ck.Usf.ScreenDither.Enabled 0 is the real passthrough."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_ScreenDither_Monochrome = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_ScreenDither_Monochrome_Ref(
        TEXT("ck.Usf.ScreenDither.Monochrome"),
        CVarUsfStylize_ScreenDither_Monochrome,
        TEXT("Override the ScreenDither monochrome collapse: -1 settings-driven, 0 off, 1 on. On, the "
             "bands are remapped through the settings' own shadow/highlight tint ramp and the palette "
             "mode stops applying."),
        Get_ChangedDelegate(),
        ECVF_Default);

    // ---- CrossHatch ----

    int32 CVarUsfStylize_CrossHatch_Enabled = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CrossHatch_Enabled_Ref(
        TEXT("ck.Usf.CrossHatch.Enabled"),
        CVarUsfStylize_CrossHatch_Enabled,
        TEXT("Override the CrossHatch stylize effect: -1 settings-driven, 0 force off, 1 force on."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarUsfStylize_CrossHatch_Debug = -1;
    static FAutoConsoleVariableRef CVarUsfStylize_CrossHatch_Debug_Ref(
        TEXT("ck.Usf.CrossHatch.Debug"),
        CVarUsfStylize_CrossHatch_Debug,
        TEXT("Override the CrossHatch debug view: -1 settings-driven, otherwise the "
             "ECk_Usf_CrossHatch_DebugMode index (0 Final, 1 HatchMask, 2 HatchDirection, 3 Darkness, "
             "4 LayerCoverage, 5 WorldNormals, 6 StylizeMask)."),
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
        Get_EnabledOverride_CrossHatch()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CrossHatch_Enabled;
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

    auto
        Get_DebugOverride_CrossHatch()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CrossHatch_Debug;
    }

    auto
        Get_BandsOverride_CelShade()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_Bands;
    }

    auto
        Get_MidpointOverride_CelShade()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_Midpoint;
    }

    auto
        Get_PatternOverride_CelShade()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_Pattern;
    }

    auto
        Get_PatternStrengthOverride_CelShade()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_PatternStrength;
    }

    auto
        Get_PatternSpaceOverride_CelShade()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_PatternSpace;
    }

    auto
        Get_OutlineOverride_CelShade()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_CelShade_Outline;
    }

    auto
        Get_PatternOverride_ScreenDither()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_Pattern;
    }

    auto
        Get_ColorStepsOverride_ScreenDither()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_ColorSteps;
    }

    auto
        Get_PixelScaleOverride_ScreenDither()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_PixelScale;
    }

    auto
        Get_StrengthOverride_ScreenDither()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_Strength;
    }

    auto
        Get_WeightOverride_ScreenDither()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_Weight;
    }

    auto
        Get_MonochromeOverride_ScreenDither()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_ScreenDither_Monochrome;
    }

    auto
        Get_StrengthOverride_HandDrawn()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_Strength;
    }

    auto
        Get_InkOverride_HandDrawn()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_Ink;
    }

    auto
        Get_InkThicknessOverride_HandDrawn()
        -> float
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_InkThickness;
    }

    auto
        Get_StrokesOverride_HandDrawn()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_Strokes;
    }

    auto
        Get_PaperOverride_HandDrawn()
        -> int32
    {
        return ck_usf_stylize_cvars::CVarUsfStylize_HandDrawn_Paper;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::usf::stylize
{
    auto
        Get_FlagOverride(
            int32 InOverride)
        -> TOptional<ECk_EnableDisable>
    {
        if (InOverride < 0)
        { return {}; }

        return InOverride > 0 ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable;
    }

    auto
        Get_ScalarOverride(
            float InOverride)
        -> TOptional<float>
    {
        if (InOverride < 0.0f)
        { return {}; }

        return InOverride;
    }

    auto
        Get_CountOverride(
            int32 InOverride)
        -> TOptional<int32>
    {
        if (InOverride < 0)
        { return {}; }

        return InOverride;
    }

    auto
        Get_ValidatedEnumOverride(
            int32 InOverride,
            const UEnum* InEnum,
            const TCHAR* InCVarName)
        -> TOptional<int32>
    {
        if (InOverride < 0)
        { return {}; }

        const auto EnumIsValid = ck::IsValid(InEnum);

        CK_ENSURE_IF_NOT(EnumIsValid,
            TEXT("[{}] was validated against a null UEnum; the setting's own value is used instead"),
            InCVarName)
        { return {}; }

        const auto OverrideIsValid =
            InEnum->IsValidEnumValue(InOverride) && InOverride != InEnum->GetMaxEnumValue();

        CK_ENSURE_IF_NOT(OverrideIsValid,
            TEXT("[{}] is [{}], which is not a [{}] value; the setting's own value is used instead"),
            InCVarName, InOverride, InEnum->GetName())
        { return {}; }

        return InOverride;
    }
}

// --------------------------------------------------------------------------------------------------------------------
