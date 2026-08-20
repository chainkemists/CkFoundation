#include "CkPixelArtRenderer/CkPixelArtRenderer_CVars.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pixel_art
{
    auto
        Get_OnCVarChanged()
        -> FCk_PixelArt_OnCVarChanged&
    {
        static FCk_PixelArt_OnCVarChanged OnCVarChanged;
        return OnCVarChanged;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_cvars
{
    auto
        DoBroadcast_Changed(
            IConsoleVariable* InCVar)
        -> void
    {
        ck::pixel_art::Get_OnCVarChanged().Broadcast();
    }

    auto
        Get_ChangedDelegate()
        -> FConsoleVariableDelegate
    {
        return FConsoleVariableDelegate::CreateStatic(&DoBroadcast_Changed);
    }

    int32 CVarPixelArt_Enabled = -1;
    static FAutoConsoleVariableRef CVarPixelArt_Enabled_Ref(
        TEXT("ck.PixelArt.Enabled"),
        CVarPixelArt_Enabled,
        TEXT("Override whether the pixel-art renderer runs: -1 configuration-driven, 0 force off, 1 force on.\n")
        TEXT("Requires an anti-aliasing method of None or FXAA. TSR and TAAU switch the view to temporal\n")
        TEXT("upscaling, which structurally disables the spatial upscale slot this renderer occupies — it then\n")
        TEXT("silently stops running, so `ck.PixelArt.Debug.LogState 1` warns when that happens."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarPixelArt_InternalHeight = -1;
    static FAutoConsoleVariableRef CVarPixelArt_InternalHeight_Ref(
        TEXT("ck.PixelArt.InternalHeight"),
        CVarPixelArt_InternalHeight,
        TEXT("Override the authored vertical texel count (e.g. 360 for 360p): negative = configuration-driven.\n")
        TEXT("The target width is derived from this and the viewport aspect."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarPixelArt_TexelsPerPixel = -1;
    static FAutoConsoleVariableRef CVarPixelArt_TexelsPerPixel_Ref(
        TEXT("ck.PixelArt.TexelsPerPixel"),
        CVarPixelArt_TexelsPerPixel,
        TEXT("Override the resolution mode to output-pixels-per-texel and set the ratio: negative =\n")
        TEXT("configuration-driven. Setting this switches the mode as well as the value, because a ratio with\n")
        TEXT("the mode left on FixedHeight would be read by nothing and look like the override was ignored."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarPixelArt_Margin = -1;
    static FAutoConsoleVariableRef CVarPixelArt_Margin_Ref(
        TEXT("ck.PixelArt.Margin"),
        CVarPixelArt_Margin,
        TEXT("Override the render margin in texels per side: negative = configuration-driven.\n")
        TEXT("The margin is what lets the sampling window shift by the camera's sub-texel snap remainder\n")
        TEXT("without reading texels that were never rendered. 0 disables it and will smear the edges."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarPixelArt_Snap = -1;
    static FAutoConsoleVariableRef CVarPixelArt_Snap_Ref(
        TEXT("ck.PixelArt.Snap"),
        CVarPixelArt_Snap,
        TEXT("Override camera texel-grid snapping: -1 configuration-driven, 0 force off, 1 force on.\n")
        TEXT("Off is the A/B control for pixel creep."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarPixelArt_Filter = -1;
    static FAutoConsoleVariableRef CVarPixelArt_Filter_Ref(
        TEXT("ck.PixelArt.Filter"),
        CVarPixelArt_Filter,
        TEXT("Override the upscale filter: -1 configuration-driven, 0 BoxFilter, 1 Nearest.\n")
        TEXT("Nearest is a debug filter — it makes the texel grid unambiguous while diagnosing snap or margin\n")
        TEXT("problems, at the cost of the aliasing the box filter exists to avoid."),
        Get_ChangedDelegate(),
        ECVF_Default);

    int32 CVarPixelArt_SnapOnly = 0;
    static FAutoConsoleVariableRef CVarPixelArt_SnapOnly_Ref(
        TEXT("ck.PixelArt.Debug.SnapOnly"),
        CVarPixelArt_SnapOnly,
        TEXT("Snap the camera to the texel grid WITHOUT re-applying the sub-texel remainder. 0 off, 1 on.\n")
        TEXT("The picture must visibly advance in whole texels — that is the proof the snap is actually live,\n")
        TEXT("and the middle state between creeping pixels and finished smooth motion."),
        Get_ChangedDelegate(),
        ECVF_Cheat);

    int32 CVarPixelArt_FreezeSnap = 0;
    static FAutoConsoleVariableRef CVarPixelArt_FreezeSnap_Ref(
        TEXT("ck.PixelArt.Debug.FreezeSnap"),
        CVarPixelArt_FreezeSnap,
        TEXT("Hold the last snapped view origin instead of snapping the live one. 0 off, 1 on.\n")
        TEXT("The image must stop moving while the gameplay camera keeps going, which separates a render-side\n")
        TEXT("problem from a camera-side one."),
        Get_ChangedDelegate(),
        ECVF_Cheat);

    float CVarPixelArt_CompSign = 1.0f;
    static FAutoConsoleVariableRef CVarPixelArt_CompSign_Ref(
        TEXT("ck.PixelArt.Debug.CompSign"),
        CVarPixelArt_CompSign,
        TEXT("Multiplier on the snap compensation applied by the upscaler: 1 derived default, -1 inverted.\n")
        TEXT("If motion looks smooth but the picture still creeps, the compensation is being applied the wrong\n")
        TEXT("way round and this settles it without a rebuild."),
        Get_ChangedDelegate(),
        ECVF_Cheat);

    int32 CVarPixelArt_LogState = 0;
    static FAutoConsoleVariableRef CVarPixelArt_LogState_Ref(
        TEXT("ck.PixelArt.Debug.LogState"),
        CVarPixelArt_LogState,
        TEXT("Log a line on every pixel-art renderer state transition (enable, disable, resolution change, and\n")
        TEXT("the anti-aliasing tripwire). 0 off, 1 on."),
        Get_ChangedDelegate(),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pixel_art
{
    auto
        Fold_Overrides(
            FCk_PixelArt_RenderConfig& InOutConfig)
        -> void
    {
        if (ck_pixel_art_cvars::CVarPixelArt_Enabled >= 0)
        { InOutConfig.Enabled = ck_pixel_art_cvars::CVarPixelArt_Enabled != 0; }

        if (ck_pixel_art_cvars::CVarPixelArt_InternalHeight >= 0)
        { InOutConfig.InternalHeight = ck_pixel_art_cvars::CVarPixelArt_InternalHeight; }

        if (ck_pixel_art_cvars::CVarPixelArt_TexelsPerPixel > 0)
        {
            InOutConfig.ResolutionMode = ECk_PixelArt_ResolutionMode::TexelsPerPixel;
            InOutConfig.TexelsPerPixel = ck_pixel_art_cvars::CVarPixelArt_TexelsPerPixel;
        }

        if (ck_pixel_art_cvars::CVarPixelArt_Margin >= 0)
        { InOutConfig.MarginTexels = ck_pixel_art_cvars::CVarPixelArt_Margin; }

        if (ck_pixel_art_cvars::CVarPixelArt_Snap >= 0)
        { InOutConfig.SnapEnabled = ck_pixel_art_cvars::CVarPixelArt_Snap != 0; }

        if (ck_pixel_art_cvars::CVarPixelArt_Filter >= 0)
        {
            InOutConfig.FilterMode = ck_pixel_art_cvars::CVarPixelArt_Filter == 0
                ? ECk_PixelArt_UpscaleFilter::BoxFilter
                : ECk_PixelArt_UpscaleFilter::Nearest;
        }
    }

    auto
        Get_LogStateEnabled()
        -> bool
    {
        return ck_pixel_art_cvars::CVarPixelArt_LogState != 0;
    }

    auto
        Get_DebugSnapOverrides()
        -> FCk_PixelArt_DebugSnapOverrides
    {
        auto Overrides = FCk_PixelArt_DebugSnapOverrides{};
        Overrides.SnapOnly = ck_pixel_art_cvars::CVarPixelArt_SnapOnly != 0;
        Overrides.FreezeSnap = ck_pixel_art_cvars::CVarPixelArt_FreezeSnap != 0;
        Overrides.CompensationSign = ck_pixel_art_cvars::CVarPixelArt_CompSign >= 0.0f ? 1.0f : -1.0f;

        return Overrides;
    }
}
