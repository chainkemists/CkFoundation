#include "CkPixelArtRender/CkPixelArtRender_CVars.h"

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
}
